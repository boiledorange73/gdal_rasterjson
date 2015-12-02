#ifndef _PRINTERROR_HEAD_
#define _PRINTERROR_HEAD_

#ifndef _PRINTERROR_EXTERN
#define _PRINTERROR_EXTERN extern
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

_PRINTERROR_EXTERN void print_perror(const char *msg);
_PRINTERROR_EXTERN void print_error(const char *msg);
_PRINTERROR_EXTERN void print_errorln(const char *msg);
_PRINTERROR_EXTERN void print_errorf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* printerror.h */
