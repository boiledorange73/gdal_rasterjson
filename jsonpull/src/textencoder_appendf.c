#ifndef _TEXTENCODER_APPENDF_C_
#define _TEXTENCODER_APPENDF_C_

/* DOES NOT support long double fully (lessy converting to double). */
/* DOES NOT support non-IEEE754 double/float binary. */

#include <wchar.h>
#include <stdlib.h> /* malloc */
#include <stdio.h> /* FILE */
#include <limits.h>
#include <stdint.h> /* uint*_t */

#ifdef _TEXTENCODER_APPENDF_INTERNAL_
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
#  include "printerror.h"
#  include "datalist.h"
#endif

#ifndef _TEXTENCODER_APPENDF_EXTERN
#  ifdef _TEXTENCODER_APPENDF_INTERNAL_
#    define _TEXTENCODER_APPENDF_EXTERN static
#  else
#    define _TEXTENCODER_APPENDF_EXTERN
#  endif
#endif
#include "textencoder_appendf.h"

/* macors for malloc and free */
#ifndef MALLOC
#define MALLOC(p) (malloc((p)))
#endif
#ifndef FREE
#define FREE(p) (free((p)))
#endif

#include <float.h>
/* real_t */
typedef double real_t;
#define REAL_EPSILON DBL_EPSILON
#define EXP_R ((real_t)1.0e-16)

/* Wheterh More Than 0 */
#define MT0F(v) ((real_t)(v) > (REAL_EPSILON))
/* Wheterh Less Than 0 */
#define LT0F(v) ((real_t)(v) < -(REAL_EPSILON))
/* Wheterh EQuals 0 */
#define EQ0F(v) ((real_t)(v) >= (-(REAL_EPSILON)) && (v) <= (REAL_EPSILON))

#define REAL_



/* ---------------------------------------------------------------- printf */
// %([0-9]+$)?([+- 0#]*[0-9]+(\.[0-9]+)?)?[LENGTH]?[TYPE]
// int parameter ... [0-9]+$
// int flag ... [+- 0#]
// int fieldwidth
// int precision (\.[0-9]+)
// int lengthmodifier ... LENGTH (hh h l ll L z j t)
// int conversion ... TYPE (d/i u f F e E g G x X o s c p a A n %)

static wchar_t hex_upper[] = L"0123456789ABCDEF";
static wchar_t hex_lower[] = L"0123456789abcdef";



/* ------------------------------------------------ printf/commons */
#define LBE  ((real_t)1.442695040888963)
#define LB10 ((real_t)3.321928094887362)
#define LN10 ((real_t)2.302585092994046)


static real_t mylogb(real_t x, int lb_prec) {
    int inv; /* 1 if x < 1 */
    real_t r, ri, rf; /* returned value, integer part of r, fraction part of r */
    real_t pow2_ri; /* pow(2,ri) */
    real_t xf; /* x/pow(2,ri) */
    real_t frf; /* factor for rf */

    if (x < 1) {
        x = 1.0 / x;
        inv = 1;
    }
    else {
        inv = 0;
    }
 
    /* calculates integer part */
    for (ri = 0, pow2_ri = 1; pow2_ri * 2 <= x; ri++, pow2_ri *= 2) {
        /* does nothing*/
    }
    /* calcualtes fraction part */
    xf = x / pow2_ri;
    frf = 0.5;
    rf = 0;
    while (lb_prec > 0) {
        xf = xf * xf;
        if (xf >= 2) {
            xf = 0.5 * xf;
            rf = rf + frf;
        }
        frf = 0.5 * frf;
        lb_prec--;
    }
    r = ri + rf;
    return inv ? -r : r;
}

static real_t myln(real_t x) {
    return mylogb(x, 60) / LBE;
}

static real_t mylog10(real_t x) {
    return mylogb(x, 60) / LB10;
}

static real_t myexp(real_t x) {
  real_t r, a;
  int n;
  r = 1;
  a = 1;
  for( n = 1; n < 1000; n++ ) {
    a = a * x /(real_t)n;
    r = r + a;
    if( -EXP_R < a && a < EXP_R ) {
      break;
    }
  }
  return r;
}

static real_t mypow10(real_t x) {
  return myexp(x*LN10);
}

static int sysis_little_endian() {
  uint16_t n;
  n = 1;
  return !!(*((char *)(&n))); /* if first bytes it not zero, little endian */
}

static int myfloor(real_t v) {
  real_t absr;
  real_t absv;
  int neg;
  if( v < 0 ) {
    neg = 1;
    absv = -v;
  }
  else {
    neg = 0;
    absv = v;
  }
  if( absv < (real_t)INT_MAX ) {
    absr = (real_t)((int)absv);
  }
  else {
    real_t fct;
    int cnt;
    for (cnt = 0, fct = 1.0; fct < absv; cnt++, fct *= 10.0) {
        /* DOES NOTHING */
    }
    absr = 0.0;
    while (cnt > 0) {
        real_t v1;
        fct = 0.1 * fct;
        v1 = (real_t)((int)(absv / fct)) * fct;
        absr = absr + v1;
        absv = absv - v1;
        cnt--;
    }
  }
  return neg ? -absr : absr;
}

