#ifndef CALIBRATION_TRAIN_DATA_H_
#define CALIBRATION_TRAIN_DATA_H_

#include "marklin/train.h"

void initTrains(marklin::Train *trains) {
  trains[0].id = 1;
  trains[0].velocity = 329000;
  trains[0].accelSlow = 86500;
  trains[0].decelSlow = 54562;
  trains[0].accelDist = 351750;
  trains[0].accelDelay = 270;
  trains[0].stopDist = 450000;

  trains[1].id = 24;
  trains[1].velocity = 342000;
  trains[1].accelSlow = 84500;
  trains[1].decelSlow = 46797;
  trains[1].accelDist = 373529;
  trains[1].accelDelay = 273;
  trains[1].stopDist = 440000;

  trains[2].id = 58;
  trains[2].velocity = 305000;
  trains[2].accelSlow = 80644;
  trains[2].decelSlow = 43000;
  trains[2].accelDist = 497461;
  trains[2].accelDelay = 340;
  trains[2].stopDist = 455000;

  trains[3].id = 74;
  trains[3].velocity = 478000;
  trains[3].accelSlow = 60700;
  trains[3].decelSlow = 46698;
  trains[3].accelDist = 567938;
  trains[3].accelDelay = 369;
  trains[3].stopDist = 610000;

  trains[4].id = 78;
  trains[4].velocity = 267500;
  trains[4].accelSlow = 44553;
  trains[4].decelSlow = 45463;
  trains[4].accelDist = 462766;
  trains[4].accelDelay = 427;
  trains[4].stopDist = 330000;

  trains[5].id = 79;
  trains[5].velocity = 372000;
  trains[5].accelSlow = 93000;
  trains[5].decelSlow = 45000;
  trains[5].accelDist = 495901;
  trains[5].accelDelay = 330;
  trains[5].stopDist = 490000;

}

#endif  // CALIBRATION_TRAIN_DATA_H_
