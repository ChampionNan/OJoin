#pragma once

#include <stdlib.h>
#include <assert.h>

#if defined(__cplusplus)
extern "C" {
#endif

void printf(const char *fmt, ...);
// TODO: void process_input(char *buf, size_t len);
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#if defined(__cplusplus)
}
#endif
