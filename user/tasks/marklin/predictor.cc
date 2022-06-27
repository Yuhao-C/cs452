#include "marklin/predictor.h"

#include "display_server.h"
#include "marklin/world.h"
#include "marklin_server.h"
#include "name_server.h"
#include "user/message.h"

namespace marklin {

void runPredictor() {
  Predictor predictor;
  predictor.run();
}

Predictor::Predictor() : nextSensor{nullptr}, nextSensorTick{0}, displayServerTid{-1} {}

void Predictor::run() {
  registerAs(PREDICTOR_NAME);

  int senderTid;
  Msg msg;
  while (true) {
    receive(senderTid, msg);
    if (world.getTrackSize() == 0 && msg.action != Msg::Action::InitTrack) {
      // track not initialized, ignore msg
      reply(senderTid, -1);
      continue;
    }
    switch (msg.action) {
      case Msg::Action::InitTrack:
        if (world.getTrackSize() == 0) {
          world.init((TrackSet)msg.data[0]);
        }
        reply(senderTid, 0);
        break;
      case Msg::Action::SensorTriggered:
        reply(senderTid, 0);
        onSensorTrigger(msg.data[0], msg.data[1]);
        break;
      case Msg::Action::SwitchCmd:
        reply(senderTid, 0);
        onSwitch(msg.data[0], msg.data[1]);
        break;
      default:
        reply(senderTid, -1);
    }
  }
}

void Predictor::onSensorTrigger(int sensorNum, int tick) {
  int velocity = 1;  // get train speed in some way

  int timeDiff{0}, distDiff{0};
  bool isLastPredictValid = true;
  if (nextSensor->type == NODE_SENSOR && sensorNum == nextSensor->num) {
    timeDiff = tick - nextSensorTick;
    distDiff = velocity * timeDiff;
  } else {
    isLastPredictValid = false;
    // did not follow predicted path
  }

  int distance;
  nextSensor = world.findNextSensor(sensorNum, distance);

  nextSensorTick = tick + distance / velocity;
  // send to display server
  while (displayServerTid < 0) {
    displayServerTid = whoIs(DISPLAY_SERVER_NAME);
  }
  send(displayServerTid, view::Msg{view::Action::Predict,
                                   {nextSensor->num, nextSensorTick,
                                    isLastPredictValid, timeDiff, distDiff},
                                   5});
  if (nextSensor->type == NODE_EXIT) {
    // may need to do something?
  }
}

void Predictor::onSwitch(int direction, int switchId) {
  world.getBranch(switchId)->status =
      direction == SWITCH_S ? DIR_STRAIGHT : DIR_CURVED;
}

}  // namespace marklin