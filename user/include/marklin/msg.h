#ifndef USER_MARKLIN_MSG_H_
#define USER_MARKLIN_MSG_H_

namespace marklin {

struct Msg {
  enum class Action {
    Ready = 0,            // data = {}
    SwitchReady,          // data = {}
    Cmd,                  // data = {cmd1[, cmd2]}
    ReverseReady = 0x10,  // data = {0 | originalSpeed, trainId}
    TrainCmd,             // data = {speed, trainId}
    ReverseCmd,           // data = {0, trainId}
    SwitchCmd = 0x20,     // data = {direction, switchId}
    InitTrack = 0x30,     // data = {trackSet}
    SetTrainLoc,          // data = {trainId, nodeIndex, offset, direction}
    SensorTriggered,      // data = {sensorNum, tick[, trainId, predictNext]}
    SetDestination,   // data = {trainId, dest_node | -1, offset[, speedLevel |
                      // stopDist, velocity, src_node, next_sensor]}
    Reroute,          // data = {trainId}
    Depart,           // data = {trainId, nodeIndex, offset}
    SetTrainBlocked,  // data = {trainId, blocked}
  };
  Action action;
  int data[12];
  int len;

  int getTrainId() const;

  static Msg go();
  static Msg stop();
  static Msg tr(char speed, char trainId);
  static Msg sw(char direction, char switchId);
  static Msg rv(char trainId);
  static Msg solenoidOff();
  static Msg querySensors();
};

}  // namespace marklin

#endif  // USER_MARKLIN_MSG_H_