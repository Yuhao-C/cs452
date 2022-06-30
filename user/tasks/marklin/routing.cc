#include "marklin/routing.h"

#include "clock_server.h"
#include "marklin/world.h"
#include "marklin_server.h"
#include "name_server.h"
#include "user/message.h"

namespace marklin {

void runRouting() {
  Routing routing;
  routing.run();
}

Routing::Routing()
    : trainId{0},
      worldTid{0},
      routeStatus{0},
      stopSensorDelay{0},
      destNode{nullptr},
      slowDownSensor{nullptr},
      stopSensor{nullptr} {
  int senderTid;
  registerAs(ROUTING_SERVER_NAME);
  receive(senderTid, trackSet);
  reply(senderTid);
  if (trackSet == TrackSet::TrackA) {
    trackSize = init_tracka(track);
  } else {
    trackSize = init_trackb(track);
  }
}

// bool Routing::canGo(track_node* src, track_node* dst) {
//   if (src->status == 1) {
//     return false;
//   }
//   src->status = 1;
//   if (src == dst) {
//     return true;
//   }
//   if (src->type == node_type::NODE_EXIT) {
//     return false;
//   }
//   int edgeSize = src->type == node_type::NODE_BRANCH ? 2 : 1;
//   for (int i = 0; i < edgeSize; ++i) {
//     if (canGo(src->edge[i].dest, dst)) {
//       return true;
//     }
//   }
//   return false;
// }

void Routing::clearStatus() {
  for (int i = 0; i < trackSize; ++i) {
    track[i].status = 0;
  }
}

void Routing::onDestinationSet(int* data) {
  trainId = data[0];
  destNode = &track[data[1]];
  int destOffset = data[2];
  int stopDist = data[3];
  int velocity = data[4];
  track_node* srcNode = &track[data[5]];
  track_node* nextSensor = &track[data[6]];
  int speedLevel = data[7];

  int stopOffset = destOffset - stopDist;
  track_node* curNode = destNode;
  int stopSensorDelayDist = 0;
  if (stopOffset > 0) {
    int remainOffset = stopOffset;
    while (true) {
      if (remainOffset - curNode->edge[0].dist < 0) {
        break;
      }
      remainOffset -= curNode->edge[0].dist;
      curNode = curNode->edge[0].dest;
    }
    stopSensorDelayDist = remainOffset;
  } else if (stopOffset < 0) {
    int remainOffset = stopOffset;
    while (true) {
      int edgeIdx = 0;
      if (curNode->type == node_type::NODE_ENTER) {
        return;
      }
      if (curNode->type == node_type::NODE_MERGE) {
        int edge0Dist =
            route(srcNode, curNode->reverse->edge[0].dest->reverse, true);
        int edge1Dist =
            route(srcNode, curNode->reverse->edge[1].dest->reverse, true);
        edgeIdx = edge0Dist < edge1Dist ? 0 : 1;
      }
      remainOffset += curNode->reverse->edge[edgeIdx].dist;
      curNode = curNode->reverse->edge[edgeIdx].dest->reverse;
      if (remainOffset >= 0) {
        break;
      }
    }
    stopSensorDelayDist = remainOffset;
  }
  // find last sensor
  while (curNode->type != node_type::NODE_SENSOR) {
    int edgeIdx = 0;
    if (curNode->type == node_type::NODE_ENTER) {
      return;
    }
    if (curNode->type == node_type::NODE_MERGE) {
      int edge0Dist =
          route(srcNode, curNode->reverse->edge[0].dest->reverse, true);
      int edge1Dist =
          route(srcNode, curNode->reverse->edge[1].dest->reverse, true);
      edgeIdx = edge0Dist < edge1Dist ? 0 : 1;
    }
    stopSensorDelayDist += curNode->reverse->edge[edgeIdx].dist;
    curNode = curNode->reverse->edge[edgeIdx].dest->reverse;
  }
  stopSensorDelay = stopSensorDelayDist * 100 / velocity;
  stopSensor = curNode;

  // find last sensor that is 120 cm away
  int slowDownDist = 0;
  do {
    int edgeIdx = 0;
    if (curNode->type == node_type::NODE_ENTER) {
      return;
    }
    if (curNode->type == node_type::NODE_MERGE) {
      int edge0Dist =
          route(srcNode, curNode->reverse->edge[0].dest->reverse, true);
      int edge1Dist =
          route(srcNode, curNode->reverse->edge[1].dest->reverse, true);
      edgeIdx = edge0Dist < edge1Dist ? 0 : 1;
    }
    slowDownDist += curNode->reverse->edge[edgeIdx].dist;
    curNode = curNode->reverse->edge[edgeIdx].dest->reverse;
  } while (curNode->type != node_type::NODE_SENSOR || slowDownDist < 1200000);
  slowDownSensor = curNode;

  if (srcNode == slowDownSensor || nextSensor == slowDownSensor) {
    int dist = route(srcNode, destNode, false);
    if (dist < __INT_MAX__) {
      send(worldTid, Msg::tr(speedLevel, trainId));
      send(worldTid, Msg::tr(23, trainId));
      routeStatus = 2;
    }
  } else {
    int dist = route(srcNode, slowDownSensor, false);
    if (dist < __INT_MAX__) {
      send(worldTid, Msg::tr(speedLevel, trainId));
      routeStatus = 1;
    }
  }
}

struct PathInfo {
  int idx;
  int distance;
  track_node* parent;
};

int findMin(PathInfo* pathInfos, int size, track_node* track) {
  int min = __INT_MAX__;
  int minIdx = -1;
  for (int i = 0; i < size; ++i) {
    if (pathInfos->distance < min && track[pathInfos->idx].status == 0) {
      min = pathInfos->distance;
      minIdx = pathInfos->idx;
    }
  }
  return minIdx;
}

int Routing::route(track_node* startNode, track_node* endNode, bool mock) {
  clearStatus();
  HashTable<String, PathInfo, TRACK_MAX, 100> pathInfos;
  for (int i = 0; i < trackSize; ++i) {
    pathInfos.put(
        track[i].name,
        PathInfo{i, &track[i] == startNode ? 0 : __INT_MAX__, nullptr});
  }

  while (true) {
    int minDist = __INT_MAX__;
    int minIdx = -1;
    PathInfo* minNode = nullptr;
    for (int i = 0; i < pathInfos.size; ++i) {
      PathInfo* node = &pathInfos.nodes[i].value;
      if (node->distance < minDist && track[node->idx].status == 0) {
        minDist = node->distance;
        minIdx = node->idx;
        minNode = node;
      }
    }
    if (minIdx < 0) {
      break;
    }
    track_node* curNode = &track[minIdx];
    PathInfo* curPathInfo = minNode;
    int edgeSize = curNode->type == node_type::NODE_BRANCH ? 2 : 1;
    for (int i = 0; i < edgeSize; ++i) {
      track_edge edge = curNode->edge[i];
      track_node* targetNode = edge.dest;
      if (!targetNode) {
        break;
      }
      PathInfo* targetPathInfo = pathInfos.get(targetNode->name);
      int curDist = curPathInfo->distance + edge.dist;
      if (curDist < targetPathInfo->distance) {
        targetPathInfo->distance = curDist;
        targetPathInfo->parent = curNode;
      }
    }
    curNode->status = 1;  // mark as visited
    if (curNode == endNode) {
      break;
    }
  }
  PathInfo* endPathInfo = pathInfos.get(endNode->name);
  track_node* parent = endPathInfo->parent;
  track_node* cur = endNode;
  int shortestDist = endPathInfo->distance;
  while (parent && parent != startNode) {
    if (!mock) {
      if (parent->type == node_type::NODE_BRANCH) {
        if (parent->edge[DIR_STRAIGHT].dest == cur) {
          send(worldTid, Msg::sw('S', parent->num));
        } else {
          send(worldTid, Msg::sw('C', parent->num));
        }
      }
    }
    cur = parent;
    PathInfo* pathInfo = pathInfos.get(cur->name);
    shortestDist += pathInfo->distance;
    parent = pathInfo->parent;
  }
  return parent == startNode ? shortestDist : __INT_MAX__;
}

void Routing::run() {
  marklin::Msg msg;
  while (true) {
    receive(worldTid, msg);
    reply(worldTid);
    switch (msg.action) {
      case marklin::Msg::Action::SetDestination:
        if (routeStatus == 0) {
          onDestinationSet(msg.data);
        }
        break;
      case marklin::Msg::Action::SensorTriggered: {
        int sensorNum = msg.data[0];
        int nextSensorNum = msg.data[3];
        if (routeStatus == 1) {
          if (nextSensorNum == slowDownSensor->num) {
            route(slowDownSensor, destNode, false);
          } else if (sensorNum == slowDownSensor->num) {
            send(worldTid, Msg::tr(23, trainId));
            ++routeStatus;
          }
        } else if (routeStatus == 2) {
          if (sensorNum == stopSensor->num) {
            clock::delay(stopSensorDelay);
            send(worldTid, Msg::tr(0, trainId));
            routeStatus = 0;
          }
        }
        break;
      }
      default:
        break;
    }
  }
}
}  // namespace marklin