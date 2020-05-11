#include "lib.h"

int parse_number(const char* str) {
    char* str_end;
    long number = strtol(str, &str_end, 10);

    if (errno == ERANGE && (number == LONG_MAX || number == LONG_MIN))
        return -1;
    if ((str_end == str) || (errno != 0 && number == 0))
        return -1;

    return (int) number;
}
