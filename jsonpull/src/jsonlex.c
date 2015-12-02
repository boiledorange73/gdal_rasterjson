#ifndef _JSONLEX_C_
#define _JSONLEX_C_

#include <stdlib.h>
#include <wchar.h>
//#include <ctype.h>

#ifdef _JSONLEX_INTERNAL_
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

#include "jsonpull_tkn.h"

#ifndef _JSONLEX_EXTERN
#  ifdef _JSONLEX_INTERNAL_
#    define _JSONLEX_EXTERN static
#  else
#    define _JSONLEX_EXTERN
#  endif
#endif
#include "jsonlex.h"

/* macors for malloc and free */
#ifndef MALLOC
#define MALLOC(p) (malloc((p)))
#endif
#ifndef FREE
#define FREE(p) (free((p)))
#endif

/* -------------------------------- begin internal -------- */
#define _SPC (0x01)
#define _DIG (0x02)
#define _LET (0x03)

static wint_t charcode[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, _SPC, _SPC, _SPC, _SPC, _SPC, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  _SPC, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x2B, 0x2C, 0x2D, 0x2E, 0x00,
  _DIG, _DIG, _DIG, _DIG, _DIG, _DIG, _DIG, _DIG,
  _DIG, _DIG, 0x3A, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, _LET, _LET, _LET, _LET, 0x45, _LET, _LET,
  _LET, _LET, _LET, _LET, _LET, _LET, _LET, _LET,
  _LET, _LET, _LET, _LET, _LET, _LET, _LET, _LET,
  _LET, _LET, _LET, 0x5B, 0x5C, 0x5D, 0x00, _LET,
  0x00, _LET, _LET, _LET, _LET, 0x65, _LET, _LET,
  _LET, _LET, _LET, _LET, _LET, _LET, _LET, _LET,
  _LET, _LET, _LET, _LET, _LET, _LET, _LET, _LET,
  _LET, _LET, _LET, 0x7B, 0x00, 0x7D, 0x00, 0x00
};

#define _ST_NONE (0x00001000)
#define _ST_PLUSMINUS (0x00001001)
#define _ST_INT (0x00001002)
#define _ST_DOT (0x00001003)
#define _ST_REAL (0x00001004)
#define _ST_EXP_HEAD (0x00001005)
#define _ST_EXP_PLUSMINUS (0x00001006)
#define _ST_EXP_NUM (0x00001007)
#define _ST_TEXT (0x00001008)
#define _ST_TEXT_ESC (0x00001009)
#define _ST_LITERAL (0x0000100A)
#define _ST_ERROR (-1)

static int JsonLex_DataListCompare(DataList *buff, const wchar_t *text) {
  int ix = 0;
  wchar_t *p;

  while(1) {
    p = (wchar_t *)DataList_Get(buff, ix);
    if( p == NULL ) {
      if( *text == L'\0' ) {
        return 0;
      }
      else {
        return -1;
      }
    }
    if( *p < *text ) {
      return -1;
    }
    else if( *p > *text ) {
      return 1;
    }
    ix++;
    text++;
  }
}

/* -------------------------------- end internal -------- */

JsonLex *JsonLex_New() {
    JsonLex *lex;
    lex = (JsonLex *)MALLOC(sizeof(JsonLex));
    if (lex == NULL) {
        print_perror("malloc");
        return NULL;
    }
    lex->st = _ST_NONE;
    lex->err = JSONLEX_ERR_NOERROR;
    lex->allow_notquoted_text = 0;
    lex->tbuff = DataList_New(4096, sizeof(wchar_t));
    if (lex->tbuff == NULL) {
        /* could not allocate DataList */
        JsonLex_Free(lex);
        return NULL;
    }
    return lex;
}

void JsonLex_Free(JsonLex *lex) {
    if (lex->tbuff != NULL) {
        DataList_Free(lex->tbuff);
    }
    FREE(lex);
}

void JsonLex_Clear(JsonLex *lex) {
    lex->st = _ST_NONE;
    lex->err = JSONLEX_ERR_NOERROR;
    if (lex->tbuff != NULL) {
        DataList_Clear(lex->tbuff);
    }
}

