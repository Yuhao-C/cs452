#include "marklin/reservation.h"

#include "lib/io.h"
#include "lib/log.h"
#include "name_server.h"
#include "user/message.h"
#include "user/task.h"

namespace marklin {

ResvRequest::ResvRequest()
    : action{Action::Query}, segments{0}, semaphores{0} {}

bool ResvRequest::canReserve(int trainId, int segId) const {
  if (segId == -1) {
    return true;
  }
  return segments[segId] == 0 || segments[segId] == trainId;
}

bool ResvRequest::isReservedBy(int trainId, int segId) const {
  if (segId == -1) {
    return false;
  }
  return segments[segId] == trainId;
}

bool ResvRequest::reserve(int trainId, int segId) {
  if (segId == -1) {
    return true;
  }
  if (canReserve(trainId, segId)) {
    segments[segId] = trainId;
    ++semaphores[segId];
    return true;
  }
  return false;
}

bool ResvRequest::free(int trainId, int segId) {
  if (segments[segId] == trainId) {
    --semaphores[segId];
    return true;
  }
  return false;
}

void ResvRequest::print() {
  for (int i = 0; i < NUM_SEGMENTS; ++i) {
    log("seg %d: train %d times %d", i, segments[i], semaphores[i]);
  }
}

ReservationServer::ReservationServer() : segments{0}, semaphores{0} {}

void ReservationServer::run() {
  registerAs(RESERVATION_SERVER_NAME);
  int senderTid;
  ResvRequest request;
  while (true) {
    receive(senderTid, request);
    // handles must reply to sender
    if (request.action == ResvRequest::Action::Query) {
      onReceiveQuery(senderTid, request);
    } else {
      onReceiveUpdate(senderTid, request);
    }
  }
}

void ReservationServer::onReceiveQuery(int senderTid, ResvRequest &request) {
  request.action = ResvRequest::Action::Update;
  for (int i = 0; i < NUM_SEGMENTS; ++i) {
    request.segments[i] = semaphores[i] > 0 ? segments[i] : 0;
  }
  reply(senderTid, request);
}

void ReservationServer::onReceiveUpdate(int senderTid, ResvRequest &request) {
  reply(senderTid);
  // request.print();
  for (int i = 0; i < NUM_SEGMENTS; ++i) {
    semaphores[i] += request.semaphores[i];
    if (request.semaphores[i] > 0) {
      segments[i] = request.segments[i];
    } else if (semaphores[i] == 0) {
      segments[i] = 0;
    }
  }
#if RESERVATION_VERBOSE
  log("Resv after update");
  for (int i = 0; i < NUM_SEGMENTS; ++i) {
    log("[Resv] seg %d: train %d times %d", i, segments[i], semaphores[i]);
  }
#endif
}

void ReservationServer::runServer() {
  ReservationServer server;
  server.run();
}

}  // namespace marklin