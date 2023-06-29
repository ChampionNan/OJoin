#include <stdarg.h>
#include <stdio.h>

#include "Enclave.h"
#include "Enclave_t.h"

#include "join.hpp"

void printf(const char *fmt, ...)
{
    char buf[BUFSIZ] = {'\0'};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    ocall_print_string(buf);
}

void callEnclave(int *tableIds, int64_t n0, int64_t n1, int64_t N, int64_t M, int B) {
  printf("Enclave begin\n");
  join(tableIds[0], tableIds[1], tableIds[2], n0 + n1, n0, n1);
  printf("Enclave end\n");
}
