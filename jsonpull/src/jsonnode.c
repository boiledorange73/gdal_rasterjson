#ifndef _JSONNODE_C_
#define _JSONNODE_C_

#include <wchar.h>
#include <stdlib.h> /* malloc */
#include <string.h> /* strlen */
#include <stdio.h> /* snprintf */


#ifdef _JSONNODE_INTERNAL_
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
#endif

#ifndef _JSONNODE_EXTERN
#  ifdef _JSONNODE_INTERNAL_
#    define _JSONNODE_EXTERN static
#  else
#    define _JSONNODE_EXTERN
#  endif
#endif
#include "jsonnode.h"

/* macors for malloc and free */
#ifndef MALLOC
#define MALLOC(p) (malloc((p)))
#endif
#ifndef FREE
#define FREE(p) (free((p)))
#endif

/* macro for wcscasecmp */
#if __POSIX_VISIBLE >= 200809
  #define WCSCASECMP(A1,A2) (wcscasecmp((A1),(A2)))
#else
  #define WCSCASECMP(A1,A2) (my_wcscasecmp((A1),(A2)))

  static int my_wcscasecmp(const wchar_t *a1, const wchar_t *a2) {
    wchar_t c1, c2;
    if( a1 == a2 ) {
      return 0;
    }
    do {
      c1 = *a1++;
      c2 = *a2++;
      // break if a1 arrives at terminater.
      if( c1 == L'\0' ) {
        break;
      }
      // tolower if c1 is lower ASCII letter.
      if( c1 >= L'a' && c1 <= L'z' ) {
        c1 = c1 - L'a' + L'A';
      }
      // tolower if c2 is lower ASCII letter.
      if( c2 >= L'a' && c2 <= L'z' ) {
        c2 = c2 - L'a' + L'A';
      }
    }
    while( c1 == c2 );
    // end of loop
    return c1 - c2;
  }
#endif

/* ---------------------------------------------------------------- internal */

static size_t long2wcsn(long value, wchar_t *ptr, size_t max_count) {
  wchar_t *p;
  wchar_t *t;
  wchar_t *q1, *q2;
  int with_minus = 0;

  if( !(max_count > 0) ) {
    return 0;
  }

  p = ptr;
  t = p + max_count;

  if( value == 0 ) {
    *(p++) = L'0';
    *p = L'\0';
    return 1;
  }

  if( value < 0 ) {
    with_minus = 1;
    value = -value;
    t = t - 1;
  }
  while( value > 0 && p < t ) {
    *(p++) = (value % 10) + L'0';
    value = value / 10;
  }
  if( with_minus ) {
    *(p++) = '-';
  }
  for( q1 = ptr, q2 = p - 1; q1 < q2; q1++, q2-- ) {
    wchar_t a;
    a = *q1;
    *q1 = *q2;
    *q2 = a;
  }
  *p = L'\0';
  return p - ptr;
}

static size_t double2wcsn(double value, wchar_t *ptr, size_t max_count) {
  int len;
  int n;
  len = snprintf((char *)ptr, max_count, "%f", value);
  if( !(len > 0) ) {
    ptr[0] = L'\0';
    return 0;
  }
  len = strlen((char *)ptr);
  for( n = len + 1; n >= 1; n-- ) { /* including null terminator */
    *(ptr + n-1) = *(((char *)ptr) + n-1);
  }
  return len;
}


/* key_p and node_p msut be pointers to heap mem. */
/* JsonKeyValuePair_Free frees key and node */
static JsonKeyValuePair *JsonKeyValuePair_New(wchar_t *key_p, JsonNode *jn_p) {
  JsonKeyValuePair *joe;
  joe = (JsonKeyValuePair *)MALLOC(sizeof(JsonKeyValuePair));
  if( joe == NULL ) {
    return NULL;
  }
  joe->key = key_p;
  joe->node = jn_p;
  return joe;
}

static void JsonKeyValuePair_Free(JsonKeyValuePair *joe) {
  FREE(joe->key);
  JsonNode_Free(joe->node);
  FREE(joe);
}

static JsonNode *JsonNode_NewPrimitive(int type, const void *ptr, size_t valuebytes) {
  JsonNode *jn;
  if( ptr == NULL ) {
    valuebytes = 0;
  }
  char *p = (char *)MALLOC(sizeof(JsonNode) + valuebytes);
  if( p == NULL ) {
    print_perror("malloc");
    return NULL;
  }
  jn = (JsonNode *)p;
  jn->type = type;
  if( ptr == NULL || valuebytes == 0) {
    jn->ptr = NULL;
  }
  else {
    jn->ptr = (p + sizeof(JsonNode));
    memcpy(jn->ptr, ptr, valuebytes);
  }
  return jn;
}

