#ifndef _JSONPRINTER_C_
#define _JSONPRINTER_C_

#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef _JSONPRINTER_INTERNAL_
#  ifndef _JSONNODE_INTERNAL_
#    define _JSONNODE_INTERNAL_
#  endif
#  include "jsonnode.c"
#  ifndef _TEXTENCODING_INTERNAL_
#    define _TEXTENCODING_INTERNAL_
#  endif
#  include "textencoding.c"
#  ifndef _TEXTENCODER_INTERNAL_
#    define _TEXTENCODER_INTERNAL_
#  endif
#  include "textencoder.c"
#  ifndef _TEXTENCODER_APPENDF_INTERNAL_
#    define _TEXTENCODER_APPENDF_INTERNAL_
#  endif
#  include "textencoder_appendf.c"
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
#  include "textencoder_appendf.h"
#  include "jsonnode.h"
#endif

#ifndef _JSONPRINTER_EXTERN
#  ifdef _JSONPRINTER_INTERNAL_
#    define _JSONPRINTER_EXTERN static
#  else
#    define _JSONPRINTER_EXTERN
#  endif
#endif
#include "jsonprinter.h"

/* macors for malloc and free */
#ifndef MALLOC
#define MALLOC(p) (malloc((p)))
#endif
#ifndef FREE
#define FREE(p) (free((p)))
#endif

int JsonPrinter_Print(TextEncoder *ter, _TEXTENCODER_FILE *fp, JsonNode *node, wchar_t indent_char, int indent_current, int indent_skip) {
  int ret = 0;
  int nodetype;
  int n, len;
  wchar_t sep;

  nodetype = JSONTYPE_NULL;
  if( node != NULL ) {
    nodetype = node->type;
  }

  switch( nodetype ) {
  case JSONTYPE_INT:
    TextEncoder_AppendF(ter, L"%ld", *(long *)(node->ptr));
    ret += TextEncoder_FlashToFile(ter, fp);
    break;
  case JSONTYPE_REAL:
    TextEncoder_AppendF(ter, L"%f", *(double *)(node->ptr));
    ret += TextEncoder_FlashToFile(ter, fp);
    break;
  case JSONTYPE_STRING:
    TextEncoder_AppendChar(ter, L'\"');
    TextEncoder_AppendZ(ter, (wchar_t *)(node->ptr));
    TextEncoder_AppendChar(ter, L'\"');
    ret += TextEncoder_FlashToFile(ter, fp);
    break;
  case JSONTYPE_BOOLEAN:
    TextEncoder_AppendZ(ter, *(int *)(node->ptr) ? L"true" : L"false");
    ret += TextEncoder_FlashToFile(ter, fp);
    break;
  case JSONTYPE_ARRAY:
    TextEncoder_AppendZ(ter, L"[\n");
    if( indent_char != L'\0' ) {
      TextEncoder_AppendCharRepeat(ter, indent_char, indent_current+indent_skip);
    }
    len = JsonNode_ArrayLength(node);
    for( n = 0; n < len; n++ ) {
      if( n > 0 ) {
        TextEncoder_AppendZ(ter, L", ");
      }
      JsonPrinter_Print(ter, fp, JsonNode_ArrayGet(node, n), indent_char, indent_current+indent_skip, indent_skip);
    }
    TextEncoder_AppendZ(ter, L"\n");
    if( indent_char != L'\0' ) {
      TextEncoder_AppendCharRepeat(ter, indent_char, indent_current);
    }
    TextEncoder_AppendChar(ter, L']');
    ret += TextEncoder_FlashToFile(ter, fp);
    break;
  case JSONTYPE_OBJECT:
    TextEncoder_AppendZ(ter, L"{\n");
    if( indent_char != L'\0' ) {
      TextEncoder_AppendCharRepeat(ter, indent_char, indent_current);
    }
    len = JsonNode_ObjectLength(node);
    for( n = 0; n < len; n++ ) {
      JsonKeyValuePair *pair;
      if( n > 0 ) {
        TextEncoder_AppendZ(ter, L",\n");
        ret += TextEncoder_FlashToFile(ter, fp);
      }
      pair = JsonNode_ObjectGetPair(node, n);
      if( pair != NULL ) {
        /* key */
        if( indent_char != L'\0' ) {
          TextEncoder_AppendCharRepeat(ter, indent_char, indent_current+indent_skip);
        }
        TextEncoder_AppendF(ter, L"\"%s\": ", pair->key);
/*
        TextEncoder_AppendChar(ter, L'\"');
        TextEncoder_AppendZ(ter, pair->key);
        TextEncoder_AppendChar(ter, L'\"');
        TextEncoder_AppendZ(ter, L": ");
*/
        /* value */
        JsonPrinter_Print(ter, fp, pair->node, indent_char, indent_current+indent_skip, indent_skip);
      }
    }
    TextEncoder_AppendChar(ter, L'\n');
    if( indent_char != L'\0' ) {
      TextEncoder_AppendCharRepeat(ter, indent_char, indent_current);
    }
    TextEncoder_AppendChar(ter, L'}');
    ret += TextEncoder_FlashToFile(ter, fp);
    break;
  default:
    /* including JSONTYPE_NULL */
    TextEncoder_AppendZ(ter, L"null");
    ret += TextEncoder_FlashToFile(ter, fp);
    break;

  }
  return ret;
}

#endif /* jsonprinter.c */

/*
// cc -g printerror.c datalistcell.c datalist.c textencoder.c textdecoder.c textencoding.c jsonlex.c jsonpull.c jsonnode.c jsonparser.c jsonprinter.c -lm
#include <stdio.h>
#include <wchar.h>
#include <string.h> // strlen
#include "jsonparser.h"

int main(int ac, char *av[]) {

  static wchar_t value[1025];
  int valuelen;

  static wchar_t key[1025];
  int keylen;

  char *text =
    "{\n"
    "  \"ほげ\": 123,\n"
    "  \"b\": [\"hoge\", [false, true]],\n"
    "  \"c\": [1.2,true,3]\n"
    "}\n";
text="{\"type\": \"raster\",";

  JsonParser *jsonparser;
  JsonNode *pnode;
  TextEncoding *te = TextEncoding_New_UTF8();
  TextEncoder *ter = TextEncoding_NewEncoder(te);
  jsonparser = JsonParser_New(te);
  jsonparser->jsonpull->lex->allow_notquoted_text = 0;

  JsonPull_AppendText(jsonparser->jsonpull, text, strlen(text));

  JsonPraser_Parse(jsonparser, &pnode);

  JsonPrinter_Print(ter, stdout, pnode, L'\t', 0, 1);

  TextEncoder_AppendChar(ter, L'\n');
  TextEncoder_FlashToFile(ter, stdout);

  TextEncoder_Free(ter);
  TextEncoding_Free(te);

  FREE(pnode);

  return 0;
}
*/

