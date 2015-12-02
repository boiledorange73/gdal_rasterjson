#ifndef _TEXTENCODER_HEAD_
#define _TEXTENCODER_HEAD_

#ifndef _TEXTENCODER_EXTERN
#define _TEXTENCODER_EXTERN extern
#endif

#include "datalist.h"
#include <wchar.h>
#include <stdio.h>

struct _TextEncoder {
  int max_mbchars;
  unsigned char *mbchars;
  DataList *mb_buff;
  int (*fn_encode)(wchar_t wc, unsigned char *mb, int max_mbchars);
};

typedef struct _TextEncoder TextEncoder;

#ifndef _TEXTENCODER_FILE
#define _TEXTENCODER_FILE FILE
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

_TEXTENCODER_EXTERN TextEncoder *TextEncoder_New(int max_mbchars, int (*fn_encode)(wchar_t wc, unsigned char *dst, int count));
_TEXTENCODER_EXTERN void TextEncoder_Free(TextEncoder *ter);
_TEXTENCODER_EXTERN int TextEncoder_Append(TextEncoder *ter, const wchar_t *wp, int count);
_TEXTENCODER_EXTERN int TextEncoder_AppendZ(TextEncoder *ter, const wchar_t *wp);
_TEXTENCODER_EXTERN int TextEncoder_AppendChar(TextEncoder *ter, wchar_t c);
_TEXTENCODER_EXTERN int TextEncoder_AppendCharRepeat(TextEncoder *ter, wchar_t c, int repeat);
_TEXTENCODER_EXTERN int TextEncoder_Get(TextEncoder *ter, unsigned char *dst, int count);
_TEXTENCODER_EXTERN int TextEncoder_Unget(TextEncoder *ter, const unsigned char *src, int count);
_TEXTENCODER_EXTERN int TextEncoder_FlashToFile(TextEncoder *ter, _TEXTENCODER_FILE *fp);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* textencoder.h */
