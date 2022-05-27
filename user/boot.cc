#include "boot.h"

#include "k1.h"
#include "lib/bwio.h"
#include "lib/timer.h"
#include "name_server.h"
#include "rps.h"
#include "user/task.h"

void boot() {
  timer::stop();
  timer::load();
  timer::start();
  create(0, nameServer);
  create(2, rps::server);
  for (int i = 0; i < 2; ++i) create(4, rps::client);
}