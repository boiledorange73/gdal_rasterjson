#ifndef _DATALISTCELL_HEAD_
#define _DATALISTCELL_HEAD_

#ifndef _DATALISTCELL_EXTERN
#define _DATALISTCELL_EXTERN extern
#endif

struct _DataListCell {
  void *body;
  struct _DataListCell *prev;
  struct _DataListCell *next;
};
typedef struct _DataListCell DataListCell;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

_DATALISTCELL_EXTERN DataListCell *DataListCell_New(int bytes);
_DATALISTCELL_EXTERN void DataListCell_Free(DataListCell *datalistcell);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* datalist.h */
