#include "marklin/world.h"

#include "clock_server.h"
#include "display_server.h"
#include "lib/async_msg.h"
#include "lib/log.h"
#include "marklin/reservation.h"
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

    reply(senderTid, 0);

    switch (msg.action) {
      case Msg::Action::InitTrack:
        init((TrackSet)msg.data[0]);
        break;
      case Msg::Action::SetTrainLoc:
        onSetTrainLoc(msg);
        break;
      case Msg::Action::SwitchCmd:
        onSwitch(msg);
        break;
      case Msg::Action::SensorTriggered:
        onSensorTrigger(msg);
        break;
      case Msg::Action::SetDestination:
      case Msg::Action::Reroute:
        onSetDestination(msg);
        break;
      case Msg::Action::TrainCmd:
        onSetTrainSpeed(senderTid, msg);
        break;
      case Msg::Action::ReverseCmd:
        onReverseTrain(msg);
        break;
      case Msg::Action::Depart:
        onTrainDepart(msg);
        break;
      case Msg::Action::SetTrainBlocked:
        getTrain(msg.data[0])->isBlocked = msg.data[1];
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

  send(marklinServerTid, Msg::go());
  for (int i = 0; i < NUM_TRAINS; ++i) {
    send(marklinServerTid, Msg::tr(16, trains[i].id));
  }

  // initialize switches
  char switchInitA[] = "SSCSCSSSSSCSSSSSSCSCSC";
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
  int temp;

  Train *t = getTrain(trainId);
  t->locNodeIdx = nodeIdx;
  t->locOffset = offset;
  t->nextSensor = offset <= 0 ? &track[nodeIdx] : findNextSensor(&track[nodeIdx], temp);
  t->nextSensorTick = 0;
  t->direction =
      direction == 'f' ? Train::Direction::Forward : Train::Direction::Backward;

  // reserve occupied track
  ResvRequest request;
  int resvServer = whoIs(RESERVATION_SERVER_NAME);
  send(resvServer, request, request);
  // request.print();
  if (offset <= 0) {
    assert(request.reserve(trainId, track[nodeIdx].leaveSeg[0]));
  } else {
    assert(request.reserve(trainId, track[nodeIdx].enterSeg[0]));
  }
  // request.print();
  send(resvServer, request);

  // send to display server TODO: change format
  // clang-format off
  send(displayServerTid, view::Msg{
    view::Action::Train, {
      t - trains, view::TrainStatus::Stationary,
      t->locNodeIdx, t->locOffset,
      t->viaNodeIdx, t->viaOffset,
      t->destNodeIdx, t->destOffset,
    }
  });
  // send(displayServerTid,
  //      view::Msg{view::Action::Predict,
  //                {t->id, (int)t->nextSensor->name, t->nextSensorTick, 0, 0},
  //                5});
  // clang-format on
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

  ResvRequest temp, request;
  int resvServerTid = whoIs(RESERVATION_SERVER_NAME);
  send(resvServerTid, temp, request);
  // request.print();

  if (!t) {
    // no train is expected to trigger this sensor
    // this should happen only if a switch is broken
    // for (int i = 0; i < NUM_TRAINS; ++i) {
    //   send(marklinServerTid, Msg::tr(0, trains[i].id));
    // }
    log("cannot find train");
    send(marklinServerTid, Msg::stop());
    return;
  }

  track_node *sensor = getSensor(sensorNum);
  if (t->nextSensor && t->nextSensor != sensor) {
    // if a sensor is skipped
    log("skipped sensor");
    if (!freeSegments(t->nextSensor, t, request)) {
      // cannot free a track (not reserved by the train)
      // send(marklinServerTid, Msg::tr(0, t->id));
      log("cannot free t->nextSensor");
      send(marklinServerTid, Msg::stop());
      return;
    }
    t->lastSensor = t->nextSensor;
    log("recovered skipped sensor");
  }
  if (!freeSegments(sensor, t, request)) {
    // cannot free a track (not reserved by the train)
    // send(marklinServerTid, Msg::tr(0, t->id));
    log("cannot free sensor");
    send(marklinServerTid, Msg::stop());
    return;
  }
  send(resvServerTid, request);

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

  for (int i = 0; i < NUM_TRAINS; ++i) {
    if (trains[i].isBlocked) {
      onSetDestination(Msg{Msg::Action::SetDestination, {trains[i].id, -1}, 2});
    }
  }

  // send to display server
  // clang-format off
  send(displayServerTid, view::Msg{
    view::Action::Train, {
      t - trains, view::TrainStatus::PassedSensor,
      sensorNum, 0,
      t->viaNodeIdx, t->viaOffset,
      t->destNodeIdx, t->destOffset,
    }
  });
  // send(displayServerTid,
  //      view::Msg{view::Action::Predict,
  //                {t->id, (int)t->nextSensor->name, t->nextSensorTick, timeDiff,
  //                 distDiff, avgVelocity},
  //                6});
  // clang-format on
}

