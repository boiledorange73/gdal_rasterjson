#ifndef _TEXTENCODER_C_
#define _TEXTENCODER_C_

#include <wchar.h>
#include <stdlib.h> /* malloc */
#include <stdio.h> /* FILE */
#include <string.h> /* strlen */
#include <limits.h>

#ifdef _TEXTENCODER_INTERNAL_
#  ifndef _DATALIST_INTERNAL_
#    define _DATALIST_INTERNAL_
#  endif
#  include "datalist.c"
#  ifndef _PRINTERROR_INTERNAL_
#    define _PRINTERROR_INTERNAL_
#  endif
#  include "printerror.c"
#else
#  include "printerror.h"
#  include "datalist.h"
#endif

#ifndef _TEXTENCODER_EXTERN
#  ifdef _TEXTENCODER_INTERNAL_
#    define _TEXTENCODER_EXTERN static
#  else
#    define _TEXTENCODER_EXTERN
#  endif
#endif
#include "textencoder.h"

/* macors for malloc and free */
#ifndef MALLOC
#define MALLOC(p) (malloc((p)))
#endif
#ifndef FREE
#define FREE(p) (free((p)))
#endif

/* macro */
#ifndef _TEXTENCODER_PUTCHAR
#define _TEXTENCODER_PUTCHAR(c,fp) (fputc((c),(fp)))
#endif

TextEncoder *TextEncoder_New(int max_mbchars, int (*fn_encode)(wchar_t, unsigned char *, int)) {
  TextEncoder *ter;
  ter = (TextEncoder *)MALLOC(sizeof(TextEncoder));
  if( ter == NULL ) {
    print_perror("malloc");
    return NULL;
  }
  ter->mbchars = NULL;
  ter->mb_buff = NULL;
  ter->fn_encode = NULL;

  ter->mb_buff = DataList_New(4096, sizeof(unsigned char));
  if( ter->mb_buff == NULL ) {
    TextEncoder_Free(ter);
    return NULL;
  }

  ter->max_mbchars = max_mbchars;
  ter->mbchars = (unsigned char *)MALLOC(sizeof(unsigned char) * (max_mbchars + 1));
  if( ter->mbchars == NULL ) {
    print_perror("malloc");
    TextEncoder_Free(ter);
    return NULL;
  }
  ter->fn_encode = fn_encode;
  return ter;
}

void TextEncoder_Free(TextEncoder *ter) {
  if( ter->mbchars ) {
    FREE(ter->mbchars);
  }
  if( ter->mb_buff ) {
    DataList_Free(ter->mb_buff);
  }
  FREE(ter);
}

int TextEncoder_Append(TextEncoder *ter, const wchar_t *wp, int count) {
  int ret = 0;
  while( count > 0 ) {
    int encoded_count;
    encoded_count = ter->fn_encode(*wp, ter->mbchars, ter->max_mbchars);
    if( encoded_count > 0 ) {
      int pushed_count;
      pushed_count = DataList_Push(ter->mb_buff, ter->mbchars, encoded_count);
      ret += pushed_count;
    }
    wp++;
    count--;
  }
  return ret;
}

int TextEncoder_AppendZ(TextEncoder *ter, const wchar_t *wp) {
  int ret = 0;
  while( *wp != L'\0' ) {
    int encoded_count;
    encoded_count = ter->fn_encode(*wp, ter->mbchars, ter->max_mbchars);
    if( encoded_count > 0 ) {
      int pushed_count;
      pushed_count = DataList_Push(ter->mb_buff, ter->mbchars, encoded_count);
      ret += pushed_count;
    }
    wp++;
  }
  return ret;
}

int TextEncoder_AppendChar(TextEncoder *ter, wchar_t c) {
  return TextEncoder_Append(ter, &c, 1);
}

int TextEncoder_AppendCharRepeat(TextEncoder *ter, wchar_t c, int repeat) {
  int ret = 0;
  while( repeat > 0 ) {
    ret += TextEncoder_Append(ter, &c, 1);
    repeat--;
  }
  return ret;
}

int TextEncoder_Get(TextEncoder *ter, unsigned char *dst, int count) {
  return DataList_Shift(ter->mb_buff, dst, count);
}

int TextEncoder_Unget(TextEncoder *ter, const unsigned char *src, int count) {
  return DataList_Unshift(ter->mb_buff, (void *)src, count);
}

int TextEncoder_FlashToFile(TextEncoder *ter, _TEXTENCODER_FILE *fp) {
  int ret = 0;
  unsigned char c;
  while( ter->mb_buff->length > 0 ) {
    if( TextEncoder_Get(ter, &c, 1) == 1 ) {
      if( _TEXTENCODER_PUTCHAR(c, fp) < 0 ) {
        return ret;
      }
      ret++;
    }
  }
  return ret;
}

