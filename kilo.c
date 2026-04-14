#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>



int main(void) {
    char c;
    while(read(STDIN_FILENO, &c, 1) == 1 && c != 'q');
    return 0;
}