/* 0 - normal, 1 - nan, 2 - inf */
static int my_invalid_float(real_t x) {
  switch(sizeof(x)) {
  case 2:  /*  16bits 1, 5, 10 */
    {
      uint16_t *pui16;
      pui16 = (uint16_t *)(&x);
      if((*pui16 & 0x7C00) == 0x7C00) {
        return  ((*pui16 & 0x03FF) != 0) ? 1 : 2;
      }
    }
    return 0;
  case 4:  /*  32bits 1, 8, 23 */
    {
      uint32_t *pui32;
      pui32 = (uint32_t *)(&x);
      if((*pui32 & 0x7F800000) == 0x7F800000) {
        return ((*pui32 & 0x007FFFFF) != 0) ? 1 : 2;
      }
    }
    return 0;
  case 8:  /*  64bits 1,11, 52 */
    {
      uint64_t *pui64;
      pui64 = (uint64_t *)(&x);
      if((*pui64 & 0x7FF0000000000000) == 0x7FF0000000000000) {
        return ((*pui64 & 0x000FFFFFFFFFFFFF) != 0) ? 1 : 2;
      }
    }
    return 0;
  case 16: /* 128bits 1,15,112 */
    {
      uint64_t *pui64u;
      uint64_t *pui64l;
      if( sysis_little_endian() ) {
        pui64u = (uint64_t *)((char *)(&x) + 8);
        pui64l = (uint64_t *)(&x);
      }
      else {
        pui64u = (uint64_t *)(&x);
        pui64l = (uint64_t *)((char *)(&x) + 8);
      }
      if( (*pui64u & 0x7FFF000000000000) == 0x7FFF000000000000 ) {
        return ((*pui64l & 0xFFFFFFFFFFFFFFFF) != 0) ? 1 : 2;
      }
    }
    return 0;
  default:
    return 0;
  }
}

static int myisnan(real_t v){
  return my_invalid_float(v) == 1;
}

static int myisinf(real_t v){
  return my_invalid_float(v) == 2;
}

/*
 *
 * flag
 * fieldwidth
 * vlen
 * pllen - Pointer of left SPACE length.
 * pzlen - Pointer of left "0" length.
 * prlen - Pointer of right SPACE length.
 */
static void calculate_length(int flag, int fieldwidth, int vlen, int *pllen, int *pzlen, int *prlen) {
  *pllen = 0;
  if( pzlen != NULL ) {
    *pzlen = 0;
  }
  *prlen = 0;
  if( fieldwidth > 0 ) {
    if( pzlen != NULL && (flag & APPENDF_F_ZERO) ) {
      /* zero padding */
      *pzlen = fieldwidth - vlen;
      if( *pzlen < 0 ) {
          *pzlen = 0;
      }
    }
    else if( flag & APPENDF_F_MINUS ) {
      /* left alignment */
      *prlen = fieldwidth - vlen;
      if( *prlen < 0 ) {
        *prlen = 0;
      }
    }
    else {
      /* right alignment */
      *pllen = fieldwidth - vlen;
      if( *pllen < 0 ) {
        *pllen = 0;
      }
    }
  }
  /* If fieldwidth is not positive,
   * does nothing. (all p?len are set by zero).*/
}

/* Appends spaces. */
static int appendF_space(TextEncoder *ter, int count) {
  int ret = 0;
  while( count > 0 ) {
    ret += TextEncoder_AppendChar(ter, L' ');
    count--;
  }
  return ret;
}

/* Appends "0". */
static int appendF_zeropadding(TextEncoder *ter, int count) {
  int ret = 0;
  while( count > 0 ) {
    ret += TextEncoder_AppendChar(ter, L'0');
    count--;
  }
  return ret;
}

/* Appends one char. if sign is '\0', does nothing. */
static int appendF_sign(TextEncoder *ter, wchar_t sign) {
  if( sign == L'\0' ) {
    return 0;
  }
  return TextEncoder_AppendChar(ter, sign);
}

/* ------------------------------------------------ printf/string */

static int appendF_s(TextEncoder *ter, const wchar_t *value, int flag, int fieldwidth, size_t valuelen) {
  int leftlen;
  int rightlen;
  int ret = 0;

  if( value == NULL ) {
    value = L"(null)";
    valuelen = 6;
  }

  calculate_length(flag, fieldwidth, valuelen, &leftlen, NULL, &rightlen);

  /* left */
  ret += appendF_space(ter, leftlen);
  /* value */
  ret += TextEncoder_Append(ter, value, valuelen);
  /* right */
  ret += appendF_space(ter, rightlen);
  return ret;
}


