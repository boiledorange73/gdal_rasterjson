#ifndef _JSONPARSER_C_
#define _JSONPARSER_C_

#include <wchar.h>
#include <stdlib.h>

#ifdef _JSONPARSER_INTERNAL_
#  ifndef _JSONNODE_INTERNAL_
#    define _JSONNODE_INTERNAL_
#  endif
#  include "jsonnode.c"
#  ifndef _JSONPULL_INTERNAL_
#    define _JSONPULL_INTERNAL_
#  endif
#  include "jsonpull.c"
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
#  include "jsonpull.h"
#  include "jsonnode.h"
#endif

#ifndef _JSONPARSER_EXTERN
#  ifdef _JSONPARSER_INTERNAL_
#    define _JSONPARSER_EXTERN static
#  else
#    define _JSONPARSER_EXTERN
#  endif
#endif
#include "jsonparser.h"

/* macors for malloc and free */
#ifndef MALLOC
#define MALLOC(p) (malloc((p)))
#endif
#ifndef FREE
#define FREE(p) (free((p)))
#endif

JsonParser *JsonParser_New(TextEncoding *te) {
  JsonParser *jpsr;

  jpsr = (JsonParser *)MALLOC(sizeof(JsonParser));
  if( jpsr == NULL ) {
    print_perror("malloc");
    return NULL;
  }

  jpsr->jsonpull = NULL;
  jpsr->node = NULL;
  jpsr->stack = NULL;

  if( te == NULL ) {
    te = TextEncoding_New_UTF8();
    jpsr->jsonpull = JsonPull_New(te);
    TextEncoding_Free(te);
  }
  else {
    jpsr->jsonpull = JsonPull_New(te);
  }

  if( jpsr->jsonpull == NULL ) {
    JsonParser_Free(jpsr);
    return NULL;
  }

  jpsr->stack = DataList_New(16, sizeof(JsonNode **));
  if( jpsr->stack == NULL ) {
    JsonParser_Free(jpsr);
    return NULL;
  }

  return jpsr;
}

void JsonParser_Clear(JsonParser *jpsr) {
  JsonParser_ClearStack(jpsr);
  if( jpsr->node ) {
    JsonNode_Free(jpsr->node);
    jpsr->node = NULL;
  }
  JsonPull_Clear(jpsr->jsonpull);
}

void JsonParser_ClearStack(JsonParser *jpsr) {
  while( jpsr->stack->length > 0 ) {
    JsonNode *jn;
    if( DataList_Pop(jpsr->stack, &jn, 1) == 1 && jn != NULL ) {
      JsonNode_Free(jn);
    }
  }
}

void JsonParser_Free(JsonParser *jpsr) {
  if( jpsr->jsonpull ) {
    JsonPull_Free(jpsr->jsonpull);
  }
  if( jpsr->node ) {
    JsonNode_Free(jpsr->node);
  }
  if( jpsr->stack != NULL ) {
    JsonParser_ClearStack(jpsr);
    DataList_Free(jpsr->stack);
  }
  FREE(jpsr);
}

int JsonParser_AppendText(JsonParser *jpsr, const unsigned char* text, int count) {
  return JsonPull_AppendText(jpsr->jsonpull, text, count);
}

void JsonParser_SetFilePointer(JsonParser *jpsr, _JSONPULL_FILE *fp) {
  return JsonPull_SetFilePointer(jpsr->jsonpull, fp);
}

