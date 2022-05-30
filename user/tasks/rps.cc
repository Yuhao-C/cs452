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
  int status = send(serverTid, msg, reply);
  return status >= 0 ? reply : -1;
}

int play(int serverTid, char move) {
  int reply = -1;
  int status = send(serverTid, move, reply);
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
  const int *opponentTId;
  const int *opponentPlay;

  int colorTable[64] = {0};
  int colorLen = 12;
  int colors[colorLen] = {31, 32, 33, 34, 35, 36, 91, 92, 93, 94, 95, 96};
  int colorCounter = 0;

  registerAs(RPS_SERVER_NAME);

  while (true) {
    int receivedLen = receive(clientTid, msg);
    assert(receivedLen == sizeof(char));

    if (colorTable[clientTid] == 0) {
      colorTable[clientTid] = colors[colorCounter];
      colorCounter = (colorCounter + 1) % colorLen;
    }

    switch (msg) {
      case ACTION_SIGN_UP:
        play.put(clientTid, -1);
        if (waitingTid >= 0) {
          bwprintf(COM2,
                   "[RPS Server]: matched \033[%dm[Player %d]\033[0m and "
                   "\033[%dm[Player %d]\033[0m\n\r",
                   colorTable[waitingTid], waitingTid, colorTable[clientTid],
                   clientTid);
          opponents.put(waitingTid, clientTid);
          opponents.put(clientTid, waitingTid);
          reply(waitingTid, colorTable[waitingTid]);
          reply(clientTid, colorTable[clientTid]);
          waitingTid = -1;
        } else {
          waitingTid = clientTid;
        }
        break;
      case ACTION_QUIT:
        play.put(clientTid, ACTION_QUIT);
        opponentTId = opponents.get(clientTid);
        if (opponentTId && *opponentTId >= 0) {
          opponentPlay = play.get(*opponentTId);
          if (opponentPlay && *opponentPlay >= 0) {
            // opponent has played
            rply = REPLY_OPPONENT_QUIT;
            // reply to opponent
            reply(*opponentTId, rply);
            play.put(clientTid, -1);
            play.put(*opponentTId, -1);
            bwgetc(COM2);
          }
        }
        // reply to self
        rply = 0;
        reply(clientTid, rply);
        break;
      case ACTION_ROCK:
      case ACTION_PAPER:
      case ACTION_SCISSORS:
        play.put(clientTid, msg);
        opponentTId = opponents.get(clientTid);

        if (opponentTId && *opponentTId >= 0) {
          const int *opponentPlay = play.get(*opponentTId);

          if (opponentPlay && *opponentPlay >= 0) {
            // opponent have played
            if (*opponentPlay == ACTION_QUIT) {
              // reply to self
              rply = REPLY_OPPONENT_QUIT;
              reply(clientTid, rply);
            } else {
              assert(ACTION_ROCK <= *opponentPlay &&
                     *opponentPlay <= ACTION_SCISSORS);
              int rpsResult = getRpsResult(msg, *opponentPlay);
              // reply to self
              reply(clientTid, rpsResult);
              // reply to opponent
              int opponentRpsResult = 2 - rpsResult;
              reply(*opponentTId, opponentRpsResult);
            }

            play.put(clientTid, -1);
            play.put(*opponentTId, -1);

            bwgetc(COM2);
          }
        }
        break;
      default:
        bwputstr(COM2, "[RPS Server]: fatal error unknown action\n\r");
        assert(false);
    }
  }
}

void clientLog(int tid, int color, const char *msg) {
  bwprintf(COM2, "\033[%dm[RPS Player %d]\033[0m: %s\n\r", color, tid, msg);
}

void logMove(int tid, int color, char move) {
  switch (move) {
    case ACTION_SIGN_UP:
      clientLog(tid, color, "🙋\tSign Up");
      break;
    case ACTION_ROCK:
      clientLog(tid, color, "👊\tRock");
      break;
    case ACTION_PAPER:
      clientLog(tid, color, "🖐️\tPaper");
      break;
    case ACTION_SCISSORS:
      clientLog(tid, color, "✌️\tScissors");
      break;
    case ACTION_QUIT:
      clientLog(tid, color, "💨\tQuit");
      break;
  }
}

void logResult(int tid, int color, int result) {
  switch (result) {
    case REPLY_WIN:
      clientLog(tid, color, "🥳\tWin");
      break;
    case REPLY_TIE:
      clientLog(tid, color, "🤔\tTie");
      break;
    case REPLY_LOSE:
      clientLog(tid, color, "😭\tLose");
      break;
    case REPLY_OPPONENT_QUIT:
      clientLog(tid, color, "🏳️\tOpponent Quit");
      break;
  }
}

/**
 * player1 plays two games and quit
 */
void player1() {
  int tid = myTid();
  int serverTid = whoIs(RPS_SERVER_NAME);

  logMove(tid, 0, ACTION_SIGN_UP);
  int color = signUp(serverTid);
  if (color < 0) {
    clientLog(tid, color, "❌\tFailed to sign up");
    return;
  }

  for (int i = 0; i < 2; ++i) {
    char move = timer::getTick() % 3;
    logMove(tid, color, move);
    int result = play(serverTid, move);
    logResult(tid, color, result);
  }
  logMove(tid, color, ACTION_QUIT);
  quit(serverTid);
}

/**
 * player2 only plays one game and quit
 */
void player2() {
  int tid = myTid();
  int serverTid = whoIs(RPS_SERVER_NAME);

  logMove(tid, 0, ACTION_SIGN_UP);
  int color = signUp(serverTid);
  if (color < 0) {
    clientLog(tid, color, "❌\tFailed to sign up");
    return;
  }

  char move = timer::getTick() % 3;
  logMove(tid, color, move);

  int result = play(serverTid, move);
  logResult(tid, color, result);

  logMove(tid, color, ACTION_QUIT);
  quit(serverTid);
}

/**
 * player3 wants to play two games, if
 * the opponent quits before two games finish,
 * player1 will sign up again to play 2 games
 */
void player3() {
  int tid = myTid();
  int serverTid = whoIs(RPS_SERVER_NAME);

  logMove(tid, 0, ACTION_SIGN_UP);
  int color = signUp(serverTid);
  if (color < 0) {
    clientLog(tid, color, "❌\tFailed to sign up");
    return;
  }

  for (int i = 0; i < 2; ++i) {
    char move = timer::getTick() % 3;
    logMove(tid, color, move);
    int result = play(serverTid, move);
    logResult(tid, color, result);

    if (result == REPLY_OPPONENT_QUIT) {
      int color = signUp(serverTid);
      if (color < 0) {
        clientLog(tid, color, "❌\tFailed to sign up");
        return;
      }
      i = -1;
    }
  }
  logMove(tid, color, ACTION_QUIT);
  quit(serverTid);
}

}  // namespace rps