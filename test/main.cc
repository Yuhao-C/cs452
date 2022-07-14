#include <memory.h>

#include <cassert>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

#include "track_data.h"
#include "train_data.h"

#define TRACK_SIZE 144
#define BLOCK_DIST 10000000

track_node track[TRACK_SIZE];
marklin::Train trains[6];
int trackSize;

struct PathInfo {
  int idx;
  int distance;
  track_node* parent;
};

void clearStatus() {
  for (int i = 0; i < trackSize; ++i) {
    track[i].status = 0;
  }
}

bool isReserved(int segment) {
  std::unordered_set<int> reserved{29};
  if (reserved.count(segment) > 0) {
    return true;
  }
  return false;
}

int switchTurnout(track_node* (&path)[TRACK_MAX]) {
  int newShortestDist = 0;
  for (int i = 0;; ++i) {
    track_node* cur = path[i];
    track_node* next = path[i + 1];
    if (!next) {
      break;
    }
    int edgeIdx = cur->edge[0].dest == next ? 0 : 1;
    newShortestDist += cur->edge[edgeIdx].dist;
    if (cur->type == NODE_BRANCH) {
      if (edgeIdx == 0) {
        std::cout << cur->name << " S" << std::endl;
      } else {
        std::cout << cur->name << " C" << std::endl;
      }
    }
  }
  return newShortestDist;
}

int route(track_node* startNode, track_node* endNode,
          track_node*& blockedSensor, track_node* (&path)[TRACK_MAX]) {
  if (startNode == endNode) {
    return 0;
  }
  if (isReserved(startNode->enterSeg[0])) {
    return -1;
  }
  clearStatus();
  std::unordered_map<std::string, PathInfo> pathInfos;
  for (int i = 0; i < trackSize; ++i) {
    pathInfos[track[i].name] =
        PathInfo{i, &track[i] == startNode ? 0 : __INT_MAX__, nullptr};
  }

  while (true) {
    int minDist = __INT_MAX__;
    int minIdx = -1;
    PathInfo* minNode = nullptr;
    for (int i = 0; i < trackSize; ++i) {
      PathInfo* node = &pathInfos[track[i].name];
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
        continue;
      }
      PathInfo* targetPathInfo = &pathInfos[targetNode->name];
      int curDist = curPathInfo->distance + edge.dist;
      if (isReserved(curNode->enterSeg[i])) {
        curDist += BLOCK_DIST;
      }
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
  PathInfo* endPathInfo = &pathInfos[endNode->name];
  track_node* parent = endPathInfo->parent;
  track_node* cur = endNode;
  int pathIdx = 0;
  path[pathIdx++] = endNode;
  while (parent && parent != startNode) {
    int edgeIdx = 0;
    if (parent->type == NODE_BRANCH) {
      edgeIdx = parent->edge[0].dest == cur ? 0 : 1;
    }
    if (isReserved(parent->enterSeg[edgeIdx])) {
      blockedSensor = parent;
    }
    path[pathIdx++] = parent;
    cur = parent;
    PathInfo* pathInfo = &pathInfos[cur->name];
    assert(pathInfo != nullptr);
    parent = pathInfo->parent;
  }
  path[pathIdx] = parent;
  path[pathIdx + 1] = nullptr;
  if (parent != startNode) {
    return -1;
  }

  // reverse path
  int l = 0;
  int r = pathIdx;
  while (l < r) {
    auto temp = path[l];
    path[l] = path[r];
    path[r] = temp;
    ++l;
    --r;
  }

  // remove path after blockedSensor
  int lastSensorIdx{-1};
  for (int i = 0; i <= pathIdx; ++i) {
    if (path[i]->type == NODE_SENSOR) {
      lastSensorIdx = i;
    }
    if (path[i] == blockedSensor) {
      blockedSensor = path[lastSensorIdx];
      path[lastSensorIdx + 1] = nullptr;
      break;
    }
  }
  // reserveTrack(path);
  return switchTurnout(path);
}

int sqrt(int x) {
  if (x < 2) {
    return x;
  }

  int left = sqrt(x >> 2) << 1;
  int right = left + 1;

  return right * right > x ? left : right;
}

void onDestinationSet(marklin::Train& t, track_node* srcNode,
                      track_node* destNode, int srcOffset, int destOffset) {
  int accelSlow = t.accelSlow;
  int accelDelay = t.accelDelay;
  int accelDist = t.accelDist;
  int decelSlow = t.decelSlow;
  int stopDist = t.stopDist;
  int velocity = t.velocity;
  int trainDirection = -30000;

  track_node* path[TRACK_MAX];
  track_node* blockedSensor{nullptr};
  int dist = route(srcNode, destNode, blockedSensor, path);
  destNode = blockedSensor ? blockedSensor : destNode;
  std::cout << "dest: " << destNode->name << std::endl;
  destOffset = blockedSensor ? trainDirection : destOffset;
  int realDist = dist - srcOffset + destOffset;

  if (dist < 0 || realDist <= 0 || blockedSensor == srcNode) {
    // TODO: cannot get to destination
  } else if (realDist <= accelDist + stopDist) {
    int accelTick = sqrt(
        2 * realDist * 100 /
        ((accelSlow + accelSlow * 100 / decelSlow * accelSlow / 100) / 100));
    std::cout << "short dist accel tick: " << accelTick << std::endl;
  } else {
    // check if there is sensor in between
    int remainAccelDist = accelDist + srcOffset;
    int traveledDist = 0;
    track_node* stopSensor = nullptr;
    int stopDelayDist = -1;
    int i = 0;
    while (path[i + 1] != nullptr) {
      track_node* cur = path[i];
      track_node* next = path[i - 1];
      track_edge edge;
      if (cur->edge[0].dest == next) {
        edge = cur->edge[0];
      } else {
        edge = cur->edge[1];
      }

      if (remainAccelDist > 0) {
        remainAccelDist -= edge.dist;
        traveledDist += edge.dist;
        --i;
        continue;
      }
      if (traveledDist <= dist + destOffset - stopDist) {
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
      int cruiseDelay = (realDist - accelDist - stopDist) * 100 / velocity;
      std::cout << "medium dist cruise delay: " << cruiseDelay << std::endl;
    } else {
      // has sensor in between
      std::cout << stopSensor->num << ", " << stopDelayDist * 100 / velocity
                << std::endl;
    }
  }
}

int main() {
  trackSize = init_trackb(track);
  initTrains(trains);
  onDestinationSet(trains[0], &track[68], &track[53], 0, 0);
}
