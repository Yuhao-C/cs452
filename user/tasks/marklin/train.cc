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
      lastSensorTick{0},
      locNodeIdx{-1},
      locOffset{0},
      isBlocked{false},
      viaNodeIdx{-1},
      viaOffset{0},
      destNodeIdx{-1},
      destOffset{0} {}

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

void Train::reverseDirection(track_node *track) {
  direction = direction == Direction::Forward ? Direction::Backward
                                              : Direction::Forward;
  int rvNodeIdx, rvOffset;
  getReversedLoc(track, locNodeIdx, locOffset, rvNodeIdx, rvOffset);
  setLoc(rvNodeIdx, rvOffset);
}

bool Train::hasDest() const { return destNodeIdx >= 0; }

bool Train::isRouteDirect() const {
  return destNodeIdx == viaNodeIdx && destOffset == viaOffset;
}

void Train::setLoc(int nodeIdx, int offset) {
  locNodeIdx = nodeIdx;
  locOffset = offset;
}

void Train::setDest(int nodeIdx, int offset) {
  destNodeIdx = nodeIdx;
  destOffset = offset;
}

void Train::setVia(int nodeIdx, int offset) {
  viaNodeIdx = nodeIdx;
  viaOffset = offset;
}

void Train::getReversedLoc(const track_node *track, int locNodeIdx,
                           int locOffset, int &rvNodeIdx, int &rvOffset) {
  rvNodeIdx = track[locNodeIdx].reverse - track;
  rvOffset = -(locOffset - TRAIN_LENGTH);
}

}  // namespace marklin