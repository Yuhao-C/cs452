#include "rps.h"

#include "lib/hashtable.h"
#include "lib/queue.h"
#include "lib/timer.h"
#include "user/message.h"
#include "user/task.h"

#define RPS_SERVER_NAME "RPS_SERVER"

#define ACTION_ROCK 0
#define ACTION_PAPER 1
#define ACTION_SCISSORS 2
#define ACTION_SIGN_UP 3
#define ACTION_QUIT 4

#define REPLY_OPPONENT_QUIT -1
#define REPLY_LOSE 0
#define REPLY_TIE 1
#define REPLY_WIN 2

namespace rps {

int signUp(int serverTid) {
  char msg = ACTION_SIGN_UP;
  int reply = -1;
  int status = send(serverTid, &msg, sizeof(char), &reply, sizeof(int));
  return status >= 0 ? reply : -1;
}

int play(int serverTid, char move) {
  int reply = -1;
  int status = send(serverTid, &move, sizeof(char), &reply, sizeof(int));
  return status >= 0 ? reply : -1;
}

int quit(int serverTid) {
  char msg = ACTION_QUIT;
  int reply = -1;
  int status = send(serverTid, msg, reply);
  return status >= 0 ? reply : -1;
}

int getRpsResult(int myPlay, int opponentPlay) {
  if ((myPlay + 1) % 3 == opponentPlay) {
    return REPLY_LOSE;
  } else if (myPlay == opponentPlay) {
    return REPLY_TIE;
  }
  return REPLY_WIN;
}

void server() {
  int clientTid = -1;
  char msg;
  int waitingTid = -1;
  // Queue<int, 10> queue;
  HashTable<int, int, 64, 10> opponents;
  HashTable<int, int, 64, 10> play;
  int rply;
  const int *opponentId;

  registerAs(RPS_SERVER_NAME);

  while (true) {
    int receivedLen = receive(&clientTid, &msg, sizeof(char));
    assert(receivedLen == sizeof(char));
    switch (msg) {
      case ACTION_SIGN_UP:
        if (waitingTid >= 0) {
          opponents.put(waitingTid, clientTid);
          opponents.put(clientTid, waitingTid);
          rply = 0;
          reply(waitingTid, &rply, sizeof(int));
          reply(clientTid, &rply, sizeof(int));

          waitingTid = -1;
        } else {
          waitingTid = clientTid;
        }
        break;
      case ACTION_QUIT:
        play.put(clientTid, ACTION_QUIT);
        opponents.put(clientTid, -1);
        rply = 0;
        reply(clientTid, &rply, sizeof(int));
        break;
      case ACTION_ROCK:
      case ACTION_PAPER:
      case ACTION_SCISSORS:
        play.put(clientTid, msg);
        opponentId = opponents.get(clientTid);

        if (opponentId && *opponentId >= 0) {
          const int *opponentPlay = play.get(*opponentId);

          if (opponentPlay && *opponentPlay >= 0) {
            if (*opponentPlay == ACTION_QUIT) {
              // reply to self
              rply = REPLY_OPPONENT_QUIT;
              reply(*opponentId, &rply, sizeof(int));
            } else {
              assert(ACTION_ROCK <= *opponentPlay &&
                     *opponentPlay <= ACTION_SCISSORS);
              int rpsResult = getRpsResult(msg, *opponentPlay);
              // reply to self
              reply(clientTid, &rpsResult, sizeof(int));
              // reply to opponent
              int opponentRpsResult = 2 - rpsResult;
              reply(*opponentId, &opponentRpsResult, sizeof(int));
            }

            play.put(clientTid, -1);
            play.put(*opponentId, -1);
          }
        }
        break;
      default:
        bwputstr(COM2, "rps server: fatal error unknown action\n\r");
        assert(false);
    }
  }
}

void client() {
  int tid = myTid();
  int serverTid = whoIs(RPS_SERVER_NAME);
  bwprintf(COM2, "rps client %d: found server at %d\n\r", tid, serverTid);
  if (signUp(serverTid) < 0) {
    bwprintf(COM2, "rps client %d: failed to sign up\n\r", tid);
    return;
  }
  for (int i = 0; i < 3; ++i) {
    char move = timer::getTick() % 3;
    switch (move) {
      case ACTION_ROCK:
        bwprintf(COM2, "rps client %d: play ROCK\n\r", tid);
        break;
      case ACTION_PAPER:
        bwprintf(COM2, "rps client %d: play PAPER\n\r", tid);
        break;
      case ACTION_SCISSORS:
        bwprintf(COM2, "rps client %d: play SCISSORS\n\r", tid);
        break;
    }
    int result = play(serverTid, move);
    switch (result) {
      case REPLY_WIN:
        bwprintf(COM2, "rps client %d: win\n\r", tid);
        break;
      case REPLY_TIE:
        bwprintf(COM2, "rps client %d: tie\n\r", tid);
        break;
      case REPLY_LOSE:
        bwprintf(COM2, "rps client %d: loss\n\r", tid);
        break;
      case REPLY_OPPONENT_QUIT:
        bwprintf(COM2, "rps client %d: opponent quit\n\r", tid);
        break;
    }
    yield();
    bwgetc(COM2);
  }
  quit(serverTid);
}

}  // namespace rps