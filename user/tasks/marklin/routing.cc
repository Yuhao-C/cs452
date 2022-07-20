#include "marklin/routing.h"

#include "clock_server.h"
#include "lib/assert.h"
#include "lib/io.h"
#include "lib/log.h"
#include "lib/math.h"
#include "marklin/reservation.h"
#include "marklin/world.h"
#include "marklin_server.h"
#include "name_server.h"
#include "user/message.h"
#include "user/task.h"

#define BLOCK_DIST 10000000

namespace marklin {

void runRouting() {
  Routing routing;
  routing.run();
}

void awaitStop() {
  int senderTid;
  int data[5];
  receive(senderTid, data);
  reply(senderTid);

  int delay = data[0];
  int worldTid = data[1];
  int trainId = data[2];
  bool rerouteOnSlow = data[3];
  bool rerouteOnStop = data[4];

  clock::delay(delay);
  if (rerouteOnSlow) {
    log("[routing]: reroute before stop train %d", trainId);
    send(worldTid, marklin::Msg{marklin::Msg::Action::Reroute, {trainId}, 1});
  } else {
    send(worldTid, Msg::tr(16, trainId));
    if (rerouteOnStop) {
      clock::delay(400);
      log("[routing]: reroute after stop train %d", trainId);
      send(worldTid, Msg{Msg::Action::SetDestination, {trainId, -1}, 2});
    }
  }
}

Routing::Routing() : worldTid{0} {
  reservationServer = whoIs(RESERVATION_SERVER_NAME);
  registerAs(ROUTING_SERVER_NAME);
  receive(worldTid, trackSet);
  reply(worldTid);
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

void Routing::clearStatus() {
  for (int i = 0; i < trackSize; ++i) {
    track[i].status = 0;
  }
}

void Routing::updateTrainLoc(int trainId, track_node* dest, int offset) {
  send(worldTid, Msg{Msg::Action::Depart, {trainId, dest - track, offset}, 3});
}

void Routing::handleDeparture(int trainId, int speed, int delay,
                              bool rerouteOnSlow, bool rerouteOnStop) {
  send(worldTid, Msg::tr(speed, trainId));
  int tid = create(1, awaitStop);
  assert(tid >= 0);
  int data[5]{delay, worldTid, trainId, rerouteOnSlow, rerouteOnStop};
  send(tid, data);
}

void Routing::setTrainBlocked(int trainId, bool blocked) {
  log("[routing]: set train %d blocked", trainId);
  send(worldTid, Msg{Msg::Action::SetTrainBlocked, {trainId, blocked}, 2});
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
  Train::Direction trainDirection = (Train::Direction)data[11];

  track_node* path[TRACK_MAX];
  track_node* blockedSensor{nullptr};

  ResvRequest req;
  send(reservationServer, req, req);
  int dist = route(srcNode, destNode, blockedSensor, path, trainId, req);
  destNode = blockedSensor ? blockedSensor : destNode;
  destOffset = blockedSensor ? -trainDirection : destOffset;
  int realDist = dist - srcOffset + destOffset;

  if (blockedSensor) {
    log("[set dest]: reroute train %d, blocked sensor: %s", trainId,
        blockedSensor->name);
  } else {
    log("[set dest]: reroute train %d, no blocked sensor", trainId);
  }

  if (dist < 0 || realDist <= 0 || blockedSensor == srcNode) {
    // cannot get to destination
    setTrainBlocked(trainId, true);
  } else if (realDist <= accelDist + stopDist) {
    // stop before reaching max speed
    if (!blockedSensor) {
      // only go if not blocked
      reserveTrack(path, trainId, destOffset, req);
      switchTurnout(path);
      int accelTick = sqrt(
          2 * realDist * 100 /
          ((accelSlow + accelSlow * 100 / decelSlow * accelSlow / 100) / 100));
      handleDeparture(trainId, 26, accelTick, false, false);
      updateTrainLoc(trainId, destNode, destOffset);
      log("[set dest]: real dist: %d, accl: %d, decl: %d", realDist, accelSlow,
          decelSlow);
      log("[set dest]: tick: %d", accelTick);
    } else {
      setTrainBlocked(trainId, true);
    }
  } else {
    reserveTrack(path, trainId, destOffset, req);
    switchTurnout(path);
    // check if there is sensor in between
    int remainAccelDist = accelDist + srcOffset;
    int traveledDist = 0;
    track_node* stopSensor{nullptr};
    int stopDelayDist = -1;

    int i = 0;
    while (path[i + 1] != nullptr) {
      track_node* cur = path[i];
      track_node* next = path[i + 1];
      track_edge edge;
      if (cur->edge[0].dest == next) {
        edge = cur->edge[0];
      } else {
        assert(cur->edge[1].dest == next);
        edge = cur->edge[1];
      }

      if (remainAccelDist > 0) {
        remainAccelDist -= edge.dist;
      } else if (traveledDist <= dist + destOffset - stopDist) {
        // track B C13 broken
        if (cur->type == NODE_SENSOR &&
            !(trackSet == TrackSet::TrackB && cur->num == 44)) {
          stopSensor = cur;
          stopDelayDist = dist + destOffset - stopDist - traveledDist;
        }
      }
      traveledDist += edge.dist;
      ++i;
    }
    if (stopSensor == nullptr) {
      // no sensor in between
      int cruiseDelay = (realDist - accelDist - stopDist) * 100 / velocity;
      handleDeparture(trainId, 26, accelDelay + cruiseDelay,
                      blockedSensor != nullptr, false);
      updateTrainLoc(trainId, destNode, destOffset);
      log("[set dest]: total dist: %d, accelDist: %d, stopDist: %d, "
          "cruiseDist: %d",
          realDist, accelDist, stopDist, realDist - accelDist - stopDist);
    } else {
      // has sensor in between
      trainStopSensor[trainId][0] = stopSensor->num;
      trainStopSensor[trainId][1] = stopDelayDist * 100 / velocity;
      trainStopSensor[trainId][2] = blockedSensor != nullptr;
      send(worldTid, Msg::tr(26, trainId));
      updateTrainLoc(trainId, destNode, destOffset);
      log("[set dest]: %d, %d", stopSensor->num,
          stopDelayDist * 100 / velocity);
    }
  }
}

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

int Routing::calcDist(track_node* (&path)[TRACK_MAX]) {
  int newShortestDist = 0;
  for (int i = 0;; ++i) {
    track_node* cur = path[i];
    track_node* next = path[i + 1];
    if (!next) {
      break;
    }
    int edgeIdx = cur->edge[0].dest == next ? 0 : 1;
    newShortestDist += cur->edge[edgeIdx].dist;
  }
  return newShortestDist;
}

void Routing::switchTurnout(track_node* (&path)[TRACK_MAX]) {
  for (int i = 0;; ++i) {
    track_node* cur = path[i];
    track_node* next = path[i + 1];
    if (!next) {
      break;
    }
    int edgeIdx = cur->edge[0].dest == next ? 0 : 1;
    if (cur->type == NODE_BRANCH) {
      if (edgeIdx == 0) {
        send(worldTid, Msg::sw('S', cur->num));
      } else {
        send(worldTid, Msg::sw('C', cur->num));
      }
    }
  }
}

void Routing::reserveTrack(track_node* (&path)[TRACK_MAX], int trainId, int offset,
                           ResvRequest& req) {
  for (int i = 0;; ++i) {
    track_node* cur = path[i];
    track_node* next = path[i + 1];
    if (!next && offset <= 0) {
      break;
    }
    int edgeIdx = (!next || cur->edge[0].dest == next) ? 0 : 1;
    bool reserveRes = req.reserve(trainId, cur->enterSeg[edgeIdx]);
    assert(reserveRes);
    if (!next) {
      break;
    }
  }
  send(reservationServer, req);
}

int Routing::route(track_node* startNode, track_node* endNode,
                   track_node*& blockedSensor, track_node* (&path)[TRACK_MAX],
                   int trainId, ResvRequest& req) {
  if (startNode == endNode) {
    return 0;
  }
  if (!req.canReserve(trainId, startNode->enterSeg[0])) {
    if (startNode->num == 68) {
      log("train %d cannot reserve %d", trainId, startNode->enterSeg[0]);
      req.print();
    }
    return -1;
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
        continue;
      }
      PathInfo* targetPathInfo = pathInfos.get(targetNode->name);
      int curDist = curPathInfo->distance + edge.dist;
      if (!req.canReserve(trainId, curNode->enterSeg[i])) {
        // add large weight to reserved segment
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
  PathInfo* endPathInfo = pathInfos.get(endNode->name);
  track_node* parent = endPathInfo->parent;
  track_node* cur = endNode;
  blockedSensor = nullptr;
  int pathIdx = 0;
  path[pathIdx++] = endNode;
  while (parent && parent != startNode) {
    int edgeIdx = 0;
    if (parent->type == NODE_BRANCH) {
      edgeIdx = parent->edge[0].dest == cur ? 0 : 1;
    }
    if (parent->type == NODE_SENSOR &&
        !req.canReserve(trainId, parent->enterSeg[edgeIdx])) {
      blockedSensor = parent;
    }
    path[pathIdx++] = parent;
    cur = parent;
    PathInfo* pathInfo = pathInfos.get(cur->name);
    assert(pathInfo != nullptr);
    parent = pathInfo->parent;
  }
  path[pathIdx] = parent;
  path[pathIdx + 1] = nullptr;
  if (parent != startNode) {
    if (startNode->num == 68) {
      log("parent %s, startNode %s", parent->name, startNode->name);
    }
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
  return calcDist(path);
}

void Routing::handleReroute(int* data) {
  int trainId = data[0];
  int stopDist = data[5];
  int velocity = data[6];
  track_node* srcNode = &track[data[7]];
  int srcOffset = data[8];
  track_node* destNode = &track[data[9]];
  int destOffset = data[10];
  Train::Direction trainDirection = (Train::Direction)data[11];

  track_node* path[TRACK_MAX];
  track_node* blockedSensor{nullptr};

  ResvRequest req;
  send(reservationServer, req, req);
  int dist = route(srcNode, destNode, blockedSensor, path, trainId, req);
  destNode = blockedSensor ? blockedSensor : destNode;
  destOffset = blockedSensor ? -trainDirection : destOffset;
  int realDist = dist - srcOffset + destOffset;

  if (blockedSensor) {
    log("[reroute]: reroute train %d, blocked sensor: %s", trainId,
        blockedSensor->name);
  } else {
    log("[reroute]: reroute train %d, no blocked sensor", trainId);
  }
  if (dist < 0 || realDist <= 0 || blockedSensor == srcNode) {
    // stop as usual
    log("[reroute]: reroute train %d, dist: %d, realDist: %d, stop directly",
        trainId, dist, realDist);
    handleDeparture(trainId, 16, 0, false, true);
  } else {
    reserveTrack(path, trainId, destOffset, req);
    switchTurnout(path);

    // check if there is sensor in between
    int i = 0;
    int traveledDist = 0;
    track_node* stopSensor = nullptr;
    int stopDelayDist = 0;
    while (path[i + 1] != nullptr) {
      track_node* cur = path[i];
      track_node* next = path[i + 1];
      track_edge edge;
      if (cur->edge[0].dest == next) {
        edge = cur->edge[0];
      } else {
        assert(cur->edge[1].dest == next);
        edge = cur->edge[1];
      }
      if (traveledDist > dist + destOffset - stopDist) {
        break;
      }
      if (cur->type == NODE_SENSOR) {
        stopSensor = cur;
        stopDelayDist = dist + destOffset - stopDist - traveledDist;
      }
      traveledDist += edge.dist;
      ++i;
    }
    if (stopSensor == nullptr) {
      // no sensor in between
      int cruiseDelay = realDist * 100 / velocity;
      log("[reroute]: reroute train %d, no sensor in between, cruise Delay %d",
          trainId, cruiseDelay);
      handleDeparture(trainId, 26, cruiseDelay, blockedSensor != nullptr,
                      false);
      updateTrainLoc(trainId, destNode, destOffset);
    } else {
      // has sensor in between
      log("[reroute]: reroute train %d, stop sensor %s", trainId,
          stopSensor->name);
      trainStopSensor[trainId][0] = stopSensor->num;
      trainStopSensor[trainId][1] = stopDelayDist * 100 / velocity;
      trainStopSensor[trainId][2] = blockedSensor != nullptr;
      updateTrainLoc(trainId, destNode, destOffset);
    }
  }
}

void Routing::run() {
  marklin::Msg msg;
  int senderTid;
  while (true) {
    receive(senderTid, msg);
    reply(senderTid);
    switch (msg.action) {
      case marklin::Msg::Action::SetDestination:
        onDestinationSet(msg.data);
        break;
      case Msg::Action::Reroute: {
        log("[reroute]: received reroute train %d", msg.data[0]);
        handleReroute(msg.data);
        break;
      }
      case marklin::Msg::Action::SensorTriggered: {
        int sensorNum = msg.data[0];
        int trainId = msg.data[2];
        int awaitSensor = trainStopSensor[trainId][0];
        int stopDelay = trainStopSensor[trainId][1];
        bool reroute = trainStopSensor[trainId][2];
        if (awaitSensor == sensorNum) {
          int tid = create(1, awaitStop);
          assert(tid >= 0);
          int data[5]{stopDelay, worldTid, trainId, reroute, false};
          send(tid, data);
          trainStopSensor[trainId][0] = -1;
          trainStopSensor[trainId][1] = -1;
          trainStopSensor[trainId][2] = -1;
        }
        break;
      }
      default:
        break;
    }
  }
}
}  // namespace marklin