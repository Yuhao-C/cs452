#include "marklin/world.h"
#include "marklin_server.h"
#include "name_server.h"
#include "user/message.h"

namespace marklin {

World::World() : trackSet{TrackSet::Unknown}, trackSize{0} {}

void World::init(TrackSet trackSet) {
  this->trackSet = trackSet;
  if (trackSet == TrackSet::TrackA) {
    trackSize = init_tracka(track);
  } else {
    trackSize = init_trackb(track);
  }
  char switchInit[] = "SSSSCSSSSSCSSSSSSCSCSC";
  int marklinServerTid = whoIs(MARKLIN_SERVER_NAME);
  for (int i = 0; i < 22; ++i) {
    track[80 + 2 * i].status = switchInit[i] == 'S' ? DIR_STRAIGHT : DIR_CURVED;
    send(marklinServerTid, marklin::Msg::sw(switchInit[i], track[80 + 2 * i].num));
  }
}

track_node *World::getSensor(int sensorNum) { return &track[sensorNum]; }

track_node *World::getBranch(int switchNum) {
  int index;
  if (switchNum >= 153) {
    index = 80 + 2 * (switchNum - 153 + 18);
  } else {
    index = 80 + 2 * (switchNum - 1);
  }
  return &track[index];
}

track_node *World::findNextSensor(int sensorNum, int &distance) {
  track_node *sensor = getSensor(sensorNum);
  track_node *next = sensor->edge[0].dest;
  distance = sensor->edge[0].dist;
  while (next->type != NODE_SENSOR && next->type != NODE_EXIT) {
    if (next->type == NODE_BRANCH) {
      distance += next->edge[next->status].dist;
      next = next->edge[next->status].dest;
    } else {
      distance += next->edge[DIR_AHEAD].dist;
      next = next->edge[DIR_AHEAD].dest;
    }
  }
  return next;
}

int World::getTrackSize() const { return trackSize; }

}  // namespace marklin