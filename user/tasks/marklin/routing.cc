#include "marklin/routing.h"

#include "clock_server.h"
#include "lib/assert.h"
#include "lib/io.h"
#include "lib/math.h"
#include "marklin/world.h"
#include "marklin_server.h"
#include "name_server.h"
#include "user/message.h"
#include "user/task.h"

namespace marklin {

void runRouting() {
  Routing routing;
  routing.run();
}

void awaitStop() {
  // println(COM2, "task created");
  int senderTid;
  int data[3];
  receive(senderTid, data);
  reply(senderTid);

  int delay = data[0];
  int worldTid = data[1];
  int trainId = data[2];

  clock::delay(delay);
  send(worldTid, Msg::tr(0, trainId));
  // println(COM2, "sent train stop");
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
  for (int i = 0; i < 80; ++i) {
    trainStopSensor[i][0] = -1;
    trainStopSensor[i][1] = -1;
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
  int trainId = data[0];
  int accelSlow = data[1];
  int accelDelay = data[2];
  int accelDist = data[3];
  int decelSlow = data[4];
  int stopDist = data[5];
  int velocity = data[6];
  track_node* srcNode = &track[data[7]];
  int srcOffset = data[8];
  track_node* destNode = &track[data[9]];
  int destOffset = data[10];

  track_node* path[TRACK_MAX];
  int dist = route(srcNode, destNode, path, false);
  int realDist = dist - srcOffset + destOffset;

  if (dist < 0) {
    // TODO: cannot get to destination
  } else if (realDist <= accelDist + stopDist) {
    // stop before reaching max speed
    assert(realDist >= 0);
    println(COM2, "real dist: %d, accl: %d, decl: %d", realDist, accelSlow,
            decelSlow);
    int acclTick = sqrt(
        2 * realDist * 100 /
        ((accelSlow + accelSlow * 100 / decelSlow * accelSlow / 100) / 100));
    send(worldTid, Msg::tr(26, trainId));
    int tid = create(1, awaitStop);
    println(COM2, "tick: %d", acclTick);
    int data[3]{acclTick, worldTid, trainId};
    send(tid, data);
  } else {
    assert(realDist >= 0);
    // check if there is sensor in between
    int i = 0;
    while (path[i] != nullptr) {
      ++i;
    }
    --i;
    int remainAcclDist = accelDist + srcOffset;
    int traveledDist = 0;
    track_node* stopSensor = nullptr;
    int stopDelayDist = -1;

    while (i > 0) {
      track_node* cur = path[i];
      track_node* next = path[i - 1];
      track_edge edge;
      if (cur->edge[0].dest == next) {
        edge = cur->edge[0];
      } else {
        assert(cur->edge[1].dest == next);
        edge = cur->edge[1];
      }

      if (remainAcclDist > 0) {
        remainAcclDist -= edge.dist;
      } else if (traveledDist <= dist + destOffset - stopDist) {
        if (cur->type == NODE_SENSOR) {
          stopSensor = cur;
          stopDelayDist = dist + destOffset - stopDist - traveledDist;
        }
      }
      traveledDist += edge.dist;
      --i;
    }
    if (stopSensor == nullptr) {
      // no sensor in between
      // int acclTick = sqrt(2 * realDist * 100 /
      //                     ((accl + accl * 100 / decl * accl / 100) / 100));
      int cruiseDelay = (realDist - accelDist - stopDist) * 100 / velocity;
      println(COM2,
              "total dist: %d, accelDist: %d, stopDist: %d, cruiseDist: %d",
              realDist, accelDist, stopDist, realDist - accelDist - stopDist);
      send(worldTid, Msg::tr(26, trainId));
      int tid = create(1, awaitStop);
      int data[3]{accelDelay + cruiseDelay, worldTid, trainId};
      send(tid, data);
    } else {
      // has sensor in between
      println(COM2, "%d, %d", stopSensor->num, stopDelayDist * 100 / velocity);
      trainStopSensor[trainId][0] = stopSensor->num;
      trainStopSensor[trainId][1] = stopDelayDist * 100 / velocity;
      send(worldTid, Msg::tr(26, trainId));
    }
  }
}

// void Routing::onDestinationSet(int* data) {
//   trainId = data[0];
//   destNode = &track[data[1]];
//   int destOffset = data[2];
//   int stopDist = data[3];
//   int velocity = data[4];
//   track_node* srcNode = &track[data[5]];
//   track_node* nextSensor = &track[data[6]];
//   int speedLevel = data[7];

//   int stopOffset = destOffset - stopDist;
//   track_node* curNode = destNode;
//   int stopSensorDelayDist = 0;
//   if (stopOffset > 0) {
//     int remainOffset = stopOffset;
//     while (true) {
//       if (remainOffset - curNode->edge[0].dist < 0) {
//         break;
//       }
//       remainOffset -= curNode->edge[0].dist;
//       curNode = curNode->edge[0].dest;
//     }
//     stopSensorDelayDist = remainOffset;
//   } else if (stopOffset < 0) {
//     int remainOffset = stopOffset;
//     while (true) {
//       int edgeIdx = 0;
//       if (curNode->type == node_type::NODE_ENTER) {
//         return;
//       }
//       if (curNode->type == node_type::NODE_MERGE) {
//         int edge0Dist =
//             route(srcNode, curNode->reverse->edge[0].dest->reverse, true);
//         int edge1Dist =
//             route(srcNode, curNode->reverse->edge[1].dest->reverse, true);
//         edgeIdx = edge0Dist < edge1Dist ? 0 : 1;
//       }
//       remainOffset += curNode->reverse->edge[edgeIdx].dist;
//       curNode = curNode->reverse->edge[edgeIdx].dest->reverse;
//       if (remainOffset >= 0) {
//         break;
//       }
//     }
//     stopSensorDelayDist = remainOffset;
//   }
//   // find last sensor
//   while (curNode->type != node_type::NODE_SENSOR) {
//     int edgeIdx = 0;
//     if (curNode->type == node_type::NODE_ENTER) {
//       return;
//     }
//     if (curNode->type == node_type::NODE_MERGE) {
//       int edge0Dist =
//           route(srcNode, curNode->reverse->edge[0].dest->reverse, true);
//       int edge1Dist =
//           route(srcNode, curNode->reverse->edge[1].dest->reverse, true);
//       edgeIdx = edge0Dist < edge1Dist ? 0 : 1;
//     }
//     stopSensorDelayDist += curNode->reverse->edge[edgeIdx].dist;
//     curNode = curNode->reverse->edge[edgeIdx].dest->reverse;
//   }
//   stopSensorDelay = stopSensorDelayDist * 100 / velocity;
//   stopSensor = curNode;

//   // find last sensor that is 120 cm away
//   int slowDownDist = 0;
//   do {
//     int edgeIdx = 0;
//     if (curNode->type == node_type::NODE_ENTER) {
//       return;
//     }
//     if (curNode->type == node_type::NODE_MERGE) {
//       int edge0Dist =
//           route(srcNode, curNode->reverse->edge[0].dest->reverse, true);
//       int edge1Dist =
//           route(srcNode, curNode->reverse->edge[1].dest->reverse, true);
//       edgeIdx = edge0Dist < edge1Dist ? 0 : 1;
//     }
//     slowDownDist += curNode->reverse->edge[edgeIdx].dist;
//     curNode = curNode->reverse->edge[edgeIdx].dest->reverse;
//   } while (curNode->type != node_type::NODE_SENSOR || slowDownDist <
//   1200000); slowDownSensor = curNode;

//   if (srcNode == slowDownSensor || nextSensor == slowDownSensor) {
//     int dist = route(srcNode, stopSensor, false);
//     if (dist < __INT_MAX__) {
//       send(worldTid, Msg::tr(speedLevel, trainId));
//       send(worldTid, Msg::tr(23, trainId));
//       routeStatus = 2;
//     }
//   } else {
//     int dist = route(srcNode, slowDownSensor, false);
//     if (dist < __INT_MAX__) {
//       send(worldTid, Msg::tr(speedLevel, trainId));
//       routeStatus = 1;
//     }
//   }
// }

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

int Routing::route(track_node* startNode, track_node* endNode,
                   track_node* (&path)[TRACK_MAX], bool mock) {
  if (startNode == endNode) {
    return 0;
  }
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
  int pathIdx = 0;
  path[pathIdx++] = endNode;
  while (parent && parent != startNode) {
    path[pathIdx++] = parent;
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
    assert(pathInfo != nullptr);
    parent = pathInfo->parent;
  }
  path[pathIdx++] = parent;
  path[pathIdx++] = nullptr;
  return parent == startNode ? shortestDist : -1;
}

void Routing::run() {
  marklin::Msg msg;
  while (true) {
    receive(worldTid, msg);
    reply(worldTid);
    switch (msg.action) {
      case marklin::Msg::Action::SetDestination:
        onDestinationSet(msg.data);
        break;
      case marklin::Msg::Action::SensorTriggered: {
        int sensorNum = msg.data[0];
        int trainId = msg.data[2];
        int nextSensorNum = msg.data[3];
        if (trainStopSensor[trainId][0] == sensorNum) {
          if (trainStopSensor[trainId][1] == 0) {
            send(worldTid, Msg::tr(0, trainId));
          } else {
            assert(trainStopSensor[trainId][1] > 0);
            int tid = create(1, awaitStop);
            int data[3]{trainStopSensor[trainId][1], worldTid, trainId};
            send(tid, data);
          }
          trainStopSensor[trainId][0] = -1;
          trainStopSensor[trainId][1] = -1;
        }
        // if (routeStatus == 1) {
        //   if (nextSensorNum == slowDownSensor->num) {
        //     route(slowDownSensor, stopSensor, false);
        //   } else if (sensorNum == slowDownSensor->num) {
        //     send(worldTid, Msg::tr(23, trainId));
        //     ++routeStatus;
        //   }
        // } else if (routeStatus == 2) {
        //   if (sensorNum == stopSensor->num) {
        //     clock::delay(stopSensorDelay);
        //     send(worldTid, Msg::tr(0, trainId));
        //     routeStatus = 0;
        //   }
        // }
        break;
      }
      default:
        break;
    }
  }
}
}  // namespace marklin