/* ---------------------------------------------------------------- global */
JsonNode *JsonNode_NewLiteral(const wchar_t *literal) {
  if( literal != NULL ) {
    if( WCSCASECMP(literal, L"true") ) {
      return JsonNode_NewBoolean(1);
    }
    if( WCSCASECMP(literal, L"false") ) {
      return JsonNode_NewBoolean(0);
    }
  }
  return JsonNode_NewNull();
}

JsonNode *JsonNode_NewInt(long value) {
  return JsonNode_NewPrimitive(JSONTYPE_INT, &value, sizeof(long));
}

JsonNode *JsonNode_NewIntFromText(const wchar_t *text) {
  long value;
  if( swscanf( text, L"%ld", &value) != 1 ) {
    value = 0;
  }
  return JsonNode_NewPrimitive(JSONTYPE_INT, &value, sizeof(long));
}

JsonNode *JsonNode_NewReal(double value) {
  return JsonNode_NewPrimitive(JSONTYPE_REAL, &value, sizeof(double));
}

JsonNode *JsonNode_NewRealFromText(const wchar_t *text) {
  double value;
  if( swscanf( text, L"%lf", &value) != 1 ) {
    value = 0;
  }
  return JsonNode_NewPrimitive(JSONTYPE_REAL, &value, sizeof(double));
}


JsonNode *JsonNode_NewString(const wchar_t *value) {
  size_t valuelen;
  valuelen = wcslen(value);
  return JsonNode_NewPrimitive(JSONTYPE_STRING, value, sizeof(wchar_t) * (valuelen + 1));
}

JsonNode *JsonNode_NewBoolean(int value) {
  value = value ? 1 : 0;
  return JsonNode_NewPrimitive(JSONTYPE_BOOLEAN, &value, sizeof(int));
}

JsonNode *JsonNode_NewNull() {
  return JsonNode_NewPrimitive(JSONTYPE_NULL, NULL, 0);
}

JsonNode *JsonNode_NewArray() {
  JsonNode *jn;
  jn = (JsonNode *)MALLOC(sizeof(JsonNode));
  if( jn == NULL ) {
    print_perror("malloc");
    return NULL;
  }
  jn->type = JSONTYPE_ARRAY;
  jn->ptr = DataList_New(2, sizeof(JsonNode *));
  if( jn->ptr == NULL ) {
    FREE(jn);
    return NULL;
  }
  return jn;
}

JsonNode *JsonNode_NewObject() {
  JsonNode *jn;
  jn = (JsonNode *)MALLOC(sizeof(JsonNode));
  if( jn == NULL ) {
    print_perror("malloc");
    return NULL;
  }
  jn->type = JSONTYPE_OBJECT;
  jn->ptr = DataList_New(1, sizeof(JsonKeyValuePair *));
  if( jn->ptr == NULL ) {
    FREE(jn);
    return NULL;
  }
  return jn;
}

JsonNode *JsonNode_Copy(JsonNode *jn) {
  int len, n;
  JsonNode *ret;

  switch( jn->type ) {
  case JSONTYPE_INT:
    return JsonNode_NewInt(*((long *)(jn->ptr)));
  case JSONTYPE_REAL:
    return JsonNode_NewReal(*((double *)(jn->ptr)));
  case JSONTYPE_STRING:
    return JsonNode_NewString((wchar_t *)(jn->ptr));
  case JSONTYPE_BOOLEAN:
    return JsonNode_NewBoolean(*((int *)(jn->ptr)));
  case JSONTYPE_ARRAY:
    ret = JsonNode_NewArray();
    len = JsonNode_ArrayLength(jn);
    for( n = 0; n < len; n++ ) {
      JsonNode_ArrayPushC(ret, JsonNode_ArrayGet(jn, n));
    }
    return ret;
  case JSONTYPE_OBJECT:
    ret = JsonNode_NewObject();
    len = JsonNode_ObjectLength(jn);
    for( n = 0; n < len; n++ ) {
      JsonKeyValuePair *kvpsrc;
      kvpsrc = JsonNode_ObjectGetPair(jn, n);
      JsonNode_ObjectSetC(ret, kvpsrc->key, kvpsrc->node);
    }
    return ret;
  default: /* including null */
    return JsonNode_NewNull();
  }
}

