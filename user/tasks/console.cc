#include "clock_server.h"
#include "display_server.h"
#include "lib/io.h"
#include "lib/queue.h"
#include "lib/string.h"
#include "marklin/world.h"
#include "marklin_server.h"
#include "name_server.h"
#include "uart_server.h"
#include "user/message.h"
#include "user/sys.h"
#include "user/task.h"

void readWord(char** cmdAddr, char* word) {
  int idx = 0;
  // read first word
  char* cmd = *cmdAddr;
  while (*cmd != 32 && *cmd != 0) {
    word[idx++] = *cmd;
    ++cmd;
  }
  // null terminate the string
  word[idx++] = 0;
  // skip whitespaces
  while (*cmd == 32) {
    ++cmd;
  }
  *cmdAddr = cmd;
}

bool parseInt(const char* str, int& n) {
  int sign{1}, base{0};
  if (*str == '-') {
    sign = -1;
    ++str;
  }
  while ('0' <= *str && *str <= '9') {
    base = 10 * base + (*str - '0');
    ++str;
  }
  n = base * sign;
  return *str == 0;
}

void showInvalidCommand(int displayServerTid) {
  view::Msg msg{view::Action::InvalidCmd, {1}};
  send(displayServerTid, msg);
}

void clearInvalidCommand(int displayServerTid) {
  view::Msg msg{view::Action::InvalidCmd, {0}};
  send(displayServerTid, msg);
}

void handleCmd(char* cmd, int displayServerTid, int marklinServerTid,
               int worldTid) {
  if (cmd[0] == 0) {
    showInvalidCommand(displayServerTid);
    return;
  }
  char* cmds[10];
  cmds[0] = cmd;
  int cmdsLen = 1;
  int i = 0;
  while (cmd[i] != 0) {
    if (cmd[i] == ' ') {
      while (cmd[i] == ' ') {
        cmd[i] = 0;
        ++i;
      }
      if (cmd[i] != 0) {
        cmds[cmdsLen] = cmd + i;
        ++cmdsLen;
      }
    } else {
      ++i;
    }
  }

  if (String{cmds[0]} == "tr") {
    if (cmdsLen != 3) {
      showInvalidCommand(displayServerTid);
      return;
    }
    int trainNum;
    int trainSpeed;
    if (!parseInt(cmds[1], trainNum) || !parseInt(cmds[2], trainSpeed)) {
      showInvalidCommand(displayServerTid);
      return;
    }
    clearInvalidCommand(displayServerTid);
    send(worldTid, marklin::Msg::tr(trainSpeed, trainNum));
  } else if (String{cmds[0]} == "rv") {
    if (cmdsLen != 2) {
      showInvalidCommand(displayServerTid);
      return;
    }
    int trainNum;
    if (!parseInt(cmds[1], trainNum)) {
      showInvalidCommand(displayServerTid);
      return;
    }
    clearInvalidCommand(displayServerTid);
    send(worldTid, marklin::Msg::rv(trainNum));
  } else if (String{cmds[0]} == "sw") {
    if (cmdsLen != 3) {
      showInvalidCommand(displayServerTid);
      return;
    }
    int switchNum;
    char switchDirection;
    if (!parseInt(cmds[1], switchNum)) {
      showInvalidCommand(displayServerTid);
      return;
    }
    if (String{cmds[2]} == "S" || String{cmds[2]} == "C") {
      switchDirection = cmds[2][0];
      clearInvalidCommand(displayServerTid);
      send(worldTid, marklin::Msg::sw(switchDirection, switchNum));
    } else {
      showInvalidCommand(displayServerTid);
      return;
    }
  } else if (String{cmds[0]} == "track") {
    char track;
    if (cmdsLen != 2) {
      showInvalidCommand(displayServerTid);
      return;
    }
    if (String{cmds[1]} == "A" || String{cmds[1]} == "B") {
      track = cmds[1][0];
      clearInvalidCommand(displayServerTid);
      send(worldTid, marklin::Msg{marklin::Msg::Action::InitTrack, {track}, 1});
    } else {
      showInvalidCommand(displayServerTid);
      return;
    }
  } else if (String{cmds[0]} == "loc") {
    if (cmdsLen != 4) {
      showInvalidCommand(displayServerTid);
      return;
    }
    int trainNum;
    int nodeIdx;
    if (parseInt(cmds[1], trainNum) && parseInt(cmds[2], nodeIdx) &&
        (cmds[3][0] == 'f' || cmds[3][0] == 'b')) {
      char direction = cmds[3][0];
      clearInvalidCommand(displayServerTid);
      send(worldTid, marklin::Msg{marklin::Msg::Action::SetTrainLoc,
                                  {trainNum, nodeIdx, 0, direction},
                                  4});
    } else {
      showInvalidCommand(displayServerTid);
      return;
    }
  } else if (String{cmds[0]} == "route") {
    if (cmdsLen != 5) {
      showInvalidCommand(displayServerTid);
      return;
    }
    int trainNum;
    int destIdx;
    int destOffset;
    char speed;
    if (!parseInt(cmds[1], trainNum) || !parseInt(cmds[2], destIdx) ||
        !parseInt(cmds[3], destOffset)) {
      showInvalidCommand(displayServerTid);
      return;
    }
    if (String{cmds[4]} == "h" || String{cmds[4]} == "l") {
      speed = cmds[4][0];
      clearInvalidCommand(displayServerTid);
      send(worldTid, marklin::Msg{marklin::Msg::Action::SetDestination,
                                  {trainNum, destIdx, destOffset, speed},
                                  4});
    } else {
      showInvalidCommand(displayServerTid);
      return;
    }
  } else {
    showInvalidCommand(displayServerTid);
  }
}

void render(int displayServerTid, int ch) {
  view::Msg msg{view::Action::Input, {(unsigned int)ch}};
  send(displayServerTid, msg);
}

void consoleReader() {
  int displayServerTid = whoIs(DISPLAY_SERVER_NAME);
  int marklinServerTid = whoIs(MARKLIN_SERVER_NAME);
  int worldTid = whoIs(WORLD_NAME);

  send(marklinServerTid, marklin::Msg::go());

  char cmd[32];
  int cmdIdx = 0;

  while (true) {
    int ch = getc(COM2);
    assert(ch >= 0);
    if (ch == 8) {
      // backspace
      if (cmdIdx > 0) {
        cmd[--cmdIdx] = 0;
        render(displayServerTid, ch);
      }
    } else if (ch == 13) {
      render(displayServerTid, ch);
      cmd[cmdIdx++] = 0;
      // enter
      if (String{cmd} == "q") {
        send(marklinServerTid, marklin::Msg::stop());
        send(displayServerTid, view::Msg{view::Action::Quit});
        clock::delay(20);  // make sure "q" is sent
        shutdown();
      }
      handleCmd(cmd, displayServerTid, marklinServerTid, worldTid);
      cmdIdx = 0;
    } else if (ch > 0 && cmdIdx <= 30) {
      cmd[cmdIdx++] = ch;
      render(displayServerTid, ch);
    }
  }
}
