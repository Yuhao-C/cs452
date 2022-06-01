#include "rps.h"

#include "lib/hashtable.h"
#include "lib/queue.h"
#include "lib/timer.h"
#include "name_server.h"
#include "user/message.h"
#include "user/task.h"

#define RPS_SERVER_NAME "RPS_SERVER"

namespace rps {

enum Action { Rock = 0, Paper, Scissors, SignUp, Quit };
enum Reply { OpponentQuit = -1, Lose, Tie, Win };

int signUp(int serverTid) {
  char msg = Action::SignUp;
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
  char msg = Action::Quit;
  int reply = -1;
  int status = send(serverTid, msg, reply);
  return status >= 0 ? reply : -1;
}

int getRpsResult(int myPlay, int opponentPlay) {
  if ((myPlay + 1) % 3 == opponentPlay) {
    return Reply::Lose;
  } else if (myPlay == opponentPlay) {
    return Reply::Tie;
  }
  return Reply::Win;
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
      case Action::SignUp:
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
      case Action::Quit:
        play.put(clientTid, Action::Quit);
        opponentTId = opponents.get(clientTid);
        if (opponentTId && *opponentTId >= 0) {
          opponentPlay = play.get(*opponentTId);
          if (opponentPlay && *opponentPlay >= 0) {
            // opponent has played
            rply = Reply::OpponentQuit;
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
      case Action::Rock:
      case Action::Paper:
      case Action::Scissors:
        play.put(clientTid, msg);
        opponentTId = opponents.get(clientTid);

        if (opponentTId && *opponentTId >= 0) {
          const int *opponentPlay = play.get(*opponentTId);

          if (opponentPlay && *opponentPlay >= 0) {
            // opponent have played
            if (*opponentPlay == Action::Quit) {
              // reply to self
              rply = Reply::OpponentQuit;
              reply(clientTid, rply);
            } else {
              assert(Action::Rock <= *opponentPlay &&
                     *opponentPlay <= Action::Scissors);
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
    case Action::SignUp:
      clientLog(tid, color, "🙋\tSign Up");
      break;
    case Action::Rock:
      clientLog(tid, color, "👊\tRock");
      break;
    case Action::Paper:
      clientLog(tid, color, "🖐️\tPaper");
      break;
    case Action::Scissors:
      clientLog(tid, color, "✌️\tScissors");
      break;
    case Action::Quit:
      clientLog(tid, color, "💨\tQuit");
      break;
  }
}

void logResult(int tid, int color, int result) {
  switch (result) {
    case Reply::Win:
      clientLog(tid, color, "🥳\tWin");
      break;
    case Reply::Tie:
      clientLog(tid, color, "🤔\tTie");
      break;
    case Reply::Lose:
      clientLog(tid, color, "😭\tLose");
      break;
    case Reply::OpponentQuit:
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

  logMove(tid, 0, Action::SignUp);
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
  logMove(tid, color, Action::Quit);
  quit(serverTid);
}

/**
 * player2 only plays one game and quit
 */
void player2() {
  int tid = myTid();
  int serverTid = whoIs(RPS_SERVER_NAME);

  logMove(tid, 0, Action::SignUp);
  int color = signUp(serverTid);
  if (color < 0) {
    clientLog(tid, color, "❌\tFailed to sign up");
    return;
  }

  char move = timer::getTick() % 3;
  logMove(tid, color, move);

  int result = play(serverTid, move);
  logResult(tid, color, result);

  logMove(tid, color, Action::Quit);
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

  logMove(tid, 0, Action::SignUp);
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

    if (result == Reply::OpponentQuit) {
      int color = signUp(serverTid);
      if (color < 0) {
        clientLog(tid, color, "❌\tFailed to sign up");
        return;
      }
      i = -1;
    }
  }
  logMove(tid, color, Action::Quit);
  quit(serverTid);
}

}  // namespace rps