/* ------------------------------------------------ printf/char */

static int appendF_c(TextEncoder *ter, wchar_t value, int flag, int fieldwidth) {
  wchar_t text[2];
  text[0] = value;
  text[1] = L'\0';
  return appendF_s(ter, text, flag, fieldwidth, 1);
}


/* ------------------------------------------------ printf/integer */

static int appendF_dig(
    TextEncoder *ter,
    unsigned long long value,
    int flag,
    int fieldwidth,
    int lengthmodifier,
    int conversion, /* APPENDF_C_d or APPENDF_C_u */
    unsigned int base,
    const wchar_t *hex,
    const wchar_t *hashstr,
    wchar_t sign) {
  int diglen;
  int leftlen;
  int zerolen;
  int valuelen;
  int rightlen;
  int n;
  int hashstrlen;
  unsigned long long mask;
  unsigned long long maxvalue;
  int ret = 0;

  /* calculates maxvalue */
  if( conversion == APPENDF_C_u ) {
    /* unsigned */
    if( lengthmodifier == APPENDF_L_ll ) {
      maxvalue = ULLONG_MAX;
    }
    else if( lengthmodifier == APPENDF_L_l ) {
      maxvalue = ULONG_MAX;
    }
    else  {
      maxvalue = UINT_MAX;
    }
  }
  else {
    /* signed */
    if( lengthmodifier == APPENDF_L_ll ) {
      maxvalue = LLONG_MAX;
    }
    else if( lengthmodifier == APPENDF_L_l ) {
      maxvalue = LONG_MAX;
    }
    else  {
      maxvalue = INT_MAX;
    }
  }

  /* counting digits */
  for( diglen = 1, mask = 1; (maxvalue == 0 || maxvalue/mask >= base) && value/mask >= base; diglen++, mask *= base) {
    /* DOES NOTHING */
  }

  if( (flag & APPENDF_F_HASH) && (hashstr != NULL) ) {
    hashstrlen = wcslen(hashstr);
  }
  else {
    hashstrlen = 0;
  }
  valuelen = diglen + hashstrlen;
  if( sign != L'\0' ) {
    valuelen++;
  }

  calculate_length(flag, fieldwidth, valuelen, &leftlen, &zerolen, &rightlen);

  /* left */
  ret += appendF_space(ter, leftlen);
  /* sign */
  ret += appendF_sign(ter, sign);
  /* hash */
  if( hashstrlen > 0 ) {
    ret += TextEncoder_Append(ter, hashstr, hashstrlen);
  }
  /* 0 */
  ret += appendF_zeropadding(ter, zerolen);
  /* dig */
  for( n = 0; n < diglen; n++ ) {
    ret += TextEncoder_AppendChar(ter, hex[(value / mask) % base]);
    mask = mask / base;
  }
  /* right */
  ret += appendF_space(ter, rightlen);
  return ret;
}

static int appendF_d(TextEncoder *ter, long long value, int flag, int fieldwidth, int lengthmodifier) {
  wchar_t sign;

  if( value < 0 ) {
    value = -value;
    sign = L'-';
  }
  else {
    if( flag & APPENDF_F_SPACE ) {
      sign = L' ';
    }
    else if( flag & APPENDF_F_PLUS ) {
      sign = L'+';
    }
    else {
      sign = L'\0';
    }
  }
  return appendF_dig(ter, value, flag, fieldwidth, lengthmodifier, APPENDF_C_d, 10, hex_upper, L"0", sign);
}

static int appendF_u(TextEncoder *ter, unsigned long long value, int flag, int fieldwidth, int lengthmodifier) {
  return appendF_dig(ter, value, flag, fieldwidth, lengthmodifier, APPENDF_C_u, 10, hex_upper, NULL, L'\0');
}


static int appendF_x(TextEncoder *ter, unsigned long long value, int flag, int fieldwidth, int lengthmodifier, int upper) {
  if( upper ) {
    return appendF_dig(ter, value, flag, fieldwidth, lengthmodifier, APPENDF_C_u, 16, hex_upper, L"0X", L'\0');
  }
  else {
    return appendF_dig(ter, value, flag, fieldwidth, lengthmodifier, APPENDF_C_u, 16, hex_lower, L"0x", L'\0');
  }
}

static int appendF_o(TextEncoder *ter, unsigned long long value, int flag, int fieldwidth, int lengthmodifier) {
  return appendF_dig(ter, value, flag, fieldwidth, lengthmodifier, APPENDF_C_u, 8, hex_upper, L"0", L'\0');
}

/* ------------------------------------------------ printf/float */
/* Appends "INF", "NAN" */
static int appendF_signed3chars(TextEncoder *ter, wchar_t sign, const wchar_t *chars, int flag, int fieldwidth) {
  wchar_t text[5];
  wchar_t *p;
  wchar_t *t;

  p = text;
  t = text + 5;
  if( sign != L'\0' ) {
    *(p++) = sign;
  }
  while( p < t && *chars != L'\0' ) {
    *(p++) = *(chars++);
  }
  return appendF_s(ter, text, flag, fieldwidth, p-text);
}

