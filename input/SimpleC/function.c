#include "function.h"

int getValue() {
    int r = 1;
    return r;
}

int function(int x) {
    return x + getValue();
}