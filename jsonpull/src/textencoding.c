#ifndef _TEXTENCODING_C_
#define _TEXTENCODING_C_

#include <wchar.h>
#include <stdio.h> /* fputs, FILE */
#include <stdlib.h> /* malloc */

#ifdef _TEXTENCODING_INTERNAL_
#  ifndef _TEXTDECODER_INTERNAL_
#    define _TEXTDECODER_INTERNAL_
#  endif
#  include "textdecoder.c"
#  ifndef _TEXTENCODER_INTERNAL_
#    define _TEXTENCODER_INTERNAL_
#  endif
#  include "textencoder.c"
#  ifndef _DATALIST_INTERNAL_
#    define _DATALIST_INTERNAL_
#  endif
#  include "datalist.c"
#  ifndef _PRINTERROR_INTERNAL_
#    define _PRINTERROR_INTERNAL_
#  endif
#  include "printerror.c"
#else
#  include "textdecoder.h"
#  include "textencoder.h"
#  include "printerror.h"
#  include "datalist.h"
#endif

#ifndef _TEXTENCODING_EXTERN
#  ifdef _TEXTENCODING_INTERNAL_
#    define _TEXTENCODING_EXTERN static
#  else
#    define _TEXTENCODING_EXTERN
#  endif
#endif
#include "textencoding.h"

/* macors for malloc and free */
#ifndef MALLOC
#define MALLOC(p) (malloc((p)))
#endif
#ifndef FREE
#define FREE(p) (free((p)))
#endif

TextEncoding *TextEncoding_New(
  char *decoder_map,
  int max_mbchars,
  int (*fn_encode)(wchar_t wc, unsigned char *mb, int max_mbchars),
  wchar_t (*fn_decode)(const unsigned char *mbstr, int mbchars, wchar_t *wp)
) {
  TextEncoding *te;
  te = (TextEncoding *)MALLOC(sizeof(TextEncoding));
  if( te == NULL ) {
    print_perror("malloc");
    return NULL;
  }

  if( !(max_mbchars > 0) ) {
    int n;
    max_mbchars = 0;
    for( n = 0; n < 256; n++ ) {
      int ccd;
      ccd = decoder_map[n];
      if( ccd < 0 ) {
        if( -ccd > max_mbchars ) {
          max_mbchars = -ccd;
        }
      }
    }
  }

  te->decoder_map = decoder_map;
  te->max_mbchars = max_mbchars;
  te->fn_encode = fn_encode;
  te->fn_decode = fn_decode;

  return te;
}

void TextEncoding_Free(TextEncoding *te) {
  FREE(te);
}

TextEncoder *TextEncoding_NewEncoder(TextEncoding *te) {
  return TextEncoder_New(te->max_mbchars, te->fn_encode);
}

TextDecoder *TextEncoding_NewDecoder(TextEncoding *te) {
  return TextDecoder_New(te->decoder_map, te->max_mbchars, te->fn_decode);
}

/* ---------------------------------------------------------------- UTF8 */
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


static wchar_t utf8_fn_decode(const unsigned char *cp, int bytes, wchar_t *wp) {
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
    /* 1 byte */
    if( !(maxclen >= 1) ) {
      return 0;
    }
    *cp = 0x00FF & wc;
    return 1;
  }
  if( wc >= 0x80 && wc <= 0x7FF ) {
    /* 2 bytes */
    if( !(maxclen >= 2) ) {
      return 0;
    }
    *(cp++) = 0xFF & (0xC0 | ((wc & 0x07C0) >> 6) );
    *(cp++) = 0xFF & (0x80 | ((wc & 0x003F)) );
    return 2;
  }
  if( wc >= 0x800 && wc <= 0xFFFF ) {
    /* 3 bytes */
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
      /* 4 bytes for UTF-32 */
      *(cp++) = 0xFF & (0xF0 | ((wc & 0x001C0000) >> 18) );
      *(cp++) = 0xFF & (0x80 | ((wc & 0x0003F000) >> 12) );
      *(cp++) = 0xFF & (0x80 | ((wc & 0x00000FC0) >> 6) );
      *(cp++) = 0xFF & (0x80 | ((wc & 0x0000003F)) );
    }
  }
  return 0;
}

TextEncoding *TextEncoding_New_UTF8() {
  return TextEncoding_New(utf8_decoder_map, 4, utf8_fn_encode, utf8_fn_decode);
}

#endif /* textencoding.c */

/* ---------------------------------------------------------------- test */
/*
// cc -g printerror.c datalistcell.c datalist.c textencoder.c textdecoder.c textencoding.c
int main(int ac, char *av[]) {
  static unsigned char *text = "ほaげ\n";
  TextEncoding *te;
  TextEncoder *ter;
  TextDecoder *tdr;
  char ch;
  wchar_t wc;

  te = TextEncoding_New_UTF8();
  tdr = TextEncoding_NewDecoder(te);
  ter = TextEncoding_NewEncoder(te);
  
  TextDecoder_Append(tdr, text, strlen(text));
  while( tdr->wc_buff->length > 0 ) {
    if( TextDecoder_Get(tdr, &wc, 1) > 0 ) {
      TextEncoder_Append(ter, &wc, 1);
    }
  }
  while( ter->mb_buff->length > 0 ) {
    if( TextEncoder_Get(ter, &ch, 1) > 0 ) {
      fputc(ch, stdout);
    }
  }
  return 0;
}
*/
