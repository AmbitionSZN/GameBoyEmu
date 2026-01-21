#include "dbg.h"
#include "bus.h"
#include <stdio.h>

static char dbgMsg[1024] = {0};
static int msgSize = 0;

void dbgUpdate() {
    if (busRead(0xFF02) == 0x81) {
        char c = busRead(0xFF01);

        dbgMsg[msgSize++] = c;

        busWrite(0xFF02, 0);
    }
}

void dbgPrint() {
    if (dbgMsg[0]) {
        printf("DBG: %s\n", dbgMsg);
    }
}
