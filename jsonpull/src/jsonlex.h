#ifndef _JSONLEX_HEAD_
#define _JSONLEX_HEAD_

#ifndef _JSONLEX_EXTERN
#define _JSONLEX_EXTERN extern
#endif

#include <wchar.h>

#include "datalist.h"

struct _JsonLex {
    int st;
    int err;
    int allow_notquoted_text;
    DataList *tbuff;
};

typedef struct _JsonLex JsonLex;

#define JSONLEX_ERR_NOERROR (0x00000000)
#define JSONLEX_ERR_STAY (0x00000001)
#define JSONLEX_ERR_ERROR (0x00010000)
#define JSONLEX_ERR_INVALID_CHARACTER (0x00010001)
#define JSONLEX_ERR_NOT_TERMINATED_STRING (0x00010002)
#define JSONLEX_ERR_NOT_TERMINATED_NUMBER (0x00010003)
#define JSONLEX_ERR_INVALID_LITERAL (0x00010004)

#define JSONLEX_iserror(stat) ((stat) & 0x00010000)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

_JSONLEX_EXTERN JsonLex *JsonLex_New();
_JSONLEX_EXTERN void JsonLex_Free(JsonLex *);
_JSONLEX_EXTERN void JsonLex_Clear(JsonLex *lex);
_JSONLEX_EXTERN int JsonLex_NewChar(JsonLex *, wchar_t *, int *);
_JSONLEX_EXTERN int JsonLex_Finish(JsonLex *, int *);
_JSONLEX_EXTERN const char *JsonLex_ErrorReason(JsonLex *lex);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* jsonpull_lex.h */