/* Checks value is nan or inf and print string if value is. */
static int appendF_infnan(TextEncoder *ter, wchar_t sign, real_t value, int flag, int fieldwidth, int uppercase) {
  if( myisinf(value) ) {
    if( uppercase ) {
      return appendF_signed3chars(ter, sign, L"INF", flag, fieldwidth);
    }
    else {
      return appendF_signed3chars(ter, sign, L"inf", flag, fieldwidth);
    }
  }

  if( myisnan(value) ) {
    if( uppercase ) {
      return appendF_signed3chars(ter, sign, L"NAN", flag, fieldwidth);
    }
    else {
      return appendF_signed3chars(ter, sign, L"nan", flag, fieldwidth);
    }
  }

  return -1;
}

static int appendF_f(TextEncoder *ter, real_t value, int flag, int fieldwidth, int precision, int upper) {
  wchar_t sign = L'\0';
  int diglen;
  int leftlen;
  int zerolen;
  int valuelen;
  int rightlen;
  int n;
  int ret = 0;
  real_t div;

  if( value < 0 ) {
    value = -value;
    sign = L'-';
  }
  else if( flag & APPENDF_F_PLUS ) {
    sign = L'+';
  }
  else if( flag & APPENDF_F_SPACE ) {
    sign = L' ';
  }

  /* INF/NaN check */
  ret = appendF_infnan(ter, sign, value, flag, fieldwidth, upper);
  if( ret >= 0 ) {
    return ret;
  }

  /* rounds */
/* math:  value = value + 0.5 * pow(10.0, -precision); */
  value = value + 0.5 * mypow10(-precision);
  /* log10(value) + 1 (for '.') + precision */
/* math:  diglen = (value > 1.0 ? (int)log10(value) : 0) + 1; */
  diglen = (value > 1.0 ? (int)mylog10(value) : 0) + 1;
  if( precision > 0 ) {
    /* dot and fraction */
    diglen += 1 + precision;
  }
  else if( flag & APPENDF_F_HASH ) {
    /* dot */
    diglen += 1;
  }
  valuelen = (sign != L'\0' ? 1 : 0) + diglen;

  calculate_length(flag, fieldwidth, valuelen, &leftlen, &zerolen, &rightlen);

  /* left */
  ret += appendF_space(ter, leftlen);
  /* sign */
  ret += appendF_sign(ter, sign);
  /* 0 */
  ret += appendF_zeropadding(ter, zerolen);
  /* dig / int */
/* math:  if( floor(value) == 0.0 ) { */
  if( myfloor(value) == 0.0 ) {
    ret += TextEncoder_AppendChar(ter, L'0');
  }
  else {
/* math:    div = pow(10.0, floor(log10(value))); */
    int digits;
    real_t divf;
    digits = myfloor(mylog10(value));
    divf = mypow10(digits);
    div = mypow10(myfloor(mylog10(value)));
    while( digits >= 0 ) {
      int v1;
      v1 = (int)myfloor(value / divf);
      ret += TextEncoder_AppendChar(ter, hex_upper[v1]);
      value = value - (real_t)v1 * divf;
      divf = divf / 10.0;
      digits--;
    }
  }
  /* dig / dot */
  if( precision > 0 || (flag & APPENDF_F_HASH) ) {
    ret += TextEncoder_AppendChar(ter, L'.');
  }
  /* dig / frac */
  if( precision > 0 ) {
    div = 10.0;
    for( n = 0; n < precision; n++ ) {
      int v1;
      value = value * 10.0;
      v1 = (int)value % 10;
      ret += TextEncoder_AppendChar(ter, hex_upper[v1]);
      value = value - (real_t)v1;
    }
  }
  /* right */
  ret += appendF_space(ter, rightlen);
  return ret;
}

