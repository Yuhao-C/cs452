#ifndef USER_MARKLIN_ROUTING_H_
#define USER_MARKLIN_ROUTING_H_

#include "lib/string.h"
#include "reservation.h"
#include "track_data.h"
#include "world.h"

#define ROUTING_SERVER_NAME "ROUTING_SERVER"

namespace marklin {

struct PathInfo {
  int idx;
  int distance;
  track_node* parent;
};

void runRouting();

class Routing {
  int trackSize;
  int worldTid;
  int reservationServer;
  track_node track[TRACK_MAX];
  TrackSet trackSet;

  int trainStopSensor[80][3];

  void clearStatus();
  int route(track_node* src, track_node* dest, int destOffset,
            track_node*& blockedSensor, track_node* (&path)[TRACK_MAX],
            int trainId, ResvRequest& req);
  int route(track_node* src, track_node* dest, int destOffset,
            track_node*& blockedSensor, track_node* (&path)[TRACK_MAX],
            int& totalDist, int trainId, ResvRequest& req);
  int routeRv(track_node* startNode, track_node* endNode, int destOffset,
              track_node*& blockedSensor, track_node* (&path)[TRACK_MAX],
              bool& shouldReverse, int trainId, ResvRequest& req);
  void onDestinationSet(int* data);
  void reserveTrack(track_node* (&path)[TRACK_MAX], int trainId, int offset,
                    ResvRequest& req);
  void switchTurnout(track_node* (&path)[TRACK_MAX]);
  void handleReroute(int* data);
  void updateTrainLoc(int trainId, track_node* dest, int offset);
  void handleDeparture(int trainId, int speed, int delay, bool rerouteOnSlow,
                       bool rerouteOnStop);
  void setTrainBlocked(int trainId, bool blocked);
  int calcDist(track_node* (&path)[TRACK_MAX]);

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