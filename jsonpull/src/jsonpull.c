#ifndef _JSONPULL_C_
#define _JSONPULL_C_

#include <wchar.h>
#include <stdlib.h>
#include <string.h> /* memcpy */

#ifdef _JSONPULL_INTERNAL_
#  ifndef _TEXTENCODING_INTERNAL_
#    define _TEXTENCODING_INTERNAL_
#  endif
#  include "textencoding.c"
#  ifndef _TEXTDECODER_INTERNAL_
#    define _TEXTDECODER_INTERNAL_
#  endif
#  include "textdecoder.c"
#  ifndef _JSONLEX_INTERNAL_
#    define _JSONLEX_INTERNAL_
#  endif
#  include "jsonlex.c"
#  ifndef _DATALIST_INTERNAL_
#    define _DATALIST_INTERNAL_
#  endif
#  include "datalist.c"
#  ifndef _DATALISTCELL_INTERNAL_
#    define _DATALISTCELL_INTERNAL_
#  endif
#  include "datalistcell.c"
#  ifndef _PRINTERROR_INTERNAL_
#    define _PRINTERROR_INTERNAL_
#  endif
#  include "printerror.c"
#else
#  include "printerror.h"
#  include "datalistcell.h"
#  include "datalist.h"
#  include "jsonlex.h"
#  include "textdecoder.h"
#  include "textencoding.h"
#endif

#include "jsonpull_tkn.h"
#include "jsontype.h"

#ifndef _JSONPULL_EXTERN
#  ifdef _JSONPULL_INTERNAL_
#    define _JSONPULL_EXTERN static
#  else
#    define _JSONPULL_EXTERN
#  endif
#endif
#include "jsonpull.h"

/* macors for malloc and free */
#ifndef MALLOC
#define MALLOC(p) (malloc((p)))
#endif
#ifndef FREE
#define FREE(p) (free((p)))
#endif


#define _V (257) /* Value */
#define _A1 (258) /* Array 1 */
#define _A2 (259) /* Array 2 */
#define _O1 (260) /* Object 1 */
#define _O2 (261) /* Object 2 */
#define _K (262) /* Key */

/*
V  : REAL
   | INT
   | STRING
   | LITERAL
   | BEGIN_ARRAY A1
   | BEGIN_OBJECT O1
A1 : END_ARRAY
   | V A2
A2 : END_ARRAY
   | COMMA V A2
O1 : END_OBJECT
   | K COLON V O2
O2 : END_OBJECT
   | COMMA K COLON V O2
*/


/*
static void print_grammar_stack(JsonPull *jsonpull, FILE *fp, int last, int tkncode) {
  int len, n;
  int *buff;

  fprintf(fp, "tkn: %d, grammar_stack: ", tkncode);

  len = jsonpull->grammar_stack->length;
  buff = (int *)MALLOC(sizeof(int)*len);
  DataList_CopyTo(jsonpull->grammar_stack, 0, buff, len);
  for( n = 0; n < len; n++ ) {
    fprintf(fp, "%d ", buff[n]);
  }
  if( last >= 0 ) {
    fprintf(fp, "[%d]\n", last);
  }
  else {
    fprintf(fp, "[]\n");
  }
  FREE(buff);
  return;
}
*/

#ifndef _JSONPULL_GETSTR
#define _JSONPULL_GETSTR(buff, bytes, fp) (fread((buff),1,(bytes),(fp)))
#endif

#ifndef _JSONPULL_EOF
#define _JSONPULL_EOF(fp) (feof((fp)))
#endif

static int grammar_stack_push(JsonPull *jsonpull, int value) {
  return DataList_Push(jsonpull->grammar_stack, &value, 1);
}

static int JsonPull_OnEOF(JsonPull *jsonpull, int pullret) {
  if( jsonpull->grammar_stack->length <= 0 ) {
    /* empty ok */
    return pullret;
  }
  else if( jsonpull->grammar_stack->length == 1 ) {
    int *p;
    p = (int *)DataList_Get(jsonpull->grammar_stack, 0);
    if( p != NULL && *p == _V ) {
      /* only _V in grammar_stack. OK */
      return pullret;
    }
  }
  /* error */
  jsonpull->status = JSONPULL_ST_ERROR;
  jsonpull->err = JSONPULL_EVENT_ERROR_UNEXPECTED_EOF;
  return jsonpull->err;
}


