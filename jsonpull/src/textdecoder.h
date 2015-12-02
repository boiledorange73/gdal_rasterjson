#ifndef _TEXTDECODER_HEAD_
#define _TEXTDECODER_HEAD_

#ifndef _TEXTDECODER_EXTERN
#define _TEXTDECODER_EXTERN extern
#endif

#include "datalist.h"
#include <wchar.h>

struct _TextDecoder {
  char *decoder_map;
  int max_mbchars;
  unsigned char *mbchars;
  unsigned char *mbchars_ptr;
  unsigned char *mbchars_tail;
  DataList *wc_buff;
  wchar_t (*fn_decode)(const unsigned char *mbchars, int count, wchar_t *wp);
  int hide_error_message;
};

typedef struct _TextDecoder TextDecoder;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

_TEXTDECODER_EXTERN TextDecoder *TextDecoder_New(char *decoder_map, int max_mbchars, wchar_t (*fn_decode)(const unsigned char *mbstr, int mbchars, wchar_t *wp));
_TEXTDECODER_EXTERN void TextDecoder_Free(TextDecoder *tdr);
_TEXTDECODER_EXTERN void TextDecoder_Clear(TextDecoder *tdr);
_TEXTDECODER_EXTERN int TextDecoder_Append(TextDecoder *tdr, const unsigned char *src, int count);
_TEXTDECODER_EXTERN int TextDecoder_Get(TextDecoder *tdr, wchar_t *dst, int count);
_TEXTDECODER_EXTERN int TextDecoder_Unget(TextDecoder *tdr, wchar_t *src, int count);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* textdecoder.h */