void JsonNode_Free(JsonNode *jn) {
  if( jn->type == JSONTYPE_ARRAY && jn->ptr != NULL ) {
    /* frees elements */
    JsonNode *e;
    while( ((DataList *)(jn->ptr))->length > 0 ) {
      if( DataList_Shift((DataList *)(jn->ptr), &e, 1) > 0 && e != NULL ) {
        JsonNode_Free(e);
      }
    }
    DataList_Free((DataList *)(jn->ptr));
  }
  else if( jn->type == JSONTYPE_OBJECT && jn->ptr != NULL) {
    /* frees elements */
    JsonKeyValuePair *kvp;
    while( ((DataList *)(jn->ptr))->length > 0 ) {
      if( DataList_Shift((DataList *)(jn->ptr), &kvp, 1) > 0 ) {
        if( kvp != NULL ) {
          JsonKeyValuePair_Free(kvp);
        }
      }
    }
    DataList_Free((DataList *)(jn->ptr));
  }
  else if( JsonNode_IsPrimitive(jn) ) {
    /* DOES NOTHING */
    /* The regions where jn points and jn->ptr points */
    /*   are allocated at one time. */
  }
  else if( jn->ptr != NULL ) {
      FREE(jn->ptr);
  }
  FREE(jn);
}

int JsonNode_ArrayPushC(JsonNode *jn, JsonNode *element) {
  if(jn->type != JSONTYPE_ARRAY ) {
    return 0;
  }
  JsonNode *copied = JsonNode_Copy(element);
  return DataList_Push((DataList *)(jn->ptr), &copied, 1);
}

JsonNode *JsonNode_ArrayGet(JsonNode *jn, int index) {
  if(jn->type != JSONTYPE_ARRAY ) {
    return NULL;
  }
  return *(JsonNode **)DataList_Get((DataList *)(jn->ptr), index);
}

int JsonNode_ArrayLength(JsonNode *jn) {
  if(jn->type != JSONTYPE_ARRAY ) {
    return -1;
  }
  return ((DataList *)(jn->ptr))->length;
}

int JsonNode_ObjectSetC(JsonNode *jn, const wchar_t *key, JsonNode *element) {
  /* used if new element is appended */
  JsonKeyValuePair *newelement;
  wchar_t *newkey;
  JsonNode *newnode;

  size_t length;
  size_t n;

  if(jn->type != JSONTYPE_OBJECT ) {
    return 0;
  }

  length = ((DataList *)(jn->ptr))->length;
  for( n = 0; n < length; n++ ) {
    JsonKeyValuePair **pp;
    pp = (JsonKeyValuePair **)DataList_Get((DataList *)(jn->ptr), n);
    if( pp != NULL && *pp != NULL && (*pp)->key != NULL && wcscmp((*pp)->key, key) == 0 ) {
      /* hit */
      JsonNode *newnode;
      /* Constructs copied node */
      newnode = JsonNode_Copy(element);
      if( newnode == NULL ) {
        return 0;
      }
      /* Frees old JsonNode */
      JsonNode_Free((*pp)->node);
      /* writes newnode */
      (*pp)->node = newnode;
      return 1;
    }
  }
  /* Appends */
  if( (newkey = wcsdup(key)) == NULL ) {
    print_perror("wcsdup");
    return 0;
  }
  if( (newnode = JsonNode_Copy(element)) == NULL ) {
    FREE(newkey);
    return 0;
  }
  newelement = JsonKeyValuePair_New(newkey, newnode);
  if( newelement == NULL ) {
    FREE(newkey);
    JsonNode_Free(newnode);
    return 0;
  }
  return DataList_Push((DataList *)(jn->ptr), &newelement, 1) > 0 ? 1 : 0;
}

JsonNode *JsonNode_ObjectGet(JsonNode *jn, const wchar_t *key) {
  size_t length;
  size_t n;
  if(jn->type != JSONTYPE_OBJECT ) {
    return NULL;
  }
  length = ((DataList *)(jn->ptr))->length;
  for( n = 0; n < length; n++ ) {
    JsonKeyValuePair **pp;
    pp = (JsonKeyValuePair **)DataList_Get((DataList *)(jn->ptr), n);
    if( pp != NULL && *pp != NULL && (*pp)->key != NULL && wcscmp((*pp)->key, key) == 0 ) {
      /* hit */
      return (*pp)->node;
    }
  }
  return NULL;
}

JsonKeyValuePair *JsonNode_ObjectGetPair(JsonNode *jn, int index) {
  JsonKeyValuePair **ptr;
  ptr = (JsonKeyValuePair **)DataList_Get((DataList *)(jn->ptr), index);
  return ptr != NULL ? *ptr : NULL;
}

int JsonNode_ObjectLength(JsonNode *jn) {
  if(jn->type != JSONTYPE_OBJECT ) {
    return -1;
  }
  return ((DataList *)(jn->ptr))->length;
}

