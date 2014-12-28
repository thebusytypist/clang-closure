#include <stdio.h>
#include "function.h"
#include "struct.h"

int inc(int x) {
    return x + 1;
}

int main(void) {
    int a = inc(5);
    a = function(a);
    return a;
}
