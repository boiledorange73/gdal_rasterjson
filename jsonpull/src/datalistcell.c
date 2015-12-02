#ifndef _DATALISTCELL_C_
#define _DATALISTCELL_C_

#include <stdlib.h> /* malloc, NULL */
#include <string.h> /* memcpy */

#ifdef _DATALISTCELL_INTERNAL_
#  ifndef _PRINTERROR_INTERNAL_
#    define _PRINTERROR_INTERNAL_
#  endif
#  include "printerror.c"
#else
#  include "printerror.h"
#endif

#ifndef _DATALISTCELL_EXTERN
#  ifdef _DATALISTCELL_INTERNAL_
#    define _DATALISTCELL_EXTERN static
#  else
#    define _DATALISTCELL_EXTERN
#  endif
#endif
#include "datalistcell.h"

/* macors for malloc and free */
#ifndef MALLOC
#define MALLOC(p) (malloc((p)))
#endif
#ifndef FREE
#define FREE(p) (free((p)))
#endif

DataListCell *DataListCell_New(int bytes) {
  DataListCell *datalistcell;
  datalistcell = (DataListCell *)MALLOC(sizeof(DataListCell)+bytes);
  if( datalistcell == NULL ) {
    print_perror("malloc");
    return NULL;
  }
  datalistcell->body = (char *)datalistcell + sizeof(DataListCell);
  datalistcell->prev = NULL;
  datalistcell->next = NULL;
  return datalistcell;
}

void DataListCell_Free(DataListCell *datalistcell) {
  while( datalistcell != NULL ) {
    DataListCell *next;
    next = datalistcell->next;
    FREE(datalistcell);
    datalistcell = next;
  }
}

#endif /* datalistcell.c */
