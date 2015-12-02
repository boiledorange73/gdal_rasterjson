#ifndef _TEXTENCODING_HEAD_
#define _TEXTENCODING_HEAD_

#ifndef _TEXTENCODING_EXTERN
#define _TEXTENCODING_EXTERN extern
#endif

#include "textencoder.h"
#include "textdecoder.h"

#include <wchar.h>


struct _TextEncoding {
  char *decoder_map;
  int max_mbchars;
  int (*fn_encode)(wchar_t wc, unsigned char *mb, int max_mbchars);
  wchar_t (*fn_decode)(const unsigned char *mbstr, int mbchars, wchar_t *wp);
};

typedef struct _TextEncoding TextEncoding;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

_TEXTENCODING_EXTERN TextEncoding *TextEncoding_New(
  char *decoder_map,
  int max_mbchars,
  int (*fn_encode)(wchar_t wc, unsigned char *mb, int max_mbchars),
  wchar_t (*fn_decode)(const unsigned char *mbstr, int mbchars, wchar_t *wp)
);
_TEXTENCODING_EXTERN void TextEncoding_Free(TextEncoding *te);
_TEXTENCODING_EXTERN TextEncoder *TextEncoding_NewEncoder(TextEncoding *te);
_TEXTENCODING_EXTERN TextDecoder *TextEncoding_NewDecoder(TextEncoding *te);
_TEXTENCODING_EXTERN TextEncoding *TextEncoding_New_UTF8();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* textencoding.h */