static int appendF_e(TextEncoder *ter, real_t value, int flag, int fieldwidth, int precision, int upper) {
  wchar_t sign = L'\0';
  int diglen;
  int leftlen;
  int zerolen;
  int valuelen;
  int rightlen;
  int n;
  int ret = 0;
  real_t div;

  real_t mantissa;
  int exponent;
  int exponent_abs;
  int exponentlen;
  int exponentdiv;

  /* Negative value check */
  if( value < 0 ) {
    value = -value;
    sign = L'-';
  }
  else if( flag & APPENDF_F_PLUS ) {
    sign = L'+';
  }
  else if( flag & APPENDF_F_SPACE ) {
    sign = L' ';
  }

  /* INF/NaN check */
  ret = appendF_infnan(ter, sign, value, flag, fieldwidth, upper);
  if( ret >= 0 ) {
    return ret;
  }

  /* exponent and mantissa */
  if( EQ0F(value) ) {
    exponent = 0.0;
    mantissa = 0;
  }
  else {
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! uses pow(), floor(), log10() !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
/* math:    exponent = (int)floor(log10(value)); */
/* math:    mantissa = value / pow(10.0, (real_t)exponent); */
    exponent = (int)myfloor(mylog10(value));
    mantissa = value / mypow10((real_t)exponent);
  }

  diglen = 1; /* [0].000000e-00 */
  if( precision > 0 || flag & APPENDF_F_HASH ) {
    diglen++; /* 0[.]000000e-00 */
  }
  diglen += precision; /* 0.[000000]e-00 */
  diglen += 2; /* 0.000000[e-]00 */

  /* 0.000000e-[00] */
  exponent_abs = exponent >= 0 ? exponent : -exponent;
  if( exponent_abs < 100 ) {
    exponentlen = 2;
    exponentdiv = 10;
  }
  else {
    for( exponentlen = 1, exponentdiv = 1; exponent_abs/exponentdiv >= 10; exponentlen++, exponentdiv *= 10 ) {
      /* DOES NOTHING */
    }
  }
  diglen += exponentlen;

  valuelen = (sign != L'\0' ? 1 : 0) + diglen;

  calculate_length(flag, fieldwidth, valuelen, &leftlen, &zerolen, &rightlen);

  /* left */
  ret += appendF_space(ter, leftlen);
  /* sign */
  ret += appendF_sign(ter, sign);
  /* 0 */
  ret += appendF_zeropadding(ter, zerolen);
  /* mnt / int */
/* math:  if( floor(mantissa) == 0.0 ) { */
  if( myfloor(mantissa) == 0.0 ) {
    ret += TextEncoder_AppendChar(ter, L'0');
  }
  else {
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! uses pow(), floor(), log10() !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
/* math:    div = pow(10.0, floor(log10(mantissa))); */
    div = mypow10(myfloor(mylog10(mantissa)));
    while( MT0F(div * 10.0 - 1.0) ) {
      int v1;
/* math:      v1 = (int)floor(mantissa / div); */
      v1 = (int)myfloor(mantissa / div);
      ret += TextEncoder_AppendChar(ter, hex_upper[v1]);
      mantissa = mantissa - (real_t)v1 * div;
      div = div / 10.0;
    }
  }
  /* mnt / dot */
  if( precision > 0 || (flag & APPENDF_F_HASH) ) {
    ret += TextEncoder_AppendChar(ter, L'.');
  }
  /* mnt / frac */
  if( precision > 0 ) {
    div = 10.0;
    for( n = 0; n < precision; n++ ) {
      int v1;
      mantissa = mantissa * 10.0;
      v1 = (int)mantissa % 10;
      ret += TextEncoder_AppendChar(ter, hex_upper[v1]);
      mantissa = mantissa - (real_t)v1;
    }
  }
  /* e+/e- */
  ret += TextEncoder_AppendChar(ter, upper ? L'E' : L'e');
  ret += TextEncoder_AppendChar(ter, exponent < 0 ? L'-' : L'+');
  /* exp */
  for( n = 0; n < exponentlen; n++ ) {
    ret += TextEncoder_AppendChar(ter, hex_upper[(exponent_abs / exponentdiv) % 10]);
    exponentdiv = exponentdiv / 10;
  }
  /* right */
  ret += appendF_space(ter, rightlen);
  return ret;
}

static int appendF_g(TextEncoder *ter, real_t value, int flag, int fieldwidth, int precision, int upper) {
  real_t exponent;
  real_t value_abs;

  value_abs = value < 0 ? -value : value;
/* math:  exponent = MT0F(value_abs) ? (int)log10(value_abs) : 0; */
  exponent = MT0F(value_abs) ? (int)mylog10(value_abs) : 0;
  if( exponent >= 6 || (exponent < 0 && exponent + precision <= 2) ) {
    /* 1.0e+6 1.0e-5 */
    return appendF_e(ter, value, flag, fieldwidth, precision, upper);
  }
  else {
    return appendF_f(ter, value, flag, fieldwidth, precision, upper);
  }
}

/* ------------------------------------------------ printf/others */
static int appendF_p(TextEncoder *ter, void *value, int flag, int fieldwidth, int precision, int lengthmodifier) {
  // TODO: FILL!!!
  return 0;
}

static int appendF_a(TextEncoder *ter, void *value, int flag, int fieldwidth, int precision, int lengthmodifier) {
  // TODO: FILL!!!
  return 0;
}


/* ------------------------------------------------ format analyzer */
#include <stdarg.h>
#include <inttypes.h>
#include <stddef.h>

void AppendFFormatCell_Clear(AppendFFormatCell *cell) {
  cell->conversion = 0;
  cell->flag = 0;
  cell->fieldwidth = 0;
  cell->precision = -1;
  cell->head = NULL;
  cell->tail = NULL;
}

