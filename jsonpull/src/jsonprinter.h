#ifndef _JSONPRINTER_HEAD_
#define _JSONPRINTER_HEAD_

#ifndef _JSONPRINTER_EXTERN
#define _JSONPRINTER_EXTERN extern
#endif

#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>

#include "textencoder.h"
#include "jsonnode.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

_JSONPRINTER_EXTERN int JsonPrinter_Print(TextEncoder *ter, _TEXTENCODER_FILE *fp, JsonNode *node, wchar_t indent_char, int indent_current, int indent_skip);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* jsonprinter.h */
