#include "marklin/train.h"

#include "user/message.h"
#include "user/task.h"

namespace marklin {

Train::Train()
    : id{-1},
      speedLevel{SpeedLevel::Zero},
      direction{Direction::Forward},
      reverseTid{-1},
      isReversing{false},
      nextSensor{nullptr},
      nextSensorTick{0},
      nextSensorDist{0},
      lastSensor{nullptr},
      lastSensorTick{0} {}

void Train::setSpeedLevel(SpeedLevel speedLevel) {
  this->speedLevel = speedLevel;
}

int Train::getVelocity() const { return velocity; }

int Train::getStopDist() const { return stopDist; }

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