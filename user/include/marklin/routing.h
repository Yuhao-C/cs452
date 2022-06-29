#ifndef USER_MARKLIN_ROUTING_H_
#define USER_MARKLIN_ROUTING_H_

#include "lib/string.h"
#include "track_data.h"
#include "world.h"

#define ROUTING_SERVER_NAME "ROUTING_SERVER"

namespace marklin {

void runRouting();

class Routing {
  int trainId;
  int trackSize;
  int worldTid;
  int routeStatus;
  int stopSensorDelay;
  track_node* destNode;
  track_node* slowDownSensor;
  track_node* stopSensor;
  track_node track[TRACK_MAX];
  TrackSet trackSet;

  void clearStatus();
  int route(track_node* src, track_node* dest, bool mock);
  // bool canGo(track_node* src, track_node* dst);
  void onDestinationSet(int* data);

 public:
  Routing();
  void run();
};

struct TrackNode {
  int nodeIdx;
  int distance;

  TrackNode(int nodeIdx, int distance) : nodeIdx{nodeIdx}, distance{distance} {}

  friend bool operator<(const TrackNode& l, const TrackNode& r) {
    return l.distance < r.distance;
  }
};

}  // namespace marklin
#endif  // USER_MARKLIN_ROUTING_H_