#include "marklin/train.h"

#include "user/message.h"
#include "user/task.h"

namespace marklin {

Train::Train(int id, int* velocity, int* stopDist)
    : id{id},
      speedLevel{SpeedLevel::Zero},
      direction{Direction::Forward},
      velocity{velocity[0], velocity[1], velocity[2]},
      stopDist{stopDist[0], stopDist[1], stopDist[2]},
      isReversing{false},
      nextSensor{nullptr},
      nextSensorTick{0},
      nextSensorDist{0},
      lastSensor{nullptr},
      lastSensorTick{0} {}

void Train::setSpeedLevel(SpeedLevel speedLevel) {
  this->speedLevel = speedLevel;
}

int Train::getVelocity() const { return velocity[speedLevel]; }

int Train::getStopDist() const { return stopDist[speedLevel]; }

Train::SpeedLevel Train::getSpeedLevel() const { return speedLevel; };

int Train::getSpeedLevelInt() const {
  int speeds[4] = {7, 10, 14, 0};
  return speeds[speedLevel];
}

Train::SpeedLevel Train::getSpeedLevel(int speed) {
  switch (speed & 15) {
    case 7:
      return SpeedLevel::SevenDec;
    case 10:
      return SpeedLevel::TenInc;
    case 14:
      return SpeedLevel::FourteenInc;
    default:
      return SpeedLevel::Zero;
  }
}

void Train::reverseDirection() {
  direction = direction == Direction::Forward ? Direction::Backward
                                              : Direction::Forward;
}

}  // namespace marklin