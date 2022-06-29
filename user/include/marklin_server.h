#ifndef USER_MARKLIN_SERVER_H_
#define USER_MARKLIN_SERVER_H_

#include "marklin/msg.h"

#define MARKLIN_SERVER_NAME "MARKLIN_SERVER"

namespace marklin {

#define MRV_CAP 10
#define SENSOR_CAP 80

const int REVERSE = 15;
const int LIGHT_ON = 16;
const int SWITCH_OFF = 32;
const int SWITCH_S = 33;
const int SWITCH_C = 34;
const int GO = 96;
const int STOP = 97;
const int SENSOR = 133;

struct MrvNode {
  int bitIdx;
  int time;
};

struct Sensor {
  int mrvIdx;
  int mrvSize;
  char data[SENSOR_CAP];
  char status[SENSOR_CAP];
  MrvNode mostRecentlyVisited[MRV_CAP];

  static void getSensorName(int sensorIdx, char &sensorGroup, int &sensorNum);
};

void cmdServer();

}  // namespace marklin

#endif  // USER_MARKLIN_SERVER_H_