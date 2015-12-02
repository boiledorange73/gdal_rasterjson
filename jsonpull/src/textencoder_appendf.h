#ifndef _TEXTENCODER_APPENDF_HEAD_
#define _TEXTENCODER_APPENDF_HEAD_

#include <wchar.h>
#include <stdlib.h> /* malloc */
#include <stdio.h> /* FILE */
#include <limits.h>

#include "textencoder.h"

#ifndef _TEXTENCODER_APPENDF_EXTERN
#define _TEXTENCODER_APPENDF_EXTERN extern
#endif

/* FLAG */
#define APPENDF_F_MINUS (0x00000001)
#define APPENDF_F_PLUS  (0x00000002)
#define APPENDF_F_SPACE (0x00000004)
#define APPENDF_F_ZERO  (0x00000008)
#define APPENDF_F_HASH  (0x00000010)

/* LENGTH MODIFIER */
#define APPENDF_L_hh    (0x00010001)
#define APPENDF_L_h     (0x00010002)
#define APPENDF_L_l     (0x00010003)
#define APPENDF_L_ll    (0x00010004)
#define APPENDF_L_L     (0x00010005)
#define APPENDF_L_z     (0x00010006)
#define APPENDF_L_j     (0x00010007)
#define APPENDF_L_t     (0x00010008)
#define APPENDF_L_NONE  (0x00010000)

/* CONVERSION SPECIFIER */
#define APPENDF_C_literal (0x00020000)
#define APPENDF_C_d     (0x00020001)
#define APPENDF_C_u     (0x00020003)
#define APPENDF_C_f     (0x00020005)
#define APPENDF_C_F     (0x00020006)
#define APPENDF_C_e     (0x00020007)
#define APPENDF_C_E     (0x00020008)
#define APPENDF_C_g     (0x00020009)
#define APPENDF_C_G     (0x0002000A)
#define APPENDF_C_x     (0x0002000B)
#define APPENDF_C_X     (0x0002000C)
#define APPENDF_C_o     (0x0002000D)
#define APPENDF_C_s     (0x0002000F)
#define APPENDF_C_c     (0x00020011)
#define APPENDF_C_p     (0x00020013)
#define APPENDF_C_a     (0x00020015)
#define APPENDF_C_A     (0x00020016)
#define APPENDF_C_n     (0x00020017)
#define APPENDF_C_percent   (0x0002FFFF)

struct _AppendFFormatCell {
  int conversion; /* APPENDF_C_? or 0 - simple text */
  int flag;
  int fieldwidth;
  int precision;
  int lengthmodifier;
  const wchar_t *head;
  const wchar_t *tail;
};

typedef struct _AppendFFormatCell AppendFFormatCell;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

_TEXTENCODER_APPENDF_EXTERN void AppendFFormatCell_Clear(AppendFFormatCell *cell);

_TEXTENCODER_APPENDF_EXTERN int TextEncoder_AppendFCell(TextEncoder *ter, void *pvalue, AppendFFormatCell *pfc);

/**
 * Appends formatted values.
 * @param ter TextEncoder instance.
 * @param fmt Format, like ISO 9899. Length modifies "hh" or "h", or conversion specifier "a", "A", "p" or "n" is not acceptable.
 * @returns Bytes of encoded text.
 *
 */
_TEXTENCODER_APPENDF_EXTERN int TextEncoder_AppendF(TextEncoder *ter, const wchar_t *fmt, ...);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* textencoder_appendf.h */
