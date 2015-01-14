#include "function.h"

static int getValue() {
    return 1;
}

int function(int x) {
    return x + getValue();
}