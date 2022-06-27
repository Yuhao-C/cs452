#include "display_server.h"

#include "lib/assert.h"
#include "lib/io.h"
#include "marklin_server.h"
#include "name_server.h"
#include "user/message.h"

namespace view {

Cursor *Cursor::activeCursor = nullptr;

Cursor::Cursor(int r, int c) {
  this->initR = r;
  this->initC = c;
  this->r = r;
  this->c = c;
  commit();
}

void Cursor::showCursor() { ::putstr(COM2, "\033[?25h"); }
void Cursor::hideCursor() { ::putstr(COM2, "\033[?25l"); }

void Cursor::commit() {
  printf(COM2, "\033[%d;%dH", r, c);
  activeCursor = this;
}

void Cursor::set(int r, int c) {
  this->r = r;
  this->c = c;
  commit();
}

void Cursor::setR(int r) { set(r, c); }
void Cursor::setC(int c) { set(r, c); }
void Cursor::nextLine() { set(r + 1, 1); }
void Cursor::deleteLine() {
  if (activeCursor != this) {
    commit();
  }
  ::putstr(COM2, "\033[K");
}
void Cursor::putc(char ch) {
  if (activeCursor != this) {
    commit();
  }
  ::putc(COM2, ch);
  setC(c + 1);
}
void Cursor::putstr(const char *str) {
  if (activeCursor != this) {
    commit();
  }
  int count = ::putstr(COM2, str);
  setC(c + count);
}

void parseTime(int time, int &min, int &sec, int &ms) {
  int timeMs = time * 10;
  min = timeMs / 60000;
  timeMs -= min * 60000;
  sec = timeMs / 1000;
  timeMs -= sec * 1000;
  ms = timeMs;
}

void switchInit() {
  println(COM2, "Switches");
  println(COM2, "┌───────────────┬───────────────┐");
  for (int i = 1; i <= 18; i += 2) {
    println(COM2, "│ %d\t?\t│ %d\t?\t│", i, i + 1);
  }
  for (int i = 153; i <= 156; i += 2) {
    println(COM2, "│ %d\t?\t│ %d\t?\t│", i, i + 1);
  }
  println(COM2, "└───────────────┴───────────────┘");
}

void sensorInit(Cursor &sensorCursor) {
  println(COM2, "  Most Recently Triggered Sensors");
  sensorCursor.setR(sensorCursor.r + 1);
  println(COM2, "───────────────────────────────────");
}

void renderInput(Cursor &cursor, char ch) {
  if (ch == 8) {
    // backspace
    cursor.setC(cursor.c - 1);
    cursor.deleteLine();
  } else if (ch == 13) {
    // enter
    cursor.setC(3);
    cursor.deleteLine();
  } else if (ch > 0) {
    cursor.putc(ch);
  }
}

void renderSwitch(Cursor &cursor, int *data) {
  Cursor::hideCursor();
  int switchDirection = data[0];
  int switchNum = data[1];
  if (!((switchNum >= 1 && switchNum <= 18) ||
        (switchNum >= 153 && switchNum <= 156))) {
    // incorrect switch number
    return;
  }
  int line = switchNum <= 18 ? (switchNum - 1) / 2 : 9 + (switchNum - 153) / 2;
  int row = line + cursor.initR + 2;
  int col = switchNum % 2 ? 9 : 25;
  cursor.set(row, col);
  cursor.putc(switchDirection);
}

void renderSensor(Cursor &cursor, int *data, int len) {
  Cursor::hideCursor();
  cursor.set(cursor.initR + 2, cursor.initC);
  for (int i = 0; i < len; ++i) {
    int sensorIdx = data[i * 2];
    int sensorTime = data[i * 2 + 1];
    int sensorTimeMin, sensorTimeSec, sensorTimeMs;
    parseTime(sensorTime, sensorTimeMin, sensorTimeSec, sensorTimeMs);
    char sensorGroup;
    int sensorNum;
    marklin::Sensor::getSensorName(sensorIdx, sensorGroup, sensorNum);
    printf(COM2, "%d)\t%c%02d\t%02d:%02d.%d", i + 1, sensorGroup, sensorNum,
           sensorTimeMin, sensorTimeSec, sensorTimeMs / 100);
    cursor.set(cursor.r + 1, cursor.initC);
  }
}

void renderInvalidCmd(Cursor &cursor, int enable) {
  Cursor::hideCursor();
  cursor.setC(1);
  cursor.deleteLine();
  if (enable) {
    cursor.putstr("Invalid Command!");
  }
}

void renderTime(Cursor &cursor, int *data) {
  int sysTime = data[0];
  int idleTime = data[1];
  int sysTimeMin, sysTimeSec, sysTimeMs, idleTimeMin, idleTimeSec, idleTimeMs;
  parseTime(data[0], sysTimeMin, sysTimeSec, sysTimeMs);
  parseTime(data[1], idleTimeMin, idleTimeSec, idleTimeMs);

  Cursor::hideCursor();
  cursor.setC(1);
  printf(COM2, "sys: %02d:%02d.%d, idle: %02d:%02d.%d, idle fraction: %u.%u%%",
         sysTimeMin, sysTimeSec, sysTimeMs / 100, idleTimeMin, idleTimeSec,
         idleTimeMs / 100, idleTime * 100 / sysTime,
         (idleTime * 1000 / sysTime) % 10);
  cursor.deleteLine();
}

void renderPredict(Cursor &cursor, int *data) {
  int nextSensorNum = data[0];
  int nextSensorTick = data[1];
  int isLastPredictValid = data[2];
  int timeDiff = data[3];
  int distDiff = data[4];
  char sensorGroup;
  int sensorNum;
  marklin::Sensor::getSensorName(nextSensorNum, sensorGroup, sensorNum);

  Cursor::hideCursor();
  cursor.setC(1);
  printf(COM2, "next %c%02d", sensorGroup, sensorNum);
  cursor.deleteLine();
}

void displayServer() {
  registerAs(DISPLAY_SERVER_NAME);
  putstr(COM2, "\033[2J");  // clear screen

  int senderTid = -1;

  Cursor timeCursor{1, 1};
  
  Cursor predictCursor{3, 1};

  Cursor switchCursor{5, 13};
  switchInit();

  Cursor sensorCursor{5, 40};
  sensorInit(sensorCursor);

  Cursor inputCursor{22, 1};
  inputCursor.putstr("> ");

  Cursor invalidCmdCursor{23, 1};
  bool quit = false;

  while (true) {
    struct Msg msg;
    receive(senderTid, msg);
    switch (msg.action) {
      case Input:
        renderInput(inputCursor, msg.data[0]);
        break;
      case Switch:
        renderSwitch(switchCursor, msg.data);
        break;
      case Sensor:
        renderSensor(sensorCursor, msg.data, msg.len);
        break;
      case Time:
        renderTime(timeCursor, msg.data);
        break;
      case Predict:
        renderPredict(predictCursor, msg.data);
        break;
      case InvalidCmd:
        renderInvalidCmd(invalidCmdCursor, msg.data[0]);
        break;
      case Quit:
        quit = true;
        break;
      default:
        assert(false);
        break;
    }
    inputCursor.commit();
    Cursor::showCursor();
    reply(senderTid);
    if (quit) {
      break;
    }
  }
}
}  // namespace view
