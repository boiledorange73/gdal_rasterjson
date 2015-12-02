#ifndef _PRINTERROR_C_
#define _PRINTERROR_C_

#include <stdio.h>
#include <stdarg.h>

#ifndef _PRINTERROR_EXTERN
#  ifdef _PRINTERROR_INTERNAL_
#    define _PRINTERROR_EXTERN static
#  else
#    define _PRINTERROR_EXTERN
#  endif
#endif
#include "printerror.h"

/**
 * Prints error message and system error message.
 * @param msg Message.
 */
void print_perror(const char *msg) {
  perror(msg);
}

/**
 * Prints error message.
 * @param msg Message.
 */
void print_error(const char *msg) {
  fputs(msg, stderr);
}

/**
 * Prints error message and new line.
 * @param msg Message.
 */
void print_errorln(const char *msg) {
  fputs(msg, stderr);
  fputs("\n", stderr);
}

/**
 * Prints formatted error message.
 * @param fmt Format
 */
void print_errorf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
}

#endif /* printerror.c */
