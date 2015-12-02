#ifndef _JSONNODE_HEAD_
#define _JSONNODE_HEAD_

#ifndef _JSONNODE_EXTERN
#define _JSONNODE_EXTERN extern
#endif

#include "jsontype.h"

struct _JsonNode {
  int type;
  void *ptr;
};

typedef struct _JsonNode JsonNode;

struct _JsonKeyValuePair {
  wchar_t *key;
  JsonNode *node;
};
typedef struct _JsonKeyValuePair JsonKeyValuePair;

#define JsonNode_IsNull(jn) (\
  (jn)->type == JSONTYPE_NULL \
)

#define JsonNode_IsPrimitive(jn) (\
  (jn)->type == JSONTYPE_NULL \
  || (jn)->type == JSONTYPE_INT \
  || (jn)->type == JSONTYPE_REAL \
  || (jn)->type == JSONTYPE_STRING \
  || (jn)->type == JSONTYPE_BOOLEAN \
)

#define JsonNode_IsArray(jn) (\
  (jn)->type == JSONTYPE_ARRAY \
)

#define JsonNode_IsObject(jn) (\
  (jn)->type == JSONTYPE_OBJECT \
)


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

_JSONNODE_EXTERN JsonNode *JsonNode_NewLiteral(const wchar_t *literal);
_JSONNODE_EXTERN JsonNode *JsonNode_NewInt(long value);
_JSONNODE_EXTERN JsonNode *JsonNode_NewIntFromText(const wchar_t *text);
_JSONNODE_EXTERN JsonNode *JsonNode_NewReal(double value);
_JSONNODE_EXTERN JsonNode *JsonNode_NewRealFromText(const wchar_t *text);
_JSONNODE_EXTERN JsonNode *JsonNode_NewString(const wchar_t *value);
_JSONNODE_EXTERN JsonNode *JsonNode_NewBoolean(int value);
_JSONNODE_EXTERN JsonNode *JsonNode_NewNull();
_JSONNODE_EXTERN JsonNode *JsonNode_NewArray();
_JSONNODE_EXTERN JsonNode *JsonNode_NewObject();
_JSONNODE_EXTERN JsonNode *JsonNode_Copy(JsonNode *src);
_JSONNODE_EXTERN void JsonNode_Free(JsonNode *jn);
_JSONNODE_EXTERN int JsonNode_ArrayPushC(JsonNode *node, JsonNode *element);
_JSONNODE_EXTERN JsonNode *JsonNode_ArrayGet(JsonNode *array, int index);
_JSONNODE_EXTERN int JsonNode_ArrayLength(JsonNode *jn);
_JSONNODE_EXTERN int JsonNode_ObjectSetC(JsonNode *node, const wchar_t *key, JsonNode *element);
_JSONNODE_EXTERN JsonNode *JsonNode_ObjectGet(JsonNode *object, const wchar_t *key);
_JSONNODE_EXTERN JsonKeyValuePair *JsonNode_ObjectGetPair(JsonNode *object, int index);
_JSONNODE_EXTERN int JsonNode_ObjectLength(JsonNode *jn);

_JSONNODE_EXTERN int JsonNode_GetReal(JsonNode *jn, double *ptr);
_JSONNODE_EXTERN int JsonNode_GetInt(JsonNode *jn, long *ptr);
_JSONNODE_EXTERN int JsonNode_GetBoolean(JsonNode *jn, int *ptr);
_JSONNODE_EXTERN size_t JsonNode_GetString(JsonNode *jn, wchar_t *ptr, size_t max_count);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* jsonnode.h */
