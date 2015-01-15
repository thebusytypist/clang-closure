#include "function.h"

static int getValue() {
    int r = 1;
    return r;
}

int function(int x) {
    return x + getValue();
}