static int JsonPull_NewToken(JsonPull *jsonpull, int tkncode) {
  int st;

  if( !(jsonpull->grammar_stack->length > 0) ) {
    /* grammar_stack is empty */
    jsonpull->err = JSONPULL_EVENT_ERROR_EMPTY_STACK;
    return jsonpull->err;
  }

  /* pop one */
  DataList_Pop(jsonpull->grammar_stack, &st, 1);

  switch(st) {
  case _V:
    switch(tkncode) {
    case JSONPULL_TKN_REAL:
      return JSONPULL_EVENT_REAL;
    case JSONPULL_TKN_INT:
      return JSONPULL_EVENT_INT;
    case JSONPULL_TKN_STRING:
      return JSONPULL_EVENT_STRING;
    case JSONPULL_TKN_LITERAL:
      return JSONPULL_EVENT_LITERAL;
    case JSONPULL_TKN_BEGIN_ARRAY:
      grammar_stack_push(jsonpull, _A1);
      return JSONPULL_EVENT_BEGIN_ARRAY;
    case JSONPULL_TKN_BEGIN_OBJECT:
      grammar_stack_push(jsonpull, _O1);
      return JSONPULL_EVENT_BEGIN_OBJECT;
      break;
    default:
      /* not value */
      /* print_grammar_stack(jsonpull, stderr, st, tkncode); */
      jsonpull->err = JSONPULL_EVENT_ERROR_NOT_VALUE;
      return jsonpull->err;
    }
    break;
  case _A1:
    switch(tkncode) {
    case JSONPULL_TKN_END_ARRAY:
      return JSONPULL_EVENT_END_ARRAY;
    case JSONPULL_TKN_REAL:
      grammar_stack_push(jsonpull, _A2);
      return JSONPULL_EVENT_REAL;
    case JSONPULL_TKN_INT:
      grammar_stack_push(jsonpull, _A2);
      return JSONPULL_EVENT_INT;
    case JSONPULL_TKN_STRING:
      grammar_stack_push(jsonpull, _A2);
      return JSONPULL_EVENT_STRING;
    case JSONPULL_TKN_LITERAL:
      grammar_stack_push(jsonpull, _A2);
      return JSONPULL_EVENT_LITERAL;
    case JSONPULL_TKN_BEGIN_ARRAY:
      grammar_stack_push(jsonpull, _A2);
      grammar_stack_push(jsonpull, _A1);
      return JSONPULL_EVENT_BEGIN_ARRAY;
    case JSONPULL_TKN_BEGIN_OBJECT:
      grammar_stack_push(jsonpull, _A2);
      grammar_stack_push(jsonpull, _O1);
      return JSONPULL_EVENT_BEGIN_OBJECT;
    default:
      /* missing value, object or end array */
      /* print_grammar_stack(jsonpull, stderr, st, tkncode); */
      jsonpull->err = JSONPULL_EVENT_ERROR_ARRAY;
      return jsonpull->err;
    }
    break;
  case _A2:
    switch(tkncode) {
    case JSONPULL_TKN_END_ARRAY:
      return JSONPULL_EVENT_END_ARRAY;
    case JSONPULL_TKN_COMMA:
      grammar_stack_push(jsonpull, _A2);
      grammar_stack_push(jsonpull, _V);
      return JSONPULL_EVENT_NONE; /* none */
    default:
      /* missing comma or end array */
      /* print_grammar_stack(jsonpull, stderr, st, tkncode); */
      jsonpull->err = JSONPULL_EVENT_ERROR_ARRAY;
      return jsonpull->err;
    }
    break;
  case _O1:
    switch(tkncode) {
    case JSONPULL_TKN_END_OBJECT:
      return JSONPULL_EVENT_END_OBJECT;
    case JSONPULL_TKN_STRING:
      grammar_stack_push(jsonpull, _O2);
      grammar_stack_push(jsonpull, _V);
      grammar_stack_push(jsonpull, JSONPULL_TKN_COLON);
      return JSONPULL_EVENT_OBJECT_KEY;
    default:
      /* missing key or end object */
      /* print_grammar_stack(jsonpull, stderr, st, tkncode); */
      jsonpull->err = JSONPULL_EVENT_ERROR_OBJECT;
      return jsonpull->err;
    }
    break;
  case _O2:
    switch(tkncode) {
    case JSONPULL_TKN_END_OBJECT:
      return JSONPULL_EVENT_END_OBJECT;
    case JSONPULL_TKN_COMMA:
      grammar_stack_push(jsonpull, _O2);
      grammar_stack_push(jsonpull, _V);
      grammar_stack_push(jsonpull, JSONPULL_TKN_COLON);
      grammar_stack_push(jsonpull, _K);
      return JSONPULL_EVENT_NONE; /* none */
    default:
      /* missing comma or end object */
      /* print_grammar_stack(jsonpull, stderr, st, tkncode); */
      jsonpull->err = JSONPULL_EVENT_ERROR_OBJECT;
      return jsonpull->err;
    }
    break;
  case _K:
    switch(tkncode) {
    case JSONPULL_TKN_STRING:
      return JSONPULL_EVENT_OBJECT_KEY;
      break;
    default:
      /* missing key */
      /* print_grammar_stack(jsonpull, stderr, st, tkncode); */
      jsonpull->err = JSONPULL_EVENT_ERROR_OBJECT;
      return jsonpull->err;
    }
    break;
  case JSONPULL_TKN_COLON:
    switch(tkncode) {
    case JSONPULL_TKN_COLON:
      return JSONPULL_EVENT_NONE; /* none */
    default:
      /* missing colon */
      /* print_grammar_stack(jsonpull, stderr, st, tkncode); */
      jsonpull->err = JSONPULL_EVENT_ERROR_OBJECT;
      return jsonpull->err;
    }
    break;
  case JSONPULL_TKN_COMMA:
    switch(tkncode) {
    case JSONPULL_TKN_COMMA:
      return JSONPULL_EVENT_NONE; /* none */
    default:
      /* missing comma */
      /* print_grammar_stack(jsonpull, stderr, st, tkncode); */
      jsonpull->err = JSONPULL_EVENT_ERROR_MISSING_COMMA;
      return jsonpull->err;
    }
  }
  /* FATAL */
  /* print_grammar_stack(jsonpull, stderr, st, tkncode); */
  jsonpull->err = JSONPULL_EVENT_ERROR;
  return jsonpull->err;
}