void World::onSetDestination(const Msg &msg) {
  log("world received %s: train %d",
      msg.action == Msg::Action::Reroute ? "reroute" : "setdest", msg.data[0]);
  Train *train = getTrain(msg.data[0]);
  if (msg.data[1] != -1 && msg.action != Msg::Action::Reroute) {
    train->destNodeIdx = msg.data[1];
    train->destOffset = msg.data[2];
  }
  // send to routing
  // clang-format off
  Msg msgSend{
    msg.action,
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
      train->destNodeIdx,
      train->destOffset - train->direction,
      train->direction,
    },
    12
  };
  // clang-format on
  asyncSend(routingServerTid, msgSend);
  log("send onSetDestination %d: from %s+%d to %s+%d", train->id,
      track[train->locNodeIdx].name, train->locOffset,
      track[train->destNodeIdx].name, train->destOffset);
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

        // send to display server TODO: handle reverse
        // send(displayServerTid,
        //      view::Msg{view::Action::Predict,
        //                {train->id, (int)train->nextSensor->name, 0, 0, 0},
        //                5});
      } else if (cmd == 10 || cmd == 26) {
        train->isBlocked = false;
      }

      train->setSpeedLevel(Train::getSpeedLevel(cmd));
      // log("%d", clock::time());
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
  if (train->getSpeedLevel() == Train::SpeedLevel::Zero) {
    train->reverseDirection();
    train->nextSensor =
            findNextSensor(train->nextSensor->reverse, train->nextSensorDist);
    train->locNodeIdx = track[train->locNodeIdx].reverse - track;
    train->locOffset = -(train->locOffset - TRAIN_LENGTH);
    send(marklinServerTid, Msg::tr(31, train->id));
    // clang-format off
    send(displayServerTid, view::Msg{
      view::Action::Train, {
        train - trains,
        train->isBlocked ? view::TrainStatus::Blocked : view::TrainStatus::Stationary,
        train->locNodeIdx, train->locOffset,
        train->viaNodeIdx, train->viaOffset,
        train->destNodeIdx, train->destOffset,
      }
    });
    // clang-format on
  }
  return 0;
}

void World::onTrainDepart(const Msg &msg) {
  Train *train = getTrain(msg.data[0]);
  train->viaNodeIdx = msg.data[1];
  train->viaOffset = msg.data[2] + train->direction;
  // clang-format off
  send(displayServerTid, view::Msg{
    view::Action::Train, {
      train - trains, view::TrainStatus::Departed,
      train->locNodeIdx, train->locOffset,
      train->viaNodeIdx, train->viaOffset,
      train->destNodeIdx, train->destOffset,
    }
  });
  train->locNodeIdx = train->viaNodeIdx;
  train->locOffset = train->viaOffset;
  log("train %d via %s+%d", train->id, track[train->locNodeIdx].name, train->viaOffset);
  // clang-format on
}

Train *World::predictTrainBySensor(int sensorNum, int &offDist) {
  track_node *expected[NUM_TRAINS];
  int offDistances[NUM_TRAINS];

  for (int i = 0; i < NUM_TRAINS; ++i) {
    Train *t = &trains[i];
    if (t->nextSensor && t->nextSensor->type == NODE_SENSOR &&
        t->nextSensor->num == sensorNum) {
      offDist = 0;
      return t;
    }
    expected[i] = t->nextSensor;
    offDistances[i] = 0;
  }

  for (int i = 0; i < NUM_TRAINS; ++i) {
    Train *t = &trains[i];
    if (!t->isBlocked && expected[i] && expected[i]->type == NODE_SENSOR) {
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

  return nullptr;
}

bool World::freeSegments(track_node *sensor, Train *train,
                         ResvRequest &request) {
  // leave current segment
  log("try to free %d", sensor->leaveSeg[0]);
  if (!request.free(train->id, sensor->leaveSeg[0])) {
    return false;
  }

  int leaveSeg = sensor->leaveSeg[1];
  // if coming from a segment boundary which is a branch

  if (leaveSeg >= 0 && train->lastSensor &&
      leaveSeg == train->lastSensor->enterSeg[0]) {
    log("try to free %d (branch)", leaveSeg);
    if (!request.free(train->id, leaveSeg)) {
      return false;
    }
  }
  return true;
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
  for (int i = 0; i < NUM_TRAINS; ++i) {
    if (trains[i].id == trainId) {
      return &trains[i];
    }
  }
  return nullptr;
}

}  // namespace marklin