#endif /* textencoder.c */

/* ---------------------------------------------------------------- test */
/*
static char utf8_decoder_map[] = {
  0,   1,   2,   3,   4,   5,   6,   7,
  8,   9,  10,  11,  12,  13,  14,  15,
 16,  17,  18,  19,  20,  21,  22,  23,
 24,  25,  26,  27,  28,  29,  30,  31,
 32,  33,  34,  35,  36,  37,  38,  39,
 40,  41,  42,  43,  44,  45,  46,  47,
 48,  49,  50,  51,  52,  53,  54,  55,
 56,  57,  58,  59,  60,  61,  62,  63,
 64,  65,  66,  67,  68,  69,  70,  71,
 72,  73,  74,  75,  76,  77,  78,  79,
 80,  81,  82,  83,  84,  85,  86,  87,
 88,  89,  90,  91,  92,  93,  94,  95,
 96,  97,  98,  99, 100, 101, 102, 103,
104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119,
120, 121, 122, 123, 124, 125, 126, 127,
  0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,  -2,  -2,  -2,  -2,  -2,  -2,
 -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,
 -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,
 -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,
 -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,
 -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,
 -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,
  0,   0,   0,   0,   0,   0,   0,   0
};


static int utf8_fn_decode(unsigned char *cp, int bytes, wchar_t *wp) {
  switch( bytes ) {
  case 2:
    *wp = ((cp[0] & 0x1F) << 6) | (0x3F & cp[1]);
    return 1;
  case 3:
    *wp = ((cp[0] & 0x0F) << 12) | ((0x3F & cp[1]) << 6) | (0x3F & cp[2]);
    return 1;
  case 4:
    // 4 bytes for UTF-32
    if( !(sizeof(wchar_t) >= 3) ) {
      return 0;
    }
    *wp = ((cp[0] & 0x07) << 18) | ((0x3F & cp[1]) << 12) | ((0x3F & cp[2]) << 6) | (0x3F & cp[3]);
    return 1;
  }
  return 0;
}

static int utf8_fn_encode(wchar_t wc, unsigned char *cp, int maxclen) {
  if( wc >= 0 && wc <= 0x7F ) {
    // 1 byte
    if( !(maxclen >= 1) ) {
      return 0;
    }
    *cp = 0x00FF & wc;
    return 1;
  }
  if( wc >= 0x80 && wc <= 0x7FF ) {
    // 2 bytes
    if( !(maxclen >= 2) ) {
      return 0;
    }
    *(cp++) = 0xFF & (0xC0 | ((wc & 0x07C0) >> 6) );
    *(cp++) = 0xFF & (0x80 | ((wc & 0x003F)) );
    return 2;
  }
  if( wc >= 0x800 && wc <= 0xFFFF ) {
    // 3 bytes
    if( !(maxclen >= 3) ) {
      return 0;
    }
    *(cp++) = 0xFF & (0xE0 | ((wc & 0xF000) >> 12) );
    *(cp++) = 0xFF & (0x80 | ((wc & 0x0FC0) >> 6) );
    *(cp++) = 0xFF & (0x80 | ((wc & 0x003F)) );
    return 3;
  }
  if( wc >= 0x10000 && wc <= 0x1FFFF ) {
    if( sizeof(wchar_t) >= 3 ) {
      // 4 bytes for UTF-32
      *(cp++) = 0xFF & (0xF0 | ((wc & 0x001C0000) >> 18) );
      *(cp++) = 0xFF & (0x80 | ((wc & 0x0003F000) >> 12) );
      *(cp++) = 0xFF & (0x80 | ((wc & 0x00000FC0) >> 6) );
      *(cp++) = 0xFF & (0x80 | ((wc & 0x0000003F)) );
    }
  }
  return 0;
}

#include <stdio.h>
// cc -g printerror.c datalistcell.c datalist.c textencoder.c -lm
int main(int ac, char *av[]) {
  static wchar_t *text = L"ほaげ\n";
  TextEncoder *ter;
  char ch;
  int res;

  ter = TextEncoder_New(4, utf8_fn_encode);

//  res = TextEncoder_AppendF(ter, L"Hello %s さん. This is test.\npi=%.14e\n", L"やまだ", (double)3.14159265);
//  res = TextEncoder_AppendF(ter, L"%c%.14e %f\n", L'ご', (double)3.14159265, (double)31.4159265e4);
  res = TextEncoder_AppendF(ter, L"%1 ほ\n", 1);
  printf("Returns %d\n", res);

  for( int n = 0; n < ter->mb_buff->length; n++ ) {
    unsigned char *cp = DataList_Get(ter->mb_buff, n);
    fputc(*cp, stdout);
  }
  fputc('\n', stdout);

  TextEncoder_Free(ter);
  return 0;
}
*/