int JsonLex_NewChar(JsonLex *lex, wchar_t *cp, int *tkncode) {
  wint_t chcd;
  wchar_t onechar = 0;

  if( lex->st == _ST_ERROR ) {
    *tkncode = JSONPULL_TKN_ERROR;
    return JSONLEX_ERR_ERROR;
  }

  *tkncode = JSONPULL_TKN_UNKNOWN;

  /* text */
  switch( lex->st ) {
  case _ST_TEXT:
    switch( *cp ) {
    case L'\"':
      /* text -> none */
      lex->st = _ST_NONE;
      *tkncode = JSONPULL_TKN_STRING;
      return JSONLEX_ERR_NOERROR;
    case L'\\':
      /* text -> textesc */
      lex->st = _ST_TEXT_ESC;
      return JSONLEX_ERR_NOERROR;
    default:
      /* text -> text */
      DataList_Push(lex->tbuff, cp, 1);
      return JSONLEX_ERR_NOERROR;
    }
  case _ST_TEXT_ESC:
    switch( *cp ) {
    case L'b':
      onechar= 0x08;
      DataList_Push(lex->tbuff, &onechar, 1);
      break;
    case L't':
      onechar= 0x09;
      DataList_Push(lex->tbuff, &onechar, 1);
      break;
    case L'n':
      onechar= 0x0a;
      DataList_Push(lex->tbuff, &onechar, 1);
      break;
    case L'v':
      onechar= 0x0b;
      DataList_Push(lex->tbuff, &onechar, 1);
      break;
    case L'f':
      onechar= 0x0c;
      DataList_Push(lex->tbuff, &onechar, 1);
      break;
    case L'r':
      onechar= 0x0d;
      DataList_Push(lex->tbuff, &onechar, 1);
      break;
    default:
      onechar= *cp;
      DataList_Push(lex->tbuff, &onechar, 1);
      break;
    }
    lex->st = _ST_TEXT;
    return JSONLEX_ERR_NOERROR;
  }

  /* usual */
  if( *cp >= 0 && *cp <= 127 ) {
    chcd = charcode[*cp];
  }
  else {
    chcd = _LET;
  }
  if( chcd == 0 ) {
    /* error */
    lex->st = _ST_ERROR;
    *tkncode = JSONPULL_TKN_ERROR;
    lex->err = JSONLEX_ERR_INVALID_CHARACTER;
    return lex->err;
  }

  switch( lex->st ) {
  case _ST_NONE:
    switch( chcd ) {
    case _SPC:
      /* DOES NOTHING */
      break;
    case L'\"':
      /* none -> text */
      lex->st = _ST_TEXT;
      break;
    case L'E':
    case L'e':
    case _LET:
      /* none -> literal */
      DataList_Push(lex->tbuff, cp, 1);
      lex->st = _ST_LITERAL;
      break;
    case L'+':
    case L'-':
      /* none -> plusminus */
      DataList_Push(lex->tbuff, cp, 1);
      lex->st = _ST_PLUSMINUS;
      break;
    case L'.':
      /* none -> dot */
      DataList_Push(lex->tbuff, cp, 1);
      lex->st = _ST_DOT;
      break;
    case _DIG:
      /* none -> int */
      DataList_Push(lex->tbuff, cp, 1);
      lex->st = _ST_INT;
      break;
    case L'{':
      /* one charcter */
      *tkncode = JSONPULL_TKN_BEGIN_OBJECT;
      break;
    case L'}':
      /* one charcter */
      *tkncode = JSONPULL_TKN_END_OBJECT;
      break;
    case L'[':
      /* one charcter */
      *tkncode = JSONPULL_TKN_BEGIN_ARRAY;
      break;
    case L']':
      /* one charcter */
      *tkncode = JSONPULL_TKN_END_ARRAY;
      break;
    case L',':
      /* one charcter */
      *tkncode = JSONPULL_TKN_COMMA;
      break;
    case L':':
      /* one charcter */
      *tkncode = JSONPULL_TKN_COLON;
      break;
    default:
      /* error */
      lex->st = _ST_ERROR;
      *tkncode = JSONPULL_TKN_ERROR;
      lex->err = JSONLEX_ERR_INVALID_CHARACTER;
      return lex->err;
    }
    return JSONLEX_ERR_NOERROR;
  case _ST_LITERAL:
    switch( chcd ) {
    case L'E':
    case L'e':
    case _LET:
    case _DIG:
      /* literal -> literal */
      DataList_Push(lex->tbuff, cp, 1);
      return JSONLEX_ERR_NOERROR;
    default:
      /* literal -> none */
      if( JsonLex_DataListCompare(lex->tbuff, L"null") == 0 ||
          JsonLex_DataListCompare(lex->tbuff, L"true") == 0 ||
          JsonLex_DataListCompare(lex->tbuff, L"false") == 0 ) {
        lex->st = _ST_NONE;
        *tkncode = JSONPULL_TKN_LITERAL;
        return JSONLEX_ERR_STAY;
      }
      else if( lex->allow_notquoted_text ) {
        lex->st = _ST_NONE;
        *tkncode = JSONPULL_TKN_STRING;
        return JSONLEX_ERR_STAY;
      }
      else {
        /* error */
        lex->st = _ST_ERROR;
        *tkncode = JSONPULL_TKN_ERROR;
        lex->err = JSONLEX_ERR_INVALID_LITERAL;
        return lex->err;
      }
    }
  case _ST_PLUSMINUS:
    switch( chcd ) {
    case _DIG:
      /* plusminus -> int */
      DataList_Push(lex->tbuff, cp, 1);
      lex->st = _ST_INT;
      return JSONLEX_ERR_NOERROR;
    case '.':
      /* plusminus -> dot */
      DataList_Push(lex->tbuff, cp, 1);
      lex->st = _ST_DOT;
      return JSONLEX_ERR_NOERROR;
    default:
      /* error */
      lex->st = _ST_ERROR;
      *tkncode = JSONPULL_TKN_ERROR;
      lex->err = JSONLEX_ERR_INVALID_CHARACTER;
      return lex->err;
    }
  case _ST_DOT:
    switch( chcd ) {
    case _DIG:
      /* dot -> real */
      DataList_Push(lex->tbuff, cp, 1);
      lex->st = _ST_REAL;
      return JSONLEX_ERR_NOERROR;
    default:
      /* error */
      lex->st = _ST_ERROR;
      *tkncode = JSONPULL_TKN_ERROR;
      lex->err = JSONLEX_ERR_INVALID_CHARACTER;
      return lex->err;
    }
  case _ST_INT:
    switch( chcd ) {
    case _DIG:
      /* int -> int */
      DataList_Push(lex->tbuff, cp, 1);
      return JSONLEX_ERR_NOERROR;
    case L'.':
      /* int -> real */
      DataList_Push(lex->tbuff, cp, 1);
      lex->st = _ST_REAL;
      return JSONLEX_ERR_NOERROR;
    case L'e':
    case L'E':
      /* int -> exphead */
      DataList_Push(lex->tbuff, cp, 1);
      lex->st = _ST_EXP_HEAD;
      return JSONLEX_ERR_NOERROR;
    case _LET:
      /* error */
      lex->st = _ST_ERROR;
      *tkncode = JSONPULL_TKN_ERROR;
      lex->err = JSONLEX_ERR_INVALID_CHARACTER;
      return lex->err;
    default:
      /* int -> none / stay */
      lex->st = _ST_NONE;
      *tkncode = JSONPULL_TKN_INT;
      return JSONLEX_ERR_STAY;
    }
  case _ST_REAL:
    switch( chcd ) {
    case _DIG:
      /* real -> real */
      DataList_Push(lex->tbuff, cp, 1);
      return JSONLEX_ERR_NOERROR;
    case L'e':
    case L'E':
      /* real -> exphead */
      DataList_Push(lex->tbuff, cp, 1);
      lex->st = _ST_EXP_HEAD;
      return JSONLEX_ERR_NOERROR;
    case L'.':
    case _LET:
      /* error */
      lex->st = _ST_ERROR;
      *tkncode = JSONPULL_TKN_ERROR;
      lex->err = JSONLEX_ERR_INVALID_CHARACTER;
      return lex->err;
    default:
      /* real -> none / stay */
      lex->st = _ST_NONE;
      *tkncode = JSONPULL_TKN_REAL;
      return JSONLEX_ERR_STAY;
    }
  case _ST_EXP_HEAD:
    switch( chcd ) {
    case _DIG:
      /* exphead -> expnum */
      DataList_Push(lex->tbuff, cp, 1);
      lex->st = _ST_EXP_NUM;
      return JSONLEX_ERR_NOERROR;
    case L'+':
    case L'-':
      /* exphead -> expplusminu */
      DataList_Push(lex->tbuff, cp, 1);
      lex->st = _ST_EXP_PLUSMINUS;
      return JSONLEX_ERR_NOERROR;
    default:
      /* error */
      lex->st = _ST_ERROR;
      *tkncode = JSONPULL_TKN_ERROR;
      lex->err = JSONLEX_ERR_INVALID_CHARACTER;
      return lex->err;
    }
  case _ST_EXP_PLUSMINUS:
    switch( chcd ) {
    case _DIG:
      /* expplusminus -> expnum */
      DataList_Push(lex->tbuff, cp, 1);
      lex->st = _ST_EXP_NUM;
      return JSONLEX_ERR_NOERROR;
    default:
      /* error */
      lex->st = _ST_ERROR;
      *tkncode = JSONPULL_TKN_ERROR;
      lex->err = JSONLEX_ERR_INVALID_CHARACTER;
      return lex->err;
    }
  case _ST_EXP_NUM:
    switch( chcd ) {
    case _DIG:
      /* expnum -> expnum */
      DataList_Push(lex->tbuff, cp, 1);
      return JSONLEX_ERR_NOERROR;
    case L'.':
    case L'e':
    case L'E':
    case _LET:
      /* error */
      lex->st = _ST_ERROR;
      *tkncode = JSONPULL_TKN_ERROR;
      lex->err = JSONLEX_ERR_INVALID_CHARACTER;
      return lex->err;
    default:
      /* real -> none / stay */
      lex->st = _ST_NONE;
      *tkncode = JSONPULL_TKN_REAL;
      return JSONLEX_ERR_STAY;
    }
    break;
  }
  return JSONLEX_ERR_ERROR;
}

