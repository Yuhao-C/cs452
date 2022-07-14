#ifndef USER_MARKLIN_TRAIN_H_
#define USER_MARKLIN_TRAIN_H_

#include "track_data.h"

namespace marklin {

const int TRAIN_LENGTH = 217000;

class Train {
 public:
  int id;

  enum SpeedLevel { SevenDec, TenInc, FourteenInc, Zero };
  enum Direction { Forward = 30000, Backward = 150000 };

  SpeedLevel speedLevel;
  Direction direction;

  int reverseTid;
  bool isReversing;

  track_node* nextSensor;  // predicted next sensor
  int nextSensorTick;      // predicted time of arrival at next sensor
  int nextSensorDist;      // distance to predicted next sensor
  track_node* lastSensor;  // actual last triggered sensor
  int lastSensorTick;      // actual trigger time of last sensor

  int locNodeIdx;
  int locOffset;

  bool isBlocked;

  int viaNodeIdx;
  int viaOffset;
  int destNodeIdx;
  int destOffset;

  int accelSlow;
  int decelSlow;
  int velocity;
  int stopDist;
  int accelDist;
  int accelDelay;

  Train();
  Train(int id, int* velocity, int* stopDist);
  void setSpeedLevel(SpeedLevel speedLevel);
  int getVelocity() const;
  int getStopDist() const;
  SpeedLevel getSpeedLevel() const;
  int getSpeedLevelInt() const;
  void reverseDirection();

  static SpeedLevel getSpeedLevel(int speed);
};
}  // namespace marklin

#endif  // USER_MARKLIN_TRAIN_H_