struct _AnalyzedFormat {
  int params;
  DataList *list;
};
typedef struct _AnalyzedFormat AnalyzedFormat;

static AnalyzedFormat *AnalyzedFormat_New() {
  AnalyzedFormat *ret;
  ret = (AnalyzedFormat *)MALLOC(sizeof(AnalyzedFormat));
  if( ret == NULL ) {
    print_perror("malloc");
    return NULL;
  }
  ret->list = DataList_New(16, sizeof(AppendFFormatCell));
  if( ret->list == NULL ) {
    FREE(ret);
    return NULL;
  }
  ret->params = 0;
  return ret;
}

static void AnalyzedFormat_Free(AnalyzedFormat *af) {
  DataList_Free(af->list);
  FREE(af);
}

static int set_flag(wchar_t ch, int *pflag) {
  switch( ch ) {
  case L'+':
    *pflag |= APPENDF_F_PLUS;
    *pflag &= ~APPENDF_F_SPACE;
    return 1;
  case L'-':
    *pflag |= APPENDF_F_MINUS;
    return 1;
  case L' ':
    if( !(*pflag & APPENDF_F_PLUS) ) {
      *pflag |= APPENDF_F_SPACE;
    }
    return 1;
  case L'0':
    *pflag |= APPENDF_F_ZERO;
    return 1;
  case L'#':
    *pflag |= APPENDF_F_HASH;
    return 1;
  default:
    return 0;
  }
}

static int set_number(wchar_t ch, int *pnumber) {
  switch( ch ) {
  case L'0':
    *pnumber = (*pnumber > 0 ? *pnumber * 10 : 0);
    return 1;
  case L'1':
    *pnumber = (*pnumber > 0 ? *pnumber * 10 : 0) + 1;
    return 1;
  case L'2':
    *pnumber = (*pnumber > 0 ? *pnumber * 10 : 0) + 2;
    return 1;
  case L'3':
    *pnumber = (*pnumber > 0 ? *pnumber * 10 : 0) + 3;
    return 1;
  case L'4':
    *pnumber = (*pnumber > 0 ? *pnumber * 10 : 0) + 4;
    return 1;
  case L'5':
    *pnumber = (*pnumber > 0 ? *pnumber * 10 : 0) + 5;
    return 1;
  case L'6':
    *pnumber = (*pnumber > 0 ? *pnumber * 10 : 0) + 6;
    return 1;
  case L'7':
    *pnumber = (*pnumber > 0 ? *pnumber * 10 : 0) + 7;
    return 1;
  case L'8':
    *pnumber = (*pnumber > 0 ? *pnumber * 10 : 0) + 8;
    return 1;
  case L'9':
    *pnumber = (*pnumber > 0 ? *pnumber * 10 : 0) + 9;
    return 1;
  default:
    return 0;
  }
}

static int set_lengthmodifier1(wchar_t ch, int *plengthmodifier) {
  switch( ch ) {
  case L'h':
    *plengthmodifier = APPENDF_L_h;
    return 1;
  case L'l':
    *plengthmodifier = APPENDF_L_l;
    return 1;
  case L'j':
    *plengthmodifier = APPENDF_L_j;
    return 1;
  case L'z':
    *plengthmodifier = APPENDF_L_z;
    return 1;
  case L't':
    *plengthmodifier = APPENDF_L_t;
    return 1;
  case L'L':
    *plengthmodifier = APPENDF_L_L;
    return 1;
  default:
    return 0;
  }
}

static int set_lengthmodifier2(wchar_t ch, int *plengthmodifier) {
  switch( ch ) {
  case L'h':
    if( *plengthmodifier == APPENDF_L_h ) {
      *plengthmodifier = APPENDF_L_hh;
      return 1;
    }
    return 0;
  case L'l':
    if( *plengthmodifier == APPENDF_L_l ) {
      *plengthmodifier = APPENDF_L_ll;
      return 1;
    }
    return 0;
  default:
    return 0;
  }
}

static int set_conversion(wchar_t ch, int *pconversion) {
  switch( ch ) {
  case L'd':
  case L'i':
    *pconversion = APPENDF_C_d;
    return 1;
  case L'u':
    *pconversion = APPENDF_C_u;
    return 1;
  case L'x':
    *pconversion = APPENDF_C_x;
    return 1;
  case L'X':
    *pconversion = APPENDF_C_X;
    return 1;
  case L'o':
    *pconversion = APPENDF_C_o;
    return 1;
  case L'f':
    *pconversion = APPENDF_C_f;
    return 1;
  case L'F':
    *pconversion = APPENDF_C_F;
    return 1;
  case L'e':
    *pconversion = APPENDF_C_e;
    return 1;
  case L'E':
    *pconversion = APPENDF_C_E;
    return 1;
  case L'g':
    *pconversion = APPENDF_C_g;
    return 1;
  case L'G':
    *pconversion = APPENDF_C_G;
    return 1;
  case L'c':
    *pconversion = APPENDF_C_c;
    return 1;
  case L's':
    *pconversion = APPENDF_C_s;
    return 1;
  default:
    return 0;
  }
}