int JsonParser_Parse(JsonParser *jpsr, JsonNode **pnode, int mode) {
  JsonNode *jn;
  static wchar_t value[1025];
  int valuelen;
  static wchar_t key[1025];
  int keylen;

  while( 1 ) {
    int ev;
    jn = NULL;
    ev = JsonPull_Pull(jpsr->jsonpull);
    if( ev == JSONPULL_EVENT_EOF ) {
      switch( jpsr->stack->length ) {
      case 0:
        /* empty */
        *pnode = JsonNode_NewNull();
        return 1;
      case 1:
        if( DataList_Pop(jpsr->stack, &jn, 1) != 1 ) {
          /* empty */
          if( mode & JSONPARSER_MODE_PRINT_ERROR ) {
            print_error("Parser: Cannot Pop\n");
          }
          goto ERROR;
        }
        *pnode = jn;
        return 1;
      default:
        if( mode & JSONPARSER_MODE_PRINT_ERROR ) {
          print_error("Parser: EOF but stack is more than 1\n");
        }
        goto FORCE_TO_BUILD;
      }
    }
    else if( JSONPULL_iserror(ev) ) {
      if( mode & JSONPARSER_MODE_PRINT_ERROR ) {
        JsonPull_PrintError(jpsr->jsonpull);
      }
      goto FORCE_TO_BUILD;
    }
    else if( ev == JSONPULL_EVENT_NONE ) {
      /* DOES NOTHING */
    }
    else if( JSONPULL_isvalue(ev) ) {
      JsonPull_ReadValue(jpsr->jsonpull, 1024, value, &valuelen);
      switch( ev ) {
      case JSONPULL_EVENT_LITERAL:
        jn = JsonNode_NewLiteral(value);
        break;
      case JSONPULL_EVENT_INT:
        jn = JsonNode_NewIntFromText(value);
        break;
      case JSONPULL_EVENT_REAL:
        jn = JsonNode_NewRealFromText(value);
        break;
      case JSONPULL_EVENT_STRING:
        jn = JsonNode_NewString(value);
        break;
      }
    }
    else {
      switch( ev ) {
      case JSONPULL_EVENT_OBJECT_KEY:
        /* clears value */
        JsonPull_ReadValue(jpsr->jsonpull, 0, NULL, NULL);
        break;
      case JSONPULL_EVENT_BEGIN_ARRAY:
        jn = JsonNode_NewArray();
        DataList_Push(jpsr->stack, &jn, 1);
        jn = NULL;
        break;
      case JSONPULL_EVENT_BEGIN_OBJECT:
        jn = JsonNode_NewObject();
        DataList_Push(jpsr->stack, &jn, 1);
        jn = NULL;
        break;
      case JSONPULL_EVENT_END_ARRAY:
      case JSONPULL_EVENT_END_OBJECT:
        DataList_Pop(jpsr->stack, &jn, 1);
        break;
      }
    }

    if( jn != NULL ) {
      /* new node is created at this iteration */
      if( !(jpsr->stack->length > 0) ) {
        /* empty stack */
        DataList_Push(jpsr->stack, &jn, 1);
        jn = NULL;
      }
      else {
        JsonNode **pparent;
        pparent = (JsonNode **)DataList_Get(jpsr->stack, jpsr->stack->length-1);
        if( pparent != NULL && *pparent != NULL ) {
          switch( (*pparent)->type ) {
          case JSONTYPE_OBJECT:
            JsonPull_CopyKey(jpsr->jsonpull, 1024, key, &keylen);
            JsonNode_ObjectSetC(*pparent, key, jn);
            JsonNode_Free(jn);
            jn = NULL;
            break;
          case JSONTYPE_ARRAY:
            JsonNode_ArrayPushC(*pparent, jn);
            JsonNode_Free(jn);
            jn = NULL;
            break;
          default:
            JsonNode_Free(jn);
            jn = NULL;
            if( mode & JSONPARSER_MODE_PRINT_ERROR ) {
              print_error("Primitive node must not have any element.");
            }
            goto ERROR;
          }
        }
        else {
          JsonNode_Free(jn);
          jn = NULL;
          if( mode & JSONPARSER_MODE_PRINT_ERROR ) {
            print_error("Node is null.");
          }
          goto ERROR;
        }
      }
      /* end of if( jn != NULL ) */
    }
  }

FORCE_TO_BUILD:
  if( !(mode & JSONPARSER_MODE_BUILD_ON_ERROR) ) {
    goto ERROR;
  }
  while( jpsr->stack->length > 0 ) {
    JsonNode *parent = NULL;
    if( DataList_Pop(jpsr->stack, &parent, 1) != 1 || parent == NULL ) {
      goto ERROR;
    }
    if( jn != NULL ) {
      switch( parent->type ) {
      case JSONTYPE_OBJECT:
        JsonPull_CopyKey(jpsr->jsonpull, 1024, key, &keylen);
        JsonNode_ObjectSetC(parent, key, jn);
        JsonNode_Free(jn);
        jn = NULL;
        break;
      case JSONTYPE_ARRAY:
        JsonNode_ArrayPushC(parent, jn);
        JsonNode_Free(jn);
        jn = NULL;
        break;
      default:
        JsonNode_Free(parent);
        if( mode & JSONPARSER_MODE_PRINT_ERROR ) {
          print_error("Node stack has at least one primitive node.");
        }
        goto ERROR;
      }
    }
    jn = parent;
  }
  *pnode = jn;
  return 0;

ERROR:
  if( jn != NULL ) {
    JsonNode_Free(jn);
  }
  JsonParser_ClearStack(jpsr);
  *pnode = NULL;
  return 0;
}

#endif /* jsonparser.c */

/*
// cc -g printerror.c datalistcell.c datalist.c textencoder.c textdecoder.c textencoding.c jsonlex.c jsonpull.c jsonnode.c jsonparser.c -lm
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

  JsonParser *jsonparser;
  JsonNode *pnode;
  TextEncoding *te = TextEncoding_New_UTF8();
  jsonparser = JsonParser_New(te);
  jsonparser->jsonpull->lex->allow_notquoted_text = 0;

  JsonPull_AppendText(jsonparser->jsonpull, text, strlen(text));

  JsonParser_Parse(jsonparser, &pnode);

  printf("%p\n", pnode);

  FREE(pnode);
}
*/

