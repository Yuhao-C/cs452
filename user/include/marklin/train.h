#ifndef USER_MARKLIN_TRAIN_H_
#define USER_MARKLIN_TRAIN_H_

#include "track_data.h"

#define SPEED_MASK 15
#define LIGHT_MASK 16

namespace marklin {

const int NUM_TRAINS = 6;

const int TRAIN_IDS[NUM_TRAINS] = {1, 24, 58, 74, 78, 2};

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

  bool hasDest() const;
  bool isRouteDirect() const;
  void setLoc(int nodeIdx, int offset);
  void setDest(int nodeIdx, int offset);
  void setVia(int nodeIdx, int offset);

  static SpeedLevel getSpeedLevel(int speed);
};
}  // namespace marklin

#endif  // USER_MARKLIN_TRAIN_H_