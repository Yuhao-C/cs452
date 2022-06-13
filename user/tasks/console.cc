#include "clock_server.h"
#include "display_server.h"
#include "lib/io.h"
#include "lib/queue.h"
#include "lib/string.h"
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

void showInvalidCommand(int displayServerTid) {
  view::Msg msg{view::Action::InvalidCmd, {1}};
  send(displayServerTid, msg);
}

void clearInvalidCommand(int displayServerTid) {
  view::Msg msg{view::Action::InvalidCmd, {0}};
  send(displayServerTid, msg);
}

void handleCmd(char* cmd, int displayServerTid, int marklinServerTid) {
  char word[32];
  char a2iRes;
  const char* temp_word;

  readWord(&cmd, word);
  if (String(word) == "tr") {
    int trainNum;
    int trainSpeed;
    readWord(&cmd, word);
    if (word[0] == 0) {
      showInvalidCommand(displayServerTid);
      return;
    }
    temp_word = word;
    a2iRes = a2i('0', &temp_word, 10, &trainNum);
    if (a2iRes != 0) {
      showInvalidCommand(displayServerTid);
      return;
    }
    readWord(&cmd, word);
    if (word[0] == 0) {
      showInvalidCommand(displayServerTid);
      return;
    }
    temp_word = word;
    a2iRes = a2i('0', &temp_word, 10, &trainSpeed);
    if (a2iRes != 0 || cmd[0] != 0) {
      showInvalidCommand(displayServerTid);
      return;
    }
    clearInvalidCommand(displayServerTid);
    send(marklinServerTid, marklin::Msg::tr(trainSpeed, trainNum));
  } else if (String{word} == "rv") {
    int trainNum;
    readWord(&cmd, word);
    if (word[0] == 0) {
      showInvalidCommand(displayServerTid);
      return;
    }
    temp_word = word;
    a2iRes = a2i('0', &temp_word, 10, &trainNum);
    if (a2iRes != 0 || cmd[0] != 0) {
      showInvalidCommand(displayServerTid);
      return;
    }
    clearInvalidCommand(displayServerTid);
    send(marklinServerTid, marklin::Msg::rv(trainNum));
  } else if (String{word} == "sw") {
    int switchNum;
    char switchDirection;
    readWord(&cmd, word);
    if (word[0] == 0) {
      showInvalidCommand(displayServerTid);
      return;
    }
    temp_word = word;
    a2iRes = a2i('0', &temp_word, 10, &switchNum);
    if (a2iRes != 0) {
      showInvalidCommand(displayServerTid);
      return;
    }
    readWord(&cmd, word);
    if ((word[0] != 'S' && word[0] != 'C') || word[1] != 0 || cmd[0] != 0) {
      showInvalidCommand(displayServerTid);
      return;
    }
    switchDirection = word[0];
    clearInvalidCommand(displayServerTid);
    send(marklinServerTid, marklin::Msg::sw(switchDirection, switchNum));
    send(displayServerTid,
         view::Msg{view::Action::Switch, {switchDirection, switchNum}});
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
      handleCmd(cmd, displayServerTid, marklinServerTid);
      cmdIdx = 0;
    } else if (ch > 0 && cmdIdx <= 30) {
      cmd[cmdIdx++] = ch;
      render(displayServerTid, ch);
    }
  }
}
