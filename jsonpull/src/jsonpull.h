#ifndef _JSONPULL_HEAD_
#define _JSONPULL_HEAD_

#ifndef _JSONPULL_EXTERN
#define _JSONPULL_EXTERN extern
#endif

#include <stdio.h>
#include "jsonlex.h"
#include "jsonnode.h"
#include "textencoding.h"
#include "textdecoder.h"
#include "datalist.h"

#ifndef _JSONPULL_FILE
#define _JSONPULL_FILE FILE
#endif

struct _JsonPull {
  int status;
  int col;
  int row;
  int err;
  _JSONPULL_FILE *fp;
  TextDecoder *decoder;
  DataList *grammar_stack;
  DataList *key_stack; /* stack for object key */
  JsonLex *lex;
};

typedef struct _JsonPull JsonPull;

#define JSONPULL_ST_NONE (0)
#define JSONPULL_ST_ERROR (1)
#define JSONPULL_ST_EOF (2)

#define JSONPULL_EVENT_NONE (0)
#define JSONPULL_EVENT_EOF (-1)

#define JSONPULL_EVENT_BEGIN_ARRAY (0x00000001)
#define JSONPULL_EVENT_END_ARRAY (0x00000002)
#define JSONPULL_EVENT_BEGIN_OBJECT (0x00000003)
#define JSONPULL_EVENT_END_OBJECT (0x00000004)
#define JSONPULL_EVENT_OBJECT_KEY (0x00000005)

#define JSONPULL_EVENT_LITERAL (0x00000101)
#define JSONPULL_EVENT_INT (0x00000102) /* 258*/
#define JSONPULL_EVENT_REAL (0x00000103)
#define JSONPULL_EVENT_STRING (0x00000104)

#define JSONPULL_isvalue(ev) (!((ev) & 0xFFFFFE00) && ((ev) & 0x00000100))


#define JSONPULL_EVENT_ERROR (0x00010001)
#define JSONPULL_EVENT_ERROR_LEX (0x00010002)
#define JSONPULL_EVENT_ERROR_EMPTY_STACK (0x00010003)
#define JSONPULL_EVENT_ERROR_NOT_VALUE (0x00010004)
#define JSONPULL_EVENT_ERROR_ARRAY (0x00010005)
#define JSONPULL_EVENT_ERROR_OBJECT (0x00010006)
#define JSONPULL_EVENT_ERROR_MISSING_COMMA (0x00010007)
#define JSONPULL_EVENT_ERROR_UNEXPECTED_EOF (0x00010008)
#define JSONPULL_EVENT_ERROR_SYSERROR (0x00010101)

#define JSONPULL_iserror(ev) (!((ev) & 0xFFFE0000) && ((ev) & 0x00010000))

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

_JSONPULL_EXTERN JsonPull *JsonPull_New(TextEncoding *te);
_JSONPULL_EXTERN void JsonPull_Free(JsonPull *jsonpull);
_JSONPULL_EXTERN void JsonPull_Clear(JsonPull *jsonpull);
_JSONPULL_EXTERN int JsonPull_AppendText(JsonPull *jsonpull, const unsigned char *text, int count);
_JSONPULL_EXTERN void JsonPull_SetFilePointer(JsonPull *jsonpull, _JSONPULL_FILE *fp);
_JSONPULL_EXTERN int JsonPull_ReadValue(JsonPull *jsonpull, int max_valuelen, wchar_t *value, int *valuelen);
_JSONPULL_EXTERN int JsonPull_CopyKey(JsonPull *jsonpull, int max_keylen, wchar_t *key, int *keylen);
_JSONPULL_EXTERN int JsonPull_Pull(JsonPull *jsonpull);
_JSONPULL_EXTERN const char *JsonPull_ErrorReason(JsonPull *jsonpull);
_JSONPULL_EXTERN void JsonPull_PrintError(JsonPull *jsonpull);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* jsonparser.h */
