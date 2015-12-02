#ifndef _JSONPARSER_HEAD_
#define _JSONPARSER_HEAD_

#ifndef _JSONPARSER_EXTERN
#define _JSONPARSER_EXTERN extern
#endif

#include <stdio.h>

#include "jsonpull.h"

struct _JsonParser {
  TextEncoding *te;
  JsonPull *jsonpull;
  DataList *stack;
  JsonNode *node;
};
typedef struct _JsonParser JsonParser;

#define JSONPARSER_MODE_BUILD_ON_ERROR (0x00001000)
#define JSONPARSER_MODE_PRINT_ERROR    (0x00000001)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

_JSONPARSER_EXTERN JsonParser *JsonParser_New(TextEncoding *te);
_JSONPARSER_EXTERN void JsonParser_Clear(JsonParser *jpsr);
_JSONPARSER_EXTERN void JsonParser_ClearStack(JsonParser *jpsr);
_JSONPARSER_EXTERN void JsonParser_Free(JsonParser *jpsr);
_JSONPARSER_EXTERN int JsonParser_AppendText(JsonParser *jpsr, const unsigned char* text, int count);
_JSONPARSER_EXTERN void JsonParser_SetFilePointer(JsonParser *jpsr, _JSONPULL_FILE *fp);
_JSONPARSER_EXTERN int JsonParser_Parse(JsonParser *jpsr, JsonNode **pnode, int mode);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* jsonparser.h */