static int AnalyzedFormat_Analyze(AnalyzedFormat *af, const wchar_t *fmt) {
  int st = 0;
  const wchar_t *p, *ph;

  AppendFFormatCell cell;

  AppendFFormatCell_Clear(&cell);

  ph = fmt;
  for( p = fmt; *p != L'\0'; p++ ) {
    if( st == 0 && *p == L'%' ) {
      /* % */
      if( ph < p ) {
        cell.conversion = APPENDF_C_literal;
        cell.head = ph;
        cell.tail = p;
        DataList_Push(af->list, &cell, 1);
        AppendFFormatCell_Clear(&cell);
      }
      ph = p;
      st = 1;
    }
    else if( st == 1 && *p == L'%' ) {
      cell.conversion = APPENDF_C_percent;
      DataList_Push(af->list, &cell, 1);
      AppendFFormatCell_Clear(&cell);
      st = 0;
    }
    else if( st == 0 ) {
      /* DOES NOTHING */
    }
    else if( st <= 2 && set_flag(*p, &(cell.flag)) ) {
      st = 2;
    }
    else if( st <= 3 && set_number(*p, &(cell.fieldwidth)) ) {
      st = 3;
    }
    else if( st <= 3 && *p == L'.' ) {
      st = 4;
    }
    else if( st <= 4 && set_number(*p, &(cell.precision)) ) {
      st = 4;
    }
    else if( st <= 4 && set_lengthmodifier1(*p, &(cell.lengthmodifier)) ) {
      st = 5;
    }
    else if( st == 5 && set_lengthmodifier2(*p, &(cell.lengthmodifier)) ) {
      st = 6;
    }
    else if( st <= 6 && set_conversion(*p, &(cell.conversion)) ) {
      DataList_Push(af->list, &cell, 1);
      AppendFFormatCell_Clear(&cell);
      ph = p + 1;
      st = 0;
      (af->params)++;
    }
    else {
      // TODO: error !!!
      st = 0;
    }
  }
  if( st == 0 ) {
    if( ph < p ) {
      cell.conversion = APPENDF_C_literal;
      cell.head = ph;
      cell.tail = p;
      DataList_Push(af->list, &cell, 1);
      AppendFFormatCell_Clear(&cell);
    }
  }

  return af->params;
}



static long long get_int(va_list *pargs, int lengthmodifier) {
  switch( lengthmodifier ) {
/*
  case APPENDF_L_hh:
    return (long long)va_arg(*pargs, char);
  case APPENDF_L_h:
    return (long long)va_arg(*pargs, short);
*/
  case APPENDF_L_l:
    return (long long)va_arg(*pargs, long);
  case APPENDF_L_ll:
    return (long long)va_arg(*pargs, long long);
  case APPENDF_L_j:
    return (long long)va_arg(*pargs, intmax_t);
  case APPENDF_L_z:
    return (long long)va_arg(*pargs, size_t);
  case APPENDF_L_t:
    return (long long)va_arg(*pargs, ptrdiff_t);
  default:
    return (long long)va_arg(*pargs, int);
  }
}

static unsigned long long get_uint(va_list *pargs, int lengthmodifier) {
  switch( lengthmodifier ) {
/*
  case APPENDF_L_hh:
    return (unsigned long long)va_arg(*pargs, unsigned char);
  case APPENDF_L_h:
    return (unsigned long long)va_arg(*pargs, unsigned short);
*/
  case APPENDF_L_l:
    return (unsigned long long)va_arg(*pargs, unsigned long);
  case APPENDF_L_ll:
    return (unsigned long long)va_arg(*pargs, unsigned long long);
  case APPENDF_L_j:
    return (unsigned long long)va_arg(*pargs, intmax_t);
  case APPENDF_L_z:
    return (unsigned long long)va_arg(*pargs, size_t);
  case APPENDF_L_t:
    return (unsigned long long)va_arg(*pargs, ptrdiff_t);
  default:
    return (unsigned long long)va_arg(*pargs, unsigned int);
  }
}

static real_t get_float(va_list *pargs, int lengthmodifier) {
  switch( lengthmodifier ) {
  case APPENDF_L_L:
    return (real_t)va_arg(*pargs, long double);
  default:
    return (real_t)va_arg(*pargs, double);
  }
}

/*
 * Ignores APPENDF_C_literal or APPENDF_C_percent
 * pvalue - must be (long long *), (unsigned long long *), (long double *), (wchar_t *)
 */
