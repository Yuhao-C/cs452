#ifndef USER_MARKLIN_RESERVATION_H_
#define USER_MARKLIN_RESERVATION_H_

#define RESERVATION_SERVER_NAME "RESERVATION_SERVER"

namespace marklin {

const int NUM_SEGMENTS = 37;

class ResvRequest {
  enum class Action { Query, Update };
  Action action;
  char segments[NUM_SEGMENTS];
  signed char semaphores[NUM_SEGMENTS];

  friend class ReservationServer;

 public:
  ResvRequest();
  bool canReserve(int trainId, int segId) const;
  bool isReservedBy(int trainId, int segId) const;
  bool reserve(int trainId, int segId);
  bool free(int trainId, int segId);
  void print();
};

class ReservationServer {
  char segments[NUM_SEGMENTS];
  signed char semaphores[NUM_SEGMENTS];
  void run();
  void onReceiveQuery(int senderTid, ResvRequest &request);
  void onReceiveUpdate(int senderTid, ResvRequest &request);

 public:
  ReservationServer();
  static void runServer();
};

}  // namespace marklin

#endif  // USER_MARKLIN_RESERVATION_H_