int JsonNode_GetReal(JsonNode *jn, double *ptr) {
  switch( jn->type ) {
  case JSONTYPE_REAL:
    *ptr = (double)(*((double *)(jn->ptr)));
    return 1;
  case JSONTYPE_INT:
    *ptr = (double)(*((long *)(jn->ptr)));
    return 1;
  case JSONTYPE_BOOLEAN:
    *ptr = *((int *)(jn->ptr)) != 0 ? 1.0 : 0.0;
    return 1;
  case JSONTYPE_STRING:
    return swscanf((wchar_t *)(jn->ptr), L"%f", ptr);
  case JSONTYPE_NULL:
    *ptr = 0.0;
    return 1;
  default:
    return 0;
  }
}

int JsonNode_GetInt(JsonNode *jn, long *ptr) {
  switch( jn->type ) {
  case JSONTYPE_REAL:
    *ptr =(long)(*((double *)(jn->ptr)));
    return 1;
  case JSONTYPE_INT:
    *ptr = (long)(*((long *)(jn->ptr)));
    return 1;
  case JSONTYPE_BOOLEAN:
    *ptr = *((int *)(jn->ptr)) != 0 ? 1 : 0;
    return 1;
  case JSONTYPE_STRING:
    return swscanf((wchar_t *)(jn->ptr), L"%ld", ptr);
  case JSONTYPE_NULL:
    *ptr = 0;
    return 1;
  default:
    return 0;
  }
}

int JsonNode_GetBoolean(JsonNode *jn, int *ptr) {
  switch( jn->type ) {
  case JSONTYPE_REAL:
    *ptr = *((double *)(jn->ptr)) != 0 ? 1 : 0;
    return 1;
  case JSONTYPE_INT:
    *ptr = *((long *)(jn->ptr)) != 0 ? 1 : 0;
    return 1;
  case JSONTYPE_BOOLEAN:
    *ptr = *((int *)(jn->ptr)) != 0 ? 1 : 0;
    return 1;
  case JSONTYPE_STRING:
    if( WCSCASECMP((wchar_t *)(jn->ptr), L"true") == 0 ) {
      *ptr = 1;
      return 1;
    }
    if( WCSCASECMP((wchar_t *)(jn->ptr), L"false") == 0 ) {
      *ptr = 0;
      return 1;
    }
    {
      long intvalue;
      int n;
      n = JsonNode_GetInt(jn, &intvalue) ;
      if( JsonNode_GetInt(jn, &intvalue) == 1 ) {
        *ptr = !!intvalue;
        return 1;
      }
    }
    if( ((wchar_t *)jn->ptr)[0] != L'\0' ) {
      *ptr = 1;
      return 1;
    }
    *ptr = 0;
    return 1;
  case JSONTYPE_NULL:
    *ptr = 0;
    return 1;
  default:
    return 0;
  }
}

size_t JsonNode_GetString(JsonNode *jn, wchar_t *ptr, size_t max_count) {
  switch( jn->type ) {
  case JSONTYPE_REAL:
    return double2wcsn(*(double *)(jn->ptr), ptr, max_count);
  case JSONTYPE_INT:
    return long2wcsn(*(long *)(jn->ptr), ptr, max_count);
  case JSONTYPE_BOOLEAN:
    if( (int *)(jn->ptr) != 0 ) {
      wcsncpy(ptr, L"true", max_count >= 4 ? 4 : max_count);
      return wcslen(ptr);
    }
    else {
      wcsncpy(ptr, L"false", max_count >= 5 ? 5 : max_count);
      return wcslen(ptr);
    }
  case JSONTYPE_STRING:
    wcsncpy(ptr, ((wchar_t *)jn->ptr), max_count);
    return wcslen(ptr);
  case JSONTYPE_NULL:
    wcsncpy(ptr, L"null", max_count >= 4 ? 4 : max_count);
    return wcslen(ptr);
  default:
    return 0;
  }
}

#endif /* jsonnode.c */

/*
// cc -g printerror.c datalistcell.c datalist.c jsonnode.c
int main(int ac, char *av[]) {
  JsonNode *jn;
  wchar_t buff[1025];
  int vbool = 0;
  long vint = 0;
  double vreal = 0;

  jn = JsonNode_NewString(L"a");

  JsonNode_GetInt(jn, &vint);
  printf("%ld\n", vint);

  JsonNode_GetBoolean(jn, &vbool);
  printf("%s\n", vbool ? "true" : "false");

  FREE(jn);

  jn = JsonNode_NewRealFromText(L"-1.5e-2");
  JsonNode_GetReal(jn, &vreal);
  printf("%f\n", vreal);

  FREE(jn);

  return 0;
}
*/

