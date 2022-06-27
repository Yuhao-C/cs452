#ifndef USER_MARKLIN_WORLD_H_
#define USER_MARKLIN_WORLD_H_

#include "track_data.h"


namespace marklin {
enum class TrackSet { Unknown, TrackA = 'A', TrackB };

class World {
  TrackSet trackSet;
  track_node track[TRACK_MAX];
  int trackSize;

 public:
  World();
  void init(TrackSet trackSet);
  track_node *getSensor(int sensorNum);
  track_node *getBranch(int switchNum);
  track_node *findNextSensor(int sensorNum, int &distance);

  int getTrackSize() const;
};
}  // namespace marklin

#endif  // USER_MARKLIN_WORLD_H_