int JsonLex_Finish(JsonLex *lex, int *tkncode) {
  *tkncode = JSONPULL_TKN_UNKNOWN;

  switch( lex->st ) {
  case _ST_ERROR:
    /* already error occurred */
    *tkncode = JSONPULL_TKN_ERROR;
    return JSONLEX_ERR_ERROR;
  case _ST_TEXT:
  case _ST_TEXT_ESC:
    /* error */
    lex->st = _ST_ERROR;
    *tkncode = JSONPULL_TKN_ERROR;
    lex->err = JSONLEX_ERR_NOT_TERMINATED_STRING;
    return lex->err;
  case _ST_NONE:
    return JSONLEX_ERR_NOERROR;
  case _ST_INT:
    *tkncode = JSONPULL_TKN_INT;
    return JSONLEX_ERR_NOERROR;
  case _ST_REAL:
    *tkncode = JSONPULL_TKN_REAL;
    return JSONLEX_ERR_NOERROR;
  case _ST_EXP_NUM:
    *tkncode = JSONPULL_TKN_REAL;
    return JSONLEX_ERR_NOERROR;
  case _ST_PLUSMINUS:
  case _ST_EXP_HEAD:
  case _ST_EXP_PLUSMINUS:
    /* error */
    lex->st = _ST_ERROR;
    *tkncode = JSONPULL_TKN_ERROR;
    lex->err = JSONLEX_ERR_NOT_TERMINATED_NUMBER;
    return lex->err;
  }
  return JSONLEX_ERR_ERROR;
}

const char *JsonLex_ErrorReason(JsonLex *lex) {
  switch(lex->err) {
  case JSONLEX_ERR_ERROR:
    return "An error occurred";
  case JSONLEX_ERR_INVALID_CHARACTER:
    return "Invalid caractor";
  case JSONLEX_ERR_NOT_TERMINATED_STRING:
    return "Not terminated string";
  case JSONLEX_ERR_NOT_TERMINATED_NUMBER:
    return "Not terminated number";
  case JSONLEX_ERR_INVALID_LITERAL:
    return "Invalid literal";
  default:
  /* case JSONLEX_ERR_NOERROR: */
  /* case JSONLEX_ERR_STAY: */
    return "No error";
  }
}

#endif /* jsonlex.c */