static void JsonPull_ClearKeyStack(JsonPull *jsonpull) {
  if( jsonpull->key_stack != NULL ) {
    while( jsonpull->key_stack->length > 0 ) {
      wchar_t *ptr;
      if( DataList_Pop(jsonpull->key_stack, &ptr, 1) == 1 ) {
        if( ptr != NULL ) {
          FREE(ptr);
        }
      }
    }
  }
}

/* ---------------------------------------------------------------- global */
JsonPull *JsonPull_New(TextEncoding *te) {
  JsonPull *jsonpull;

  jsonpull = (JsonPull *)MALLOC(sizeof(JsonPull));
  if( jsonpull == NULL ) {
    print_perror("malloc");
    return NULL;
  }

  jsonpull->decoder = NULL;
  jsonpull->grammar_stack = NULL;
  jsonpull->key_stack = NULL;
  jsonpull->lex = NULL;

  if( te != NULL ) {
    jsonpull->decoder = TextEncoding_NewDecoder(te);
  }
  else {
    /* If te is NULL, constructs UTF8 */
    te = TextEncoding_New_UTF8();
    if( te != NULL ) {
      jsonpull->decoder = TextEncoding_NewDecoder(te);
      TextEncoding_Free(te);
    }
  }
  if( jsonpull->decoder == NULL ) {
    JsonPull_Free(jsonpull);
    return NULL;
  }

  jsonpull->lex = JsonLex_New();
  if( jsonpull->lex == NULL ) {
    JsonPull_Free(jsonpull);
    return NULL;
  }

  jsonpull->grammar_stack = DataList_New(64, sizeof(int));
  if (jsonpull->grammar_stack == NULL) {
    JsonPull_Free(jsonpull);
    return NULL;
  }

  jsonpull->key_stack = DataList_New(16, sizeof(void *));
  if (jsonpull->grammar_stack == NULL) {
    JsonPull_Free(jsonpull);
    return NULL;
  }

  /* first */
  grammar_stack_push(jsonpull, _V);

  jsonpull->status = JSONPULL_ST_NONE;
  jsonpull->col = 0;
  jsonpull->row = 1;
  jsonpull->fp = NULL;

  return jsonpull;
}