int TextEncoder_AppendFCell(TextEncoder *ter, void *pvalue, AppendFFormatCell *pfc) {
    switch( pfc->conversion ) {
    case APPENDF_C_d:
      return appendF_d(ter, *((long long *)pvalue), pfc->flag, pfc->fieldwidth, pfc->lengthmodifier);
    case APPENDF_C_u:
      return appendF_u(ter, *((unsigned long long *)pvalue), pfc->flag, pfc->fieldwidth, pfc->lengthmodifier);
    case APPENDF_C_x:
    case APPENDF_C_X:
      return appendF_x(ter, *((unsigned long long *)pvalue), pfc->flag, pfc->fieldwidth, pfc->lengthmodifier, pfc->conversion==APPENDF_C_X);
    case APPENDF_C_o:
      return appendF_o(ter, *((unsigned long long *)pvalue), pfc->flag, pfc->fieldwidth, pfc->lengthmodifier);
    case APPENDF_C_f:
    case APPENDF_C_F:
      return appendF_f(ter, *((long double *)pvalue), pfc->flag, pfc->fieldwidth, pfc->precision < 0 ? 6 : pfc->precision, pfc->conversion==APPENDF_C_F);
    case APPENDF_C_e:
    case APPENDF_C_E:
      return appendF_e(ter, *((long double *)pvalue), pfc->flag, pfc->fieldwidth, pfc->precision < 0 ? 6 : pfc->precision, pfc->conversion==APPENDF_C_E);
    case APPENDF_C_g:
    case APPENDF_C_G:
      return appendF_g(ter, *((long double *)pvalue), pfc->flag, pfc->fieldwidth, pfc->precision < 0 ? 6 : pfc->precision, pfc->conversion==APPENDF_C_G);
    case APPENDF_C_s:
      return appendF_s(ter, (wchar_t *)pvalue, pfc->flag, pfc->fieldwidth, wcslen((wchar_t *)pvalue));
    case APPENDF_C_c:
      return appendF_c(ter, *((wchar_t *)pvalue), pfc->flag, pfc->fieldwidth);
    }
    return 0;
}

/**
 * Appends formatted values.
 * @param ter TextEncoder instance.
 * @param fmt Format, like ISO 9899. Length modifies "hh" or "h", or conversion specifier "a", "A", "p" or "n" is not acceptable.
 * @returns Bytes of encoded text.
 *
 */
int TextEncoder_AppendF(TextEncoder *ter, const wchar_t *fmt, ...) {
  AnalyzedFormat *af;
  va_list args;
  long long intv;
  unsigned long long uintv;
  long double floatv;
  wchar_t wcharv;
  int argslen;
  int n;
  int ret = 0;

  af = AnalyzedFormat_New();
  if( af == NULL ) {
    return 0;
  }

  AnalyzedFormat_Analyze(af, fmt);
  argslen = af->params;

  va_start(args, fmt);

  ret = 0;
  for( n = 0; n < af->list->length; n++ ) {
    AppendFFormatCell *cell;
    cell = (AppendFFormatCell *)DataList_Get(af->list, n);
    switch( cell->conversion ) {
    case APPENDF_C_literal:
      /* simply prints */
      ret += TextEncoder_Append(ter, cell->head, cell->tail-cell->head);
      break;
    case APPENDF_C_d:
      intv = get_int(&args, cell->lengthmodifier);
      ret += TextEncoder_AppendFCell(ter, &intv, cell);
      break;
    case APPENDF_C_u:
    case APPENDF_C_x:
    case APPENDF_C_X:
    case APPENDF_C_o:
      uintv = get_uint(&args, cell->lengthmodifier);
      ret += TextEncoder_AppendFCell(ter, &uintv, cell);
      break;
    case APPENDF_C_f:
    case APPENDF_C_F:
    case APPENDF_C_e:
    case APPENDF_C_E:
    case APPENDF_C_g:
    case APPENDF_C_G:
      floatv = get_float(&args, cell->lengthmodifier);
      ret += TextEncoder_AppendFCell(ter, &floatv, cell);
      break;
    case APPENDF_C_s:
      ret += TextEncoder_AppendFCell(ter, (wchar_t *)va_arg(args, wchar_t *), cell);
      break;
    case APPENDF_C_c:
      // 2018/03/27 changed: 2nd arg of va_arg changed from wchar_t to int
      // (sizeof(wchar_t) must not be less than sizeof(int)
      // wcharv = (wchar_t)va_arg(args, wchar_t);
      wcharv = (wchar_t)va_arg(args, int);
      ret += TextEncoder_AppendFCell(ter, &wcharv, cell);
      break;
/*
    case APPENDF_C_p:
    case APPENDF_C_a:
    case APPENDF_C_A:
    case APPENDF_C_n:
*/
    case APPENDF_C_percent:
      ret += TextEncoder_AppendChar(ter, L'%');
      break;
    }
  }
  return ret;
}

#endif /* textencoder_appendf.c */
