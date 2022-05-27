#include "boot.h"

#include "k1.h"
#include "lib/bwio.h"
#include "name_server.h"
#include "user/task.h"

void boot() { create(0, nameServer); }