#ifndef USER_MARKLIN_WORLD_H_
#define USER_MARKLIN_WORLD_H_

#include "lib/hashtable.h"
#include "msg.h"
#include "track_data.h"
#include "train.h"

#define WORLD_NAME "WORLD"

namespace marklin {
enum class TrackSet { Unknown, TrackA = 'A', TrackB };

void runWorld();

class World {
  TrackSet trackSet;
  track_node track[TRACK_MAX];
  int trackSize;
  Train trains[6];

  int marklinServerTid;
  int routingServerTid;
  int displayServerTid;

  void onSwitch(const Msg &msg);
  void onSensorTrigger(const Msg &msg);
  void onSetDestination(const Msg &msg);
  int onSetTrainSpeed(int senderTid, const Msg &msg);
  int onReverseTrain(const Msg &msg);
  void onSetTrainLoc(const Msg &msg);

  Train *predictTrainBySensor(int sensorNum, int &offDist);

 public:
  World();
  void run();
  void init(TrackSet trackSet);
  track_node *getSensor(int sensorNum);
  track_node *getBranch(int switchNum);
  track_node *findNextSensor(track_node *sensor, int &distance);
  Train *getTrain(int trainId);

  int getTrackSize() const;
};
}  // namespace marklin

#endif  // USER_MARKLIN_WORLD_H_