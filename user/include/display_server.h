#ifndef USER_DISPLAY_SERVER_H_
#define USER_DISPLAY_SERVER_H_

#include "lib/io.h"
#include "marklin_server.h"

#define DISPLAY_SERVER_NAME "DISPLAY_SERVER"

namespace view {
enum Action { Input, Sensor, Switch, Time, InvalidCmd, Quit };

struct Msg {
  Action action;
  int data[MRV_CAP * 2];
  int len;
};

struct Cursor {
  int initR;
  int initC;
  int r;
  int c;

  static Cursor *activeCursor;

  Cursor(int r, int c);

  static void showCursor();
  static void hideCursor();

  void commit();
  void set(int r, int c);
  void setR(int r);
  void setC(int c);
  void nextLine();
  void deleteLine();
  void putc(char ch);
  void putstr(const char *str);
};

void displayServer();
}  // namespace view

#endif  // USER_CMD_SERVER_H_