void JsonPull_Free(JsonPull *jsonpull) {
  if( jsonpull->lex != NULL ) {
    JsonLex_Free(jsonpull->lex);
  }
  if( jsonpull->key_stack != NULL ) {
    JsonPull_ClearKeyStack(jsonpull);
    DataList_Free(jsonpull->key_stack);
  }
  if( jsonpull->grammar_stack != NULL ) {
    DataList_Free(jsonpull->grammar_stack);
  }
  if( jsonpull->decoder != NULL ) {
    TextDecoder_Free(jsonpull->decoder);
  }
  FREE(jsonpull);
}


void JsonPull_Clear(JsonPull *jsonpull) {
  if( jsonpull->lex != NULL ) {
    JsonLex_Clear(jsonpull->lex);
  }
  if( jsonpull->key_stack != NULL ) {
    JsonPull_ClearKeyStack(jsonpull);
  }
  if( jsonpull->grammar_stack != NULL ) {
    DataList_Clear(jsonpull->grammar_stack);
    /* first */
    grammar_stack_push(jsonpull, _V);
  }
  if( jsonpull->decoder != NULL ) {
    TextDecoder_Clear(jsonpull->decoder);
  }
  jsonpull->status = JSONPULL_ST_NONE;
  jsonpull->col = 0;
  jsonpull->row = 1;
  jsonpull->err = 0;
}

int JsonPull_AppendText(JsonPull *jsonpull, const unsigned char *text, int count) {
  return TextDecoder_Append(jsonpull->decoder, text, count);
}

void JsonPull_SetFilePointer(JsonPull *jsonpull, _JSONPULL_FILE *fp) {
  jsonpull->fp = fp;
}

#define READ_BUFFER_SIZE (4096)

static int JsonPull_CopyValue(JsonPull *jsonpull, int max_valuelen, wchar_t *value, int *valuelen) {
  int ret;
  ret = DataList_CopyTo(
    jsonpull->lex->tbuff,
    0,
    value,
    jsonpull->lex->tbuff->length < max_valuelen ?
      jsonpull->lex->tbuff->length : max_valuelen
  );
  if( valuelen != NULL ) {
    *valuelen = ret;
  }
  value[ret] = L'\0';
  return ret;
}

int JsonPull_ReadValue(JsonPull *jsonpull, int max_valuelen, wchar_t *value, int *valuelen) {
  int ret = 0;
  if( value != NULL && max_valuelen > 0 ) {
    ret = JsonPull_CopyValue(jsonpull, max_valuelen, value, valuelen);
  }
  DataList_Clear(jsonpull->lex->tbuff);
  return ret;
}

int JsonPull_CopyKey(JsonPull *jsonpull, int max_keylen, wchar_t *key, int *keylen) {
  *keylen = -1;
  if( jsonpull->key_stack->length > 0 ) {
    wchar_t **pptail;
    pptail = (wchar_t **)DataList_Get(jsonpull->key_stack, jsonpull->key_stack->length-1);
    if( pptail != NULL && *pptail != NULL ) {
      wcsncpy(key, *pptail, max_keylen);
      *keylen = wcslen(key);
    }
  }
  key[*keylen > 0 ? *keylen : 0] = L'\0';
  return *keylen;
}

int JsonPull_Pull(JsonPull *jsonpull) {
  int tkncode;
  int lexret;
  wchar_t wc;
  static unsigned char buff[READ_BUFFER_SIZE];

  while(1) {
    switch( jsonpull->status ) {
    case JSONPULL_ST_EOF:
      return JSONPULL_EVENT_EOF;
    case JSONPULL_ST_ERROR:
      return JSONPULL_EVENT_ERROR;
    }
    /* reads if needed. */
    if( !(jsonpull->decoder->wc_buff->length > 0) ) {
      if( jsonpull->fp == NULL || _JSONPULL_EOF(jsonpull->fp) ) {
        /* no more input. */
        lexret = JsonLex_Finish(jsonpull->lex, &tkncode);
        jsonpull->status = JSONPULL_ST_EOF;
      }
      else {
        size_t read;
        read = _JSONPULL_GETSTR(buff, READ_BUFFER_SIZE, jsonpull->fp);
        if( !(read > 0) ) {
          return JSONPULL_EVENT_NONE;
        }
        JsonPull_AppendText(jsonpull, buff, read);
      }
    }
    if( jsonpull->decoder->wc_buff->length > 0 ) {
      int colb4;
      int rowb4;
      colb4 = jsonpull->col;
      rowb4 = jsonpull->row;
      /* Gets one */
      TextDecoder_Get(jsonpull->decoder, &wc, 1);
      /* counter up */
      if( wc == L'\n' ) {
        /* new line */
        jsonpull->col = 0;
        jsonpull->row++;
      }
      else {
        jsonpull->col++;
      }
      lexret = JsonLex_NewChar(jsonpull->lex, &wc, &tkncode);
      if( lexret == JSONLEX_ERR_STAY ) {
        /* Unget */
        TextDecoder_Unget(jsonpull->decoder, &wc, 1);
        /* counter down */
        jsonpull->col = colb4;
        jsonpull->row = rowb4;
      }
    }
    if( JSONLEX_iserror(lexret) ) {
      jsonpull->status = JSONPULL_ST_ERROR;
      jsonpull->err = JSONPULL_EVENT_ERROR_LEX;
      return jsonpull->err;
    }

    if( tkncode != JSONPULL_TKN_UNKNOWN ) {
      int pullret;
      pullret = JsonPull_NewToken(jsonpull, tkncode);
      if( JSONPULL_iserror(pullret) ) {
        /* error detected */
        return pullret;
      }
      if( JSONPULL_isvalue(pullret) ) {
        return pullret;
      }
      switch( pullret ) {
      case JSONPULL_EVENT_OBJECT_KEY:
        {
          /* replace key */
          wchar_t *pnew;
          wchar_t **pptail;
          int newlen;
          /* tail */
          pptail = (wchar_t **)DataList_Get(jsonpull->key_stack, jsonpull->key_stack->length-1);
          /* creates new key (+1 for null char) */
          pnew = (wchar_t *)MALLOC(sizeof(wchar_t) * (jsonpull->lex->tbuff->length+1));
          if( pnew == NULL ) {
            /* error */
            print_perror("malloc");
            jsonpull->status = JSONPULL_ST_ERROR;
            jsonpull->err = JSONPULL_EVENT_ERROR_SYSERROR;
            return jsonpull->err;
          }
          /* frees old key */
          if( pptail != NULL && *pptail != NULL ) {
            FREE(*pptail);
          }
          JsonPull_CopyValue(jsonpull, jsonpull->lex->tbuff->length, pnew, &newlen);
          /* sets new key */
          memcpy(pptail, &pnew, sizeof(wchar_t *));
        }
        return pullret;
      case JSONPULL_EVENT_BEGIN_ARRAY:
      case JSONPULL_EVENT_BEGIN_OBJECT:
        /* allocates "key" memory at tail of key_stack */
        {
          wchar_t *p = NULL;
          if( DataList_Push(jsonpull->key_stack, &p, 1) != 1 ) {
            /* error occurred */
            jsonpull->status = JSONPULL_ST_ERROR;
            return JSONPULL_EVENT_ERROR_SYSERROR;
          }
        }
        break;
      case JSONPULL_EVENT_END_ARRAY:
      case JSONPULL_EVENT_END_OBJECT:
        /* frees "key" memory at tail of key_stack */
        {
          wchar_t *p;
          if( DataList_Pop(jsonpull->key_stack, &p, 1) == 1 ) {
            if( p != NULL ) {
              FREE(p);
            }
          }
        }
        break;
      default:
        /* DOES NOTHING */
        break;
      }
      /* finally, checks grammar_stack if came EOF at this time. */
      if( jsonpull->status != JSONPULL_ST_EOF ) {
        /* not eof OK */
        return pullret;
      }
      /* (others) -> eof */
      return JsonPull_OnEOF(jsonpull, pullret);
    }
    else {
      /* does not return at this time. */
      /* finally, checks grammar_stack if came EOF at this time. */
      if( jsonpull->status == JSONPULL_ST_EOF ) {
        /* (others) -> eof */
        return JsonPull_OnEOF(jsonpull, JSONPULL_EVENT_EOF);
      }
    }
  } /* end of infinite loop */
}

const char *JsonPull_ErrorReason(JsonPull *jsonpull) {
  switch(jsonpull->err) {
  case JSONPULL_EVENT_ERROR:
    return "An error occurred";
  case JSONPULL_EVENT_ERROR_LEX:
    return JsonLex_ErrorReason(jsonpull->lex);
  case JSONPULL_EVENT_ERROR_EMPTY_STACK:
    return "Stack is empty";
  case JSONPULL_EVENT_ERROR_NOT_VALUE:
    return "Non value text appeared";
  case JSONPULL_EVENT_ERROR_ARRAY:
    return "Array text appeared";
  case JSONPULL_EVENT_ERROR_OBJECT:
    return "Object text appeared";
  case JSONPULL_EVENT_ERROR_MISSING_COMMA:
    return "Missing comma";
  case JSONPULL_EVENT_ERROR_UNEXPECTED_EOF:
    return "Unexpected EOF";
  case JSONPULL_EVENT_ERROR_SYSERROR:
    return "System error";
  default:
    return "No error";
  }
}

void JsonPull_PrintError(JsonPull *jsonpull) {
  const char *reason = NULL;
  reason = JsonPull_ErrorReason(jsonpull);
  print_errorf("%s at line: %d, col: %d\n", reason, jsonpull->row, jsonpull->col);
}

#endif /* jsonpull.c */

/*
// cc printerror.c datalistcell.c datalist.c textencoder.c textdecoder.c textencoding.c jsonlex.c jsonpull.c
#include <stdio.h>
#include <wchar.h>
#include <string.h> // strlen

int main(int ac, char *av[]) {
  static wchar_t value[1025];
  int valuelen;

  static wchar_t key[1025];
  int keylen;

  char *text =
    "{\n"
    "  \"ほげ\": 123,\n"
    "  \"b\": \"hoge\",\n"
    "  \"c\": [1,true,3]\n"
    "}\n";

  JsonPull *jsonpull;
  TextEncoding *te = TextEncoding_New_UTF8();
  TextEncoder *ter = TextEncoding_NewEncoder(te);
//  TextDecoder *tdr = TextEncoding_NewDecoder(te);
  jsonpull = JsonPull_New(te);
  jsonpull->lex->allow_notquoted_text = 0;
  JsonPull_AppendText(jsonpull, text, strlen(text));

  while( 1 ) {
    int ev;
    ev = JsonPull_Pull(jsonpull);
    if( ev == JSONPULL_EVENT_EOF ) {
      return 0;
    }
    else if( JSONPULL_iserror(ev) ) {
      switch(ev) {
      case JSONPULL_EVENT_ERROR_LEX:
        fprintf(stderr, "LEX ERROR line: %d, col: %d, code: %08x\n",
          jsonpull->row, jsonpull->col, jsonpull->lex->err);
        break;
      default:
        fprintf(stderr, "PULL ERROR line: %d, col: %d, code: %08x\n",
          jsonpull->row, jsonpull->col, jsonpull->err);
        break;
      }
      return 1;
    }
    else if( ev == JSONPULL_EVENT_NONE ) {
      // DOES NOTHING
    }
    else {
      if( JSONPULL_isvalue(ev) ) {
        JsonPull_ReadValue(jsonpull, 1024, value, &valuelen);
        JsonPull_CopyKey(jsonpull, 1024, key, &keylen);
        printf("%08x: ", ev);
        if( keylen >= 0 ) {
          printf("\"");
          fputwcs(ter, stdout, key);
          printf("\": ");
        }
        printf("\"");
        fputwcs(ter, stdout, value);
        printf("\"\n");
      }
      else {
        switch( ev ) {
        case JSONPULL_EVENT_OBJECT_KEY:
          JsonPull_ReadValue(jsonpull, 1024, value, &valuelen);
          printf("%08x: ", ev);
          printf("\"");
          fputwcs(ter, stdout, value);
          printf("\"\n");
          break;
        case JSONPULL_EVENT_BEGIN_ARRAY:
        case JSONPULL_EVENT_BEGIN_OBJECT:
          printf("%08x\n", ev);
          break;
        case JSONPULL_EVENT_END_ARRAY:
        case JSONPULL_EVENT_END_OBJECT:
          printf("%d ", ev);
          JsonPull_CopyKey(jsonpull, 1024, key, &keylen);
          printf("%d: ", ev);
          if( keylen >= 0 ) {
            printf("\"");
            fputwcs(ter, stdout, key);
            printf("\": ");
          }
          printf("(none)\n");
          break;
        }
      }
    }
  }
}
*/
