#include "marklin_server.h"

#include "clock_server.h"
#include "display_server.h"
#include "lib/io.h"
#include "lib/math.h"
#include "lib/queue.h"
#include "marklin/world.h"
#include "name_server.h"
#include "user/message.h"
#include "user/task.h"

namespace marklin {

int Msg::getTrainId() const { return data[1]; }

Msg Msg::go() { return {Action::Cmd, {GO}, 1}; }

Msg Msg::stop() { return {Action::Cmd, {STOP}, 1}; }

Msg Msg::tr(char speed, char trainId) {
  return {Action::TrainCmd, {speed, trainId}, 2};
}

Msg Msg::sw(char direction, char switchId) {
  assert(direction == 'S' || direction == 'C');
  return {
      Action::SwitchCmd, {direction == 'S' ? SWITCH_S : SWITCH_C, switchId}, 2};
}

Msg Msg::rv(char trainId) { return {Action::ReverseCmd, {0, trainId}, 1}; }

Msg Msg::solenoidOff() { return {Action::SwitchCmd, {SWITCH_OFF}, 1}; }

Msg Msg::querySensors() { return {Action::Cmd, {SENSOR}, 1}; }

void Sensor::getSensorName(int sensorIdx, char &sensorGroup, int &sensorNum) {
  sensorGroup = sensorIdx / 16 + 'A';
  sensorNum = sensorIdx % 16 + 1;
}

void cmdDelegate() {
  int cmdServerTid = whoIs(MARKLIN_SERVER_NAME);
  Msg readyMsg{Msg::Action::Ready, {}, 0};
  Msg cmdMsg;

  while (true) {
    send(cmdServerTid, readyMsg, cmdMsg);
    for (int i = 0; i < cmdMsg.len; ++i) {
      putc(COM1, cmdMsg.data[i]);
    }
  }
}

void swNotifier() {
  int serverTid = whoIs(MARKLIN_SERVER_NAME);
  Msg swReadyMsg{Msg::Action::SwitchReady, {}, 0};

  while (true) {
    clock::delay(15);
    send(serverTid, swReadyMsg);
  }
}

void querySensors() {
  Sensor sensor{0, 0, {0}, {0}, {{0, 0}}};
  bool firstRead = true;
  int marklinServerTid = whoIs(MARKLIN_SERVER_NAME);
  int displayServerTid = whoIs(DISPLAY_SERVER_NAME);
  int worldTid = whoIs(WORLD_NAME);
  while (true) {
    send(marklinServerTid, Msg::querySensors());
    bool updated = false;
    for (int i = 0; i < 10; ++i) {
      char byte = getc(COM1);
      int bitIdx = i * 8;
      for (int j = 7; j >= 0; --j) {
        int bit = (byte >> j) & 1;
        if (bit == 0) {
          sensor.status[bitIdx] = 0;
        } else if (sensor.status[bitIdx] == 0) {
          int currTime = clock::time();
          updated = true;
          sensor.status[bitIdx] = 1;
          sensor.mostRecentlyVisited[sensor.mrvIdx] = MrvNode{bitIdx, currTime};
          sensor.mrvIdx = (sensor.mrvIdx + 1) % MRV_CAP;
          sensor.mrvSize =
              sensor.mrvSize >= MRV_CAP ? MRV_CAP : sensor.mrvSize + 1;
          while (worldTid < 0) {
            worldTid = whoIs(WORLD_NAME);
          }
          send(worldTid,
               Msg{Msg::Action::SensorTriggered, {bitIdx, currTime}, 2});
        }
        ++bitIdx;
      }
    }
    if (firstRead) {
      firstRead = false;
      continue;
    }
    if (updated) {
      view::Msg msg;
      msg.action = view::Action::Sensor;
      int idx = mod(sensor.mrvIdx - 1, MRV_CAP);
      for (int i = 0; i < sensor.mrvSize; ++i) {
        msg.data[i * 2] = sensor.mostRecentlyVisited[idx].bitIdx;
        msg.data[i * 2 + 1] = sensor.mostRecentlyVisited[idx].time;
        idx = mod(idx - 1, MRV_CAP);
      }
      msg.len = sensor.mrvSize;
      while (displayServerTid < 0) {
        displayServerTid = whoIs(DISPLAY_SERVER_NAME);
      }
      send(displayServerTid, msg);
    }
  }
}

void cmdServer() {
  registerAs(MARKLIN_SERVER_NAME);

  int cmdDelegateTid = create(1, cmdDelegate);
  int swNotifierTid = create(1, swNotifier);
  int queryTid = create(1, querySensors);

  int senderTid;
  Msg msg;
  Queue<Msg, 64> cmdQueue;
  Queue<Msg, 64> swQueue;
  bool cmdCanSend = false;
  bool cmdCanSendSw = false;
  bool isSolenoidOn = false;

  while (true) {
    receive(senderTid, msg);

    switch (msg.action) {
      case Msg::Action::Ready: {
        cmdCanSend = true;
        break;
      }
      case Msg::Action::SwitchReady: {
        cmdCanSendSw = true;
        break;
      }
      case Msg::Action::Cmd: {
        cmdQueue.enqueue(msg);
        reply(senderTid, 0);
        break;
      }
      case Msg::Action::TrainCmd: {
        cmdQueue.enqueue(msg);
        reply(senderTid, 0);
        break;
      }
      case Msg::Action::SwitchCmd:
        swQueue.enqueue(msg);
        reply(senderTid, 0);
        break;
    }

    if (cmdCanSend) {
      if (cmdCanSendSw && swQueue.size() > 0) {
        reply(cmdDelegateTid, swQueue.dequeue());
        reply(swNotifierTid);
        isSolenoidOn = true;
        cmdCanSendSw = false;
        cmdCanSend = false;
      } else if (cmdCanSendSw && isSolenoidOn) {
        reply(cmdDelegateTid, Msg::solenoidOff());
        reply(swNotifierTid);
        isSolenoidOn = false;
        cmdCanSendSw = false;
        cmdCanSend = false;
      } else if (cmdQueue.size() > 0) {
        reply(cmdDelegateTid, cmdQueue.dequeue());
        cmdCanSend = false;
      }
    }
  }
}

}  // namespace marklin
