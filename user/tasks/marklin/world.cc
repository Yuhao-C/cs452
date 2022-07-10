#include "marklin/world.h"

#include "clock_server.h"
#include "display_server.h"
#include "marklin/routing.h"
#include "marklin_server.h"
#include "name_server.h"
#include "train_data.h"
#include "user/message.h"
#include "user/task.h"

#define WORLD_NAME "WORLD"

namespace marklin {

void runWorld() {
  World world;
  world.run();
}

void reverseWorker() {
  int worldTid, speed, trainId;
  Msg msg;
  receive(worldTid, msg);
  reply(worldTid);
  speed = msg.data[0];
  trainId = msg.data[1];

  send(worldTid, Msg::tr(16, trainId));
  clock::delay(400);
  send(worldTid, Msg::tr(31, trainId));
  send(worldTid, Msg::tr(speed | 16, trainId));
}

World::World()
    : trackSet{TrackSet::Unknown},
      trackSize{0},
      marklinServerTid{-1},
      routingServerTid{-1},
      displayServerTid{-1} {
  initTrains(trains);
}

void World::run() {
  registerAs(WORLD_NAME);
  int senderTid;
  Msg msg;
  while (true) {
    receive(senderTid, msg);

    // ignore all commands before initialization
    if (trackSize == 0 && msg.action != Msg::Action::InitTrack) {
      reply(senderTid, -1);
      continue;
    }

    switch (msg.action) {
      case Msg::Action::InitTrack:
        reply(senderTid, 0);
        init((TrackSet)msg.data[0]);
        break;
      case Msg::Action::SetTrainLoc:
        reply(senderTid, 0);
        onSetTrainLoc(msg);
        break;
      case Msg::Action::SwitchCmd:
        reply(senderTid, 0);
        onSwitch(msg);
        break;
      case Msg::Action::SensorTriggered:
        reply(senderTid, 0);
        onSensorTrigger(msg);
        break;
      case Msg::Action::SetDestination:
        reply(senderTid, 0);
        onSetDestination(msg);
        break;
      case Msg::Action::TrainCmd:
        reply(senderTid, 0);
        onSetTrainSpeed(senderTid, msg);
        break;
      case Msg::Action::ReverseCmd:
        reply(senderTid, 0);
        onReverseTrain(msg);
        break;
    }
  }
}

void World::init(TrackSet trackSet) {
  this->trackSet = trackSet;
  if (trackSet == TrackSet::TrackA) {
    trackSize = init_tracka(track);
  } else {
    trackSize = init_trackb(track);
  }

  marklinServerTid = whoIs(MARKLIN_SERVER_NAME);
  displayServerTid = whoIs(DISPLAY_SERVER_NAME);

  // initialize switches
  char switchInitA[] = "SSSSCSSSSSCSSSSSSCSCSC";
  char switchInitB[] = "SSSSCSSSSSCSSSSSSCSCCS";
  char *switchInit = trackSet == TrackSet::TrackA ? switchInitA : switchInitB;

  for (int i = 0; i < 22; ++i) {
    track[80 + 2 * i].status = switchInit[i] == 'S' ? DIR_STRAIGHT : DIR_CURVED;
    int switchDirection = switchInit[i];
    int switchNum = track[80 + 2 * i].num;
    send(marklinServerTid, marklin::Msg::sw(switchDirection, switchNum));
    send(displayServerTid,
         view::Msg{view::Action::Switch, {switchDirection, switchNum}});
  }

  // send track set to routing server
  while (routingServerTid < 0) {
    routingServerTid = whoIs(ROUTING_SERVER_NAME);
  }
  send(routingServerTid, trackSet);
}

void World::onSetTrainLoc(const Msg &msg) {
  int trainId = msg.data[0];
  int nodeIdx = msg.data[1];
  int offset = msg.data[2];
  int direction = msg.data[3];

  Train *t = getTrain(trainId);
  t->locNodeIdx = nodeIdx;
  t->locOffset = offset;
  t->nextSensor = &track[nodeIdx];
  t->nextSensorTick = 0;
  t->direction =
      direction == 'f' ? Train::Direction::Forward : Train::Direction::Backward;

  // send to display server
  send(displayServerTid,
       view::Msg{view::Action::Predict,
                 {t->id, (int)t->nextSensor->name, t->nextSensorTick, 0, 0},
                 5});
}

void World::onSwitch(const Msg &msg) {
  int direction = msg.data[0];
  int switchId = msg.data[1];
  getBranch(switchId)->status =
      direction == SWITCH_S ? DIR_STRAIGHT : DIR_CURVED;
  send(marklinServerTid, msg);
  send(displayServerTid,
       view::Msg{view::Action::Switch,
                 {direction == SWITCH_S ? 'S' : 'C', switchId}});
}

void World::onSensorTrigger(const Msg &msg) {
  int sensorNum = msg.data[0];
  int tick = msg.data[1];

  int offDistance;
  Train *t = predictTrainBySensor(sensorNum, offDistance);

  if (!t) {
    // no train is expected to trigger this sensor
    // something went wrong
    send(marklinServerTid, Msg::stop());
    return;
  }

  int totalDistance = t->nextSensorDist + offDistance;
  int totalTick = tick - t->lastSensorTick;
  int avgVelocity = totalDistance * 100 / totalTick;

  int velocity = t->getVelocity();  // get pre-calibrated train speed
  int timeDiff = tick - t->nextSensorTick;
  int distDiff = velocity * timeDiff / 100000;

  t->nextSensor = findNextSensor(getSensor(sensorNum), t->nextSensorDist);
  t->nextSensorTick = tick + t->nextSensorDist * 100 / velocity;

  t->lastSensor = getSensor(sensorNum);
  t->lastSensorTick = tick;

  if (t->nextSensor->type == NODE_SENSOR) {
    int distance;
    track_node *next2 = findNextSensor(t->nextSensor, distance);
    int next2Num = next2->type == NODE_SENSOR ? next2->num : -1;
    // send to routing
    send(routingServerTid, Msg{Msg::Action::SensorTriggered,
                               {sensorNum, tick, t->id, next2Num},
                               4});
  } else {
    // type is NODE_EXIT
    // may need to do something?
  }

  // send to display server
  send(displayServerTid,
       view::Msg{view::Action::Predict,
                 {t->id, (int)t->nextSensor->name, t->nextSensorTick, timeDiff,
                  distDiff, avgVelocity},
                 6});
}

void World::onSetDestination(const Msg &msg) {
  Train *train = getTrain(msg.data[0]);
  // send to routing
  // clang-format off
  Msg msgSend{
    Msg::Action::SetDestination,
    {
      train->id,
      train->accelSlow,
      train->accelDelay,
      train->accelDist,
      train->decelSlow,
      train->stopDist,
      train->velocity,
      train->locNodeIdx,
      train->locOffset - train->direction,
      msg.data[1],
      msg.data[2] - train->direction
    },
    11
  };
  // clang-format on
  send(routingServerTid, msgSend);
}

int World::onSetTrainSpeed(int senderTid, const Msg &msg) {
  Train *train = getTrain(msg.data[1]);
  int cmd = msg.data[0];
  if (0 <= cmd && cmd <= 31) {
    if (!train->isReversing || senderTid == train->reverseTid) {
      // perform action on reverse
      if (cmd == 15 || cmd == 31) {
        // TODO: need to check NODE_EXIT
        train->nextSensor =
            findNextSensor(train->nextSensor->reverse, train->nextSensorDist);
        train->isReversing = false;
        train->reverseDirection();

        // send to display server
        send(displayServerTid,
             view::Msg{view::Action::Predict,
                       {train->id, (int)train->nextSensor->name, 0, 0, 0},
                       5});
      }

      train->setSpeedLevel(Train::getSpeedLevel(cmd));
      // println(COM2, "%d", clock::time());
      send(marklinServerTid, msg);
      return 0;
    }
    return 1;
  }
  send(marklinServerTid, msg);
  return 0;
}

int World::onReverseTrain(const Msg &msg) {
  Train *train = getTrain(msg.data[1]);
  if (train->isReversing) {
    return -1;
  } else {
    train->isReversing = true;
    train->reverseTid = create(2, reverseWorker);
    send(train->reverseTid, Msg{Msg::Action::ReverseCmd,
                                {train->getSpeedLevelInt(), msg.data[1]}});
    return 0;
  }
}

Train *World::predictTrainBySensor(int sensorNum, int &offDist) {
  int searchRound = 2;
  track_node *expected[6];
  int offDistances[6];

  int predicted = -1;
  for (int i = 0; i < 6; ++i) {
    Train *t = &trains[i];
    if (t->nextSensor && t->nextSensor->type == NODE_SENSOR &&
        t->nextSensor->num == sensorNum) {
      if (predicted < 0 ||
          t->nextSensorTick < trains[predicted].nextSensorTick) {
        predicted = i;
      }
    }
    expected[i] = t->nextSensor;
    offDistances[i] = 0;
  }

  if (predicted >= 0) {
    offDist = 0;
    return &trains[predicted];
  }

  for (int round = 0; round < searchRound; ++round) {
    for (int i = 0; i < 6; ++i) {
      if (expected[i] && expected[i]->type == NODE_SENSOR) {
        int distance;
        expected[i] = findNextSensor(expected[i], distance);
        offDistances[i] += distance;
        if (expected[i] && expected[i]->type == NODE_SENSOR &&
            expected[i]->num == sensorNum) {
          offDist = offDistances[i];
          return &trains[i];
          // TODO: check train expected time/distance to find out which train
          // when one sensor broken
          // TODO: handle case when one switch broken
        }
      }
    }
  }

  if (predicted >= 0) {
    offDist = offDistances[predicted];
    return &trains[predicted];
  }

  return nullptr;
}

track_node *World::getSensor(int sensorNum) { return &track[sensorNum]; }

track_node *World::getBranch(int switchNum) {
  int index;
  if (switchNum >= 153) {
    index = 80 + 2 * (switchNum - 153 + 18);
  } else {
    index = 80 + 2 * (switchNum - 1);
  }
  return &track[index];
}

track_node *World::findNextSensor(track_node *sensor, int &distance) {
  track_node *next = sensor->edge[0].dest;
  distance = sensor->edge[0].dist;
  while (next->type != NODE_SENSOR && next->type != NODE_EXIT) {
    if (next->type == NODE_BRANCH) {
      distance += next->edge[next->status].dist;
      next = next->edge[next->status].dest;
    } else {
      distance += next->edge[DIR_AHEAD].dist;
      next = next->edge[DIR_AHEAD].dest;
    }
  }
  return next;
}

int World::getTrackSize() const { return trackSize; }

Train *World::getTrain(int trainId) {
  for (int i = 0; i < 6; ++i) {
    if (trains[i].id == trainId) {
      return &trains[i];
    }
  }
  return nullptr;
}

}  // namespace marklin