#include "arch/ts7200.h"
#include "lib/bwio.h"

int main() { bwputstr(COM2, "Hello kernel!\r\n"); }
