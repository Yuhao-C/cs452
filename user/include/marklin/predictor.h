#ifndef USER_MARKLIN_PREDICTOR_H_
#define USER_MARKLIN_PREDICTOR_H_

#include "world.h"

#define PREDICTOR_NAME "PREDICTOR"

namespace marklin {

void runPredictor();

class Predictor {
  World world;
  track_node *nextSensor;
  int nextSensorTick;
  int displayServerTid;

 public:
  Predictor();
  void run();
  void onSensorTrigger(int sensorNum, int tick);
  void onSwitch(int direction, int switchId);
};

}  // namespace marklin
#endif  // USER_MARKLIN_PREDICTOR_H_