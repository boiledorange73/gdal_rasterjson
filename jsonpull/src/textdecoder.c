#ifndef _TEXTDECODER_C_
#define _TEXTDECODER_C_

#include <wchar.h>
#include <stdlib.h> /* malloc */

#ifdef _TEXTDECODER_INTERNAL_
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

#ifndef _TEXTDECODER_EXTERN
#  ifdef _TEXTDECODER_INTERNAL_
#    define _TEXTDECODER_EXTERN static
#  else
#    define _TEXTDECODER_EXTERN
#  endif
#endif
#include "textdecoder.h"

/* macors for malloc and free */
#ifndef MALLOC
#define MALLOC(p) (malloc((p)))
#endif
#ifndef FREE
#define FREE(p) (free((p)))
#endif

TextDecoder *TextDecoder_New(char *decoder_map, int max_mbchars, wchar_t (*fn_decode)(const unsigned char *, int, wchar_t *)) {
  TextDecoder *tdr;
  tdr = (TextDecoder *)MALLOC(sizeof(TextDecoder));
  if( tdr == NULL ) {
    print_perror("malloc");
    return NULL;
  }

  tdr->mbchars = (unsigned char *)MALLOC(sizeof(unsigned char) * (max_mbchars+1));
  if( tdr->mbchars == NULL ) {
    print_perror("malloc");
    FREE(tdr);
    return NULL;
  }

  tdr->wc_buff = DataList_New(4096, sizeof(wchar_t));
  if( tdr->wc_buff == NULL ) {
    FREE(tdr->mbchars);
    FREE(tdr);
    return NULL;
  }

  tdr->decoder_map = decoder_map;
  tdr->max_mbchars = max_mbchars;
  tdr->fn_decode = fn_decode;

  tdr->mbchars_ptr = NULL;
  tdr->mbchars_tail = NULL;

  tdr->hide_error_message = 0;

  return tdr;
}

void TextDecoder_Free(TextDecoder *tdr) {
  DataList_Free(tdr->wc_buff);
  FREE(tdr->mbchars);
  FREE(tdr);
}

void TextDecoder_Clear(TextDecoder *tdr) {
  if( tdr->wc_buff != NULL ) {
    DataList_Clear(tdr->wc_buff);
  }
}

int TextDecoder_Append(TextDecoder *tdr, const unsigned char *src, int count) {
  int ret = 0;
  unsigned char *tail;
  tail = (unsigned char *)src + count;

  while( src < tail ) {
    int ch = 0xFF & *src;
    if( tdr->mbchars_ptr == NULL ) {
      /* not in mbchar sequence */
      int ccd;
      ccd = tdr->decoder_map[ch];
      if( ccd > 0 ) {
        /* single character */
        wchar_t wc;
        wc = (wchar_t)ch;
        ret += DataList_Push(tdr->wc_buff, &wc, 1);
      }
      else if( ccd < 0 ) {
        /* into mbchar sequence */
        int len = -ccd;
        if( len >= 0 && len <= tdr->max_mbchars ) {
          tdr->mbchars_ptr = tdr->mbchars;
          tdr->mbchars_tail = tdr->mbchars + (-ccd);
        }
        else {
          if( !(tdr->hide_error_message) ) {
            print_error("Sequence length by decoder_map excesses max_mbchars. Check decoder_map.\n");
          }
        }
      }
      else {
        if( !(tdr->hide_error_message) ) {
          print_error("Unknown character\n");
        }
      }
    }
    /* sequence process if in mbchar seq newly or already */
    if( tdr->mbchars_ptr != NULL ) {
      /* in mbchar seq */
      *((tdr->mbchars_ptr)++) = ch;
      if( tdr->mbchars_ptr >= tdr->mbchars_tail ) {
        /* end of mbchar sequence */
        int res;
        wchar_t wc;
        *(tdr->mbchars_tail) = '\0';
        res = tdr->fn_decode(tdr->mbchars, tdr->mbchars_tail-tdr->mbchars, &wc);
        if( res > 0 ) {
          ret += DataList_Push(tdr->wc_buff, &wc, 1);
        }
        else {
          if( !(tdr->hide_error_message) ) {
            print_error("Invalid multibyte character sequence.\n");
          }
        }
        tdr->mbchars_ptr = NULL;
        tdr->mbchars_tail = NULL;
      }
    }
    /* end of iteration */
    src++;
  }
  return ret;
}

int TextDecoder_Get(TextDecoder *tdr, wchar_t *dst, int count) {
  return DataList_Shift(tdr->wc_buff, dst, count);
}

int TextDecoder_Unget(TextDecoder *tdr, wchar_t *src, int count) {
  return DataList_Unshift(tdr->wc_buff, src, count);
}

#endif /* textdecoder.c */

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
// cc -g printerror.c datalistcell.c datalist.c textdecoder.c
int main(int ac, char *av[]) {
  char *text = "ほaげ\n";
  TextDecoder *tdr;

  tdr = TextDecoder_New(utf8_decoder_map, 4, utf8_fn_decode);
  TextDecoder_Append(tdr, text, strlen(text));
  int n = 0;
  while( tdr->wc_buff->length > 0 ) {
    wchar_t wc;
    TextDecoder_Get(tdr, &wc, 1);
    if( n == 2 ) {
      TextDecoder_Unget(tdr, &wc, 1);
      printf("unshift at %d\n", n);
    }
    printf("%d: %04x\n", n, wc);
    n++;
  }
  TextDecoder_Free(tdr);
  return 0;
}
*/
