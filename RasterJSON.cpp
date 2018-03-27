#include "gdal_pam.h"
#include "cpl_string.h"
#include "ogr_spatialref.h"

#include <wchar.h> // wcscmp

// VISLFILE
#define _JSONPULL_FILE VSILFILE
#define _JSONPULL_GETSTR(buff,bytes,fp) (VSIFReadL((buff),1,(bytes),(fp)))
#define _JSONPULL_EOF(fp) (VSIFEofL((fp)))

#define _TEXTENCODER_FILE VSILFILE
#define _TEXTENCODER_PUTCHAR(c,fp) (VSIFPutc((c),(fp)))


#define _TEXTENCODER_APPENDF_INTERNAL_
#include "textencoder_appendf.c"

#define _JSONPARSER_INTERNAL_
#include "jsonparser.c"

#define _JSONPARSER_INTERNAL_
#include "jsonparser.c"

#define _RASTERJSON_EXTERN
#include "RasterJSON.h"


#define _RASTERJSON_DEBUG
/* #define _RASTERJSON_DEBUG_ALL */

#ifdef _RASTERJSON_DEBUG
#include <stdio.h>
#endif

CPL_C_START
void GDALRegister_RasterJSON();
CPL_C_END

/* ------------------------------------------------ Internal */
static int IsInt(GDALDataType eDataType) {
  return (
    eDataType == GDT_Byte
    || eDataType == GDT_Int16
    || eDataType == GDT_UInt16
    || eDataType == GDT_Int32
  );
}

/* (static) outputs float */
static void OutputFloat(TextEncoder *ter, double v, int conversion, int nPrecision) {
  AppendFFormatCell cell;
  long double floatv;
  cell.conversion = conversion;
  cell.lengthmodifier = APPENDF_L_NONE;
  cell.flag = 0;
  cell.fieldwidth = 0;
  cell.precision = nPrecision;
  cell.head = NULL;
  cell.tail = NULL;
  floatv = v;
  TextEncoder_AppendFCell(ter, &floatv, &cell);
}



/* ------------------------------------------------ RasterJSONDataset */
RasterJSONDataset::RasterJSONDataset() {

#ifdef _RASTERJSON_DEBUG_ALL
fprintf(stderr, "Constructor\n");
#endif

  papszPrj = NULL;
  pszProjection = CPLStrdup("");
  adfGeoTransform[0] = 0.0;
  adfGeoTransform[1] = 1.0;
  adfGeoTransform[2] = 0.0;
  adfGeoTransform[3] = 0.0;
  adfGeoTransform[4] = 0.0;
  adfGeoTransform[5] = 1.0;

  pJsonRoot = NULL;
  pJsonValues = NULL;

#ifdef _RASTERJSON_DEBUG_ALL
fprintf(stderr, "Constructor done.\n");
#endif

}

RasterJSONDataset::~RasterJSONDataset() {
#ifdef _RASTERJSON_DEBUG_ALL
fprintf(stderr, "Destructor\n");
#endif

  FlushCache();
  CPLFree( pszProjection );
  CSLDestroy( papszPrj );
  if( pJsonRoot ) {
    JsonNode_Free(pJsonRoot);
  }

#ifdef _RASTERJSON_DEBUG_ALL
fprintf(stderr, "Destructor done.\n");
#endif
}

CPLErr RasterJSONDataset::ReadBlock(int nBand, int nBlockXOff, int nBlockYOff, void *pImage, GDALDataType eDataType) {

#ifdef _RASTERJSON_DEBUG_ALL
fprintf(stderr, "ReadBlock\n");
#endif

  // Gets the row.
  JsonNode *node;
  node = JsonNode_ArrayGet(pJsonValues, nBand-1);
  node = JsonNode_ArrayGet(node, nBlockYOff);
  //
  char *p = (char *)pImage;

  if( IsInt(eDataType) ) {
    // integer
    long v;
    for( int n = 0; n < nRasterXSize; n++ ) {
      JsonNode_GetInt(JsonNode_ArrayGet(node, n), &v);
      switch( eDataType ) {
      case GDT_Byte:
        *((char *)p) = (char)(0xff & v);
        p += 1;
        break;
      case GDT_Int16:
        *((int16_t *)p) = (int16_t)v;
        p += 2;
        break;
      case GDT_UInt16:
        *((uint16_t *)p) = (uint16_t)v;
        p += 2;
        break;
      case GDT_UInt32:
        *((uint32_t *)p) = (uint32_t)v;
        p += 4;
        break;
      default:
        *((int32_t *)p) = (int32_t)v;
        p += 4;
        break;
      }
    }
  }
  else {
    // real
    double v;
    for( int n = 0; n < nRasterXSize; n++ ) {
      JsonNode_GetReal(JsonNode_ArrayGet(node, n), &v);
      switch( eDataType ) {
      case GDT_Float32:
        *((float *)p) = (float)v;
        p += 4;
        break;
      default:
        *((double *)p) = v;
        p += 8;
        break;
      }
    }
  }

#ifdef _RASTERJSON_DEBUG_ALL
fprintf(stderr, "ReadBlock Done.\n");
#endif

  return CE_None;
}

CPLErr RasterJSONDataset::GetGeoTransform(double *adTransform) {
  memcpy( adTransform, this->adfGeoTransform, sizeof(double) * 6);
  return( CE_None );
}

const char *RasterJSONDataset::GetProjectionRef() {
  return pszProjection;
}


/* ------------------------------------------------ RasterJSONDataset static */
GDALDataset *RasterJSONDataset::Open(GDALOpenInfo * poOpenInfo) {
#ifdef _RASTERJSON_DEBUG_ALL
fprintf(stderr, "Open\n");
#endif

  RasterJSONDataset *poDS;
  JsonParser *jpsr;
  VSILFILE *fp;
//  FILE *fp;
  JsonNode *json_target;
  JsonNode *json_nodata;
  JsonNode *json_datatype;
  int bands;

  if( !Identify(poOpenInfo) ) {
    return NULL;
  }

  poDS = new RasterJSONDataset();

  fp = VSIFOpenL(poOpenInfo->pszFilename, "r");
//  fp = fopen(poOpenInfo->pszFilename, "r");
  if( fp == NULL ) {
    CPLError(CE_Failure, CPLE_OpenFailed, "VSIFOpenL(%s) failed unexpectedly.", poOpenInfo->pszFilename);
    delete poDS;
    return NULL;
  }
  // Parses All
  jpsr = JsonParser_New(NULL);
  if( jpsr == NULL ) {
    delete poDS;
    VSIFCloseL( fp );
//    fclose(fp);
    return NULL;
  }
  JsonParser_SetFilePointer(jpsr, fp);

//  if( !(JsonParser_Parse(jpsr, &(poDS->pJsonRoot), JSONPARSER_MODE_PRINT_ERROR) > 0) ) {

  int r = JsonParser_Parse(jpsr, &(poDS->pJsonRoot), JSONPARSER_MODE_PRINT_ERROR);
  if( !(r>0) ) {
    delete poDS;
    JsonParser_Free(jpsr);
    VSIFCloseL( fp );
//    fclose(fp);
    return NULL;
  }
  // Frees parser.
  JsonParser_Free(jpsr);
  // Closes file
  VSIFCloseL(fp);
//  fclose(fp);

  // reads all
  // transform (6 elements array)
  json_target = JsonNode_ObjectGet(poDS->pJsonRoot, L"transform");
  if( json_target != NULL ) {
    // exists
    if( JsonNode_ArrayLength(json_target) != 6 ) {
      CPLError(CE_Failure, CPLE_OpenFailed, "transform must be 6 elements array.");
      delete poDS;
      return NULL;
    }
    for( int n = 0; n < 6; n++ ) {
      JsonNode *e;
      if( (e = JsonNode_ArrayGet(json_target, n)) == NULL || !JsonNode_IsPrimitive(e) ) {
        CPLError(CE_Failure, CPLE_OpenFailed, "element of transform must be non-null primitive.");
        delete poDS;
        return NULL;
      }
    }
    JsonNode_GetReal(JsonNode_ArrayGet(json_target, 4), &(poDS->adfGeoTransform[0]));
    JsonNode_GetReal(JsonNode_ArrayGet(json_target, 0), &(poDS->adfGeoTransform[1]));
    JsonNode_GetReal(JsonNode_ArrayGet(json_target, 1), &(poDS->adfGeoTransform[2]));
    JsonNode_GetReal(JsonNode_ArrayGet(json_target, 5), &(poDS->adfGeoTransform[3]));
    JsonNode_GetReal(JsonNode_ArrayGet(json_target, 2), &(poDS->adfGeoTransform[4]));
    JsonNode_GetReal(JsonNode_ArrayGet(json_target, 3), &(poDS->adfGeoTransform[5]));
  }

  // values
  poDS->pJsonValues = JsonNode_ObjectGet(poDS->pJsonRoot, L"values");
  if( poDS->pJsonValues == NULL ) {
    CPLError(CE_Failure, CPLE_OpenFailed, "values is not available.");
    delete poDS;
    return NULL;
  }
  // values/bands
  bands = JsonNode_ArrayLength(poDS->pJsonValues);
  if( !(bands > 0) ) {
    CPLError(CE_Failure, CPLE_OpenFailed, "values must be array.");
    delete poDS;
    return NULL;
  }
  // values/bands/y
  json_target = JsonNode_ArrayGet(poDS->pJsonValues,0);
  poDS->nRasterYSize = JsonNode_ArrayLength(json_target);
  if( !(poDS->nRasterYSize > 0) ) {
    CPLError(CE_Failure, CPLE_OpenFailed, "values must be 3d array.");
    delete poDS;
    return NULL;
  }
  json_target = JsonNode_ArrayGet(json_target,0);
  poDS->nRasterXSize = JsonNode_ArrayLength(json_target);
  if( !(poDS->nRasterYSize > 0) ) {
    CPLError(CE_Failure, CPLE_OpenFailed, "values must be 3d array.");
    delete poDS;
    return NULL;
  }

  // crs
  json_target = JsonNode_ObjectGet(poDS->pJsonRoot, L"crs");
  if( json_target != NULL ) {
    OGRSpatialReference oSRS;
    if( JsonNode_IsPrimitive(json_target) && !JsonNode_IsNull(json_target) ) {
      if( json_target->type == JSONTYPE_INT ) {
        long v;
        if( JsonNode_GetInt(json_target, &v) > 0 ) {
          oSRS.importFromEPSG((int)v);
          oSRS.exportToWkt(&(poDS->pszProjection));
        }
        else {
          CPLError(CE_Warning, CPLE_OpenFailed, "Cannot get EPSG code from \"crs\".");
        }
      }
      else {
        int len = wcslen((const wchar_t *)json_target->ptr);
        char *encoded = (char *)CPLMalloc(sizeof(char)*(len+1));
        if( encoded == NULL ) {
          CPLError(CE_Warning, CPLE_OpenFailed, "Cannot allocate memory on \"crs\".");
        }
        else {
          /* json_target->ptr to encoded */
          char *pd = encoded;
          wchar_t *ps = (wchar_t *)json_target->ptr;
          while( *ps != L'\0' ) {
            *(pd++) = (char)(0xff & *(ps++));
          }
          *pd = '\0';
          /* compiles */
          if( CE_None == oSRS.SetFromUserInput(encoded) ) {
            oSRS.exportToWkt(&(poDS->pszProjection));
          }
          else {
            // error
            if( *encoded == '\0' ) {
              // EMPTY. Does not print error message.
            }
            else {
              CPLError(CE_Warning, CPLE_OpenFailed, "Cannot recognize \"crs\".");
            }
          }
          CPLFree(encoded);
        }
      }
    }
  }

  // nodata ($bands elements array)
  json_nodata = JsonNode_ObjectGet(poDS->pJsonRoot, L"nodata");
  if( json_nodata != NULL ) {
    if( JsonNode_ArrayLength(json_nodata) != bands ) {
      CPLError(CE_Failure, CPLE_OpenFailed, "nodata must be array whose length equals $bands.");
      json_nodata = NULL;
    }
  }

  // datatype ($bands elements array)
  json_datatype = JsonNode_ObjectGet(poDS->pJsonRoot, L"datatype");
  if( json_datatype != NULL ) {
    if( JsonNode_ArrayLength(json_datatype) != bands ) {
      CPLError(CE_Failure, CPLE_OpenFailed, "datatype must be array whose length equals $bands.");
      json_datatype = NULL;
    }
  }

  // creates bands
  for( int n = 1; n <= bands; n++ ) {
    GDALDataType eDataType = GDT_Int32;
    if( json_datatype != NULL ) {
      wchar_t typestr[65];
      JsonNode *e;
      e = JsonNode_ArrayGet(json_datatype, n-1);
      if( e != NULL && JsonNode_IsPrimitive(e) && !JsonNode_IsNull(e) ) {
        JsonNode_GetString(e, typestr, 64);
        if( wcscmp(L"byte", typestr) == 0 ) {
          eDataType = GDT_Byte;
        }
        else if( wcscmp(L"int16", typestr) == 0 ) {
          eDataType = GDT_Int16;
        }
        else if( wcscmp(L"uint16", typestr) == 0 ) {
          eDataType = GDT_UInt16;
        }
        else if( wcscmp(L"uint32", typestr) == 0 ) {
          eDataType = GDT_UInt32;
        }
        else if( wcscmp(L"float32", typestr) == 0 ) {
          eDataType = GDT_Float32;
        }
        else if( wcscmp(L"float64", typestr) == 0 ) {
          eDataType = GDT_Float64;
        }
      }
    }
    RasterJSONRasterBand *band;
    band = new RasterJSONRasterBand(poDS, n, eDataType);
    poDS->SetBand(n, band);
    if( json_nodata != NULL ) {
      JsonNode *e;
      e = JsonNode_ArrayGet(json_nodata, n-1);
      if( e != NULL && JsonNode_IsPrimitive(e) && !JsonNode_IsNull(e) ) {
        double nodatavalue;
        if( JsonNode_GetReal(e, &nodatavalue) > 0 ) {
          band->SetNoDataValue(nodatavalue);
        }
      }
    }
  }

  // other informations
  poDS->SetDescription( poOpenInfo->pszFilename );
  poDS->TryLoadXML();

#ifdef _RASTERJSON_DEBUG_ALL
fprintf(stderr, "Open: done.\n");
#endif

  return poDS;
}

GDALDataset *RasterJSONDataset::CreateCopy(
      const char * pszFilename, GDALDataset *poSrcDS,
      int bStrict, char ** papszOptions,
      GDALProgressFunc pfnProgress, void * pProgressData
    ) {
  //

  if( !pfnProgress( 0.0, NULL, pProgressData ) ) {
    return NULL;
  }

  RasterJSONDataset* poRasterJSONDS = NULL;
  TextEncoder *ter = NULL;

  int nBands = poSrcDS->GetRasterCount();
  int nXSize = poSrcDS->GetRasterXSize();
  int nYSize = poSrcDS->GetRasterYSize();

  int nValueLinesAll = nBands * nYSize;
  int nValueLines = 0;

  int nFloatPrecision = 14;
  int nFloatConversion = APPENDF_C_e;

#ifdef _RASTERJSON_DEBUG_ALL
fprintf(stderr, "CreateCopy band=%d x=%d y=%d.\n", nBands, nXSize, nYSize);
#endif

  if( !(nBands > 0) ) {
        CPLError( CE_Failure, CPLE_NotSupported, "Bands must be more than 1.\n");
        return NULL;
  }

  if( !pfnProgress(0.0, NULL, pProgressData) ) {
    return NULL;
  }

  // overwrites precision
  const char *pszPrecision;
  pszPrecision = CSLFetchNameValue(papszOptions, "PRECISION");
  if( pszPrecision != NULL ) {
    int nFloatPrecision_Opt = atoi(pszPrecision);
    if( nFloatPrecision_Opt > 0 ) {
      nFloatPrecision = nFloatPrecision_Opt;
    }
  }
  // overwrites bFFormat
  if( CSLFetchBoolean(papszOptions, "F_FORMAT", FALSE) ) {
    nFloatConversion = APPENDF_C_f;
  }

#ifdef _RASTERJSON_DEBUG_ALL
fprintf(stderr, "nFloatPrecision %d\n", nFloatPrecision);
#endif

  // creates the dataset
  VSILFILE *fp;
  fp = VSIFOpenL( pszFilename, "wt" );
  if( fp == NULL ) {
    CPLError( CE_Failure, CPLE_OpenFailed, "Unable to create file %s.\n", pszFilename);
    return NULL;
  }
  // writes json
  TextEncoding *te = TextEncoding_New_UTF8();
  ter = TextEncoding_NewEncoder(te);
  TextEncoding_Free(te);

  double adGeoTransform[6];
  poSrcDS->GetGeoTransform(adGeoTransform);

  TextEncoder_AppendZ(ter, L"{\n");
  TextEncoder_AppendZ(ter, L"  \"type\": \"raster\",\n");
  // transform
  TextEncoder_AppendF(ter,
    L"  \"transform\": [%.14e, %.14e, %.14e, %.14e, %.14e, %.14e],\n",
    adGeoTransform[1], adGeoTransform[2], adGeoTransform[4], adGeoTransform[5],
    adGeoTransform[0], adGeoTransform[3]);
  // crs
  const char *pref = poSrcDS->GetProjectionRef();
  if( pref != NULL || *pref != '\0' ) {
    OGRSpatialReference oSRS(pref);
    const char *name = NULL;
    const char *code = NULL;
    const char *strs[4] = {NULL, NULL, NULL, NULL};
    if( oSRS.IsGeographic() ) {
      // geog
      name = oSRS.GetAuthorityName("GEOGCS");
      code = oSRS.GetAuthorityCode("GEOGCS");
    }
    else {
      // proj
      name = oSRS.GetAuthorityName("PROJCS");
      code = oSRS.GetAuthorityCode("PROJCS");
    }
    if( name != NULL && code != NULL ) {
      strs[0] = name;
      strs[1] = ":";
      strs[2] = code;
    }
    else {
      strs[0] = pref;
    }
    if( strs[0] != NULL ) {
      TextEncoder_AppendZ(ter, L"  \"crs\": \"");
      for( const char **pp = strs; *pp != NULL; pp++ ) {
        for( const char *p = *pp; *p != '\0'; p++ ) {
          if( *p == '\"' ) {
            TextEncoder_AppendChar(ter, L'\\');
          }
          TextEncoder_AppendChar(ter, (wchar_t)(*p));
        }
      }
      TextEncoder_AppendZ(ter, L"\",\n");
    }
  }

  // nodata
  int has_nodata = 0;
  for( int b = 1; has_nodata == 0 && b <= nBands; b++ ) {
    GDALRasterBand* poBand = poSrcDS->GetRasterBand(b);
    if( poBand != NULL ) {
      poBand->GetNoDataValue(&has_nodata);
    }
  }
  if( has_nodata ) {
    TextEncoder_AppendZ(ter, L"  \"nodata\": [");
    for( int b = 1; b <= nBands; b++ ) {
      GDALRasterBand* poBand = poSrcDS->GetRasterBand(b);
      if( poBand != NULL ) {
        int thisband_has_nodata;
        double nodatavalue = poBand->GetNoDataValue(&thisband_has_nodata);
        if( !thisband_has_nodata ) {
          TextEncoder_AppendZ(ter, L"null");
        }
        else if( IsInt(poBand->GetRasterDataType()) ) {
          TextEncoder_AppendF(ter, L"%ld", (long)(nodatavalue));
        }
        else {
          OutputFloat(ter, nodatavalue, nFloatConversion, nFloatPrecision);
        }
      }
      if( b < nBands ) {
        TextEncoder_AppendZ(ter, L",");
      }
    }
    TextEncoder_AppendZ(ter, L"],\n");
  }

  // datatype
  TextEncoder_AppendZ(ter, L"  \"datatype\": [");
  for( int b = 1; b <= nBands; b++ ) {
    GDALRasterBand * poBand = poSrcDS->GetRasterBand(b);
    switch(poBand->GetRasterDataType()) {
    case GDT_Byte:
      TextEncoder_AppendZ(ter, L"\"byte\"");
      break;
    case GDT_Int16:
      TextEncoder_AppendZ(ter, L"\"int16\"");
      break;
    case GDT_UInt16:
      TextEncoder_AppendZ(ter, L"\"uint16\"");
      break;
    case GDT_UInt32:
      TextEncoder_AppendZ(ter, L"\"uint32\"");
      break;
    case GDT_Float32:
      TextEncoder_AppendZ(ter, L"\"float32\"");
      break;
    case GDT_Float64:
      TextEncoder_AppendZ(ter, L"\"float64\"");
      break;
    default:
      TextEncoder_AppendZ(ter, L"\"int32\"");
      break;
    }
    if( b < nBands ) {
      TextEncoder_AppendZ(ter, L", ");
    }
  }
  TextEncoder_AppendZ(ter, L"],\n");

  // values
  TextEncoder_AppendZ(ter, L"  \"values\": [\n");
  void *buff;
  buff = CPLMalloc(nXSize * GDALGetDataTypeSize(GDT_Float64) / 8 );
  if( buff == NULL ) {
    CPLError( CE_Failure, CPLE_OpenFailed, "Cannot allocate line memory.\n");
    goto ERROR;
  }

  nValueLines = 0;

  for( int b = 1; b <= nBands; b++ ) {
    TextEncoder_AppendZ(ter, L"    [\n");
    GDALRasterBand * poBand = poSrcDS->GetRasterBand(b);
    int bReadAsInt = IsInt(poBand->GetRasterDataType());
    for( int y = 0; y < nYSize; y++ ) {
      // 2018/03/27 Cheking error for RasterIO().
      CPLErr eErr;
      eErr = poBand->RasterIO(
        GF_Read, 0, y, nXSize, 1, buff, nXSize, 1,
        bReadAsInt ? GDT_Int32 : GDT_Float64,
        0, 0);
      if( eErr != CE_None ) {
        goto ERROR;
      }
      TextEncoder_AppendZ(ter, L"      [");
      if( bReadAsInt ) {
        int32_t *p;
        p = (int32_t *)buff;
        for( int x = 0; x < nXSize; x++ ) {
          TextEncoder_AppendF(ter, L"%d", *(p++));
          if( x < nXSize - 1 ) {
            TextEncoder_AppendZ(ter, L", ");
          }
        }
      }
      else {
        double *p;
        p = (double *)buff;
        for( int x = 0; x < nXSize; x++ ) {
          OutputFloat(ter, *(p++), nFloatConversion, nFloatPrecision);
          if( x < nXSize - 1 ) {
            TextEncoder_AppendZ(ter, L", ");
          }
        }
      }
      // end of one line.
      if( y < nYSize - 1 ) {
        TextEncoder_AppendZ(ter, L"],\n");
      }
      else {
        TextEncoder_AppendZ(ter, L"]\n");
      }
      nValueLines++;
      if( !pfnProgress((double)nValueLines/(double)nValueLinesAll, NULL, pProgressData) ) {
        // failed to progress.
        CPLError( CE_Failure, CPLE_UserInterrupt,  "User terminated CreateCopy()" );
        goto ERROR;
      }
    }
    if( b < nBands ) {
      TextEncoder_AppendZ(ter, L"    ],\n  ");
    }
    else {
      TextEncoder_AppendZ(ter, L"    ]\n");
    }
  }
  TextEncoder_AppendZ(ter, L"  ]\n");
  // end of object
  TextEncoder_AppendZ(ter, L"}\n");

  CPLFree(buff);

  while(ter->mb_buff->length > 0 ) {
    unsigned char ch;
    TextEncoder_Get(ter, &ch, 1);
    VSIFWriteL(&ch, sizeof(unsigned char), 1,fp);
  }

  TextEncoder_Free(ter);
  ter = NULL;

  poRasterJSONDS = new RasterJSONDataset();
  poRasterJSONDS->nRasterXSize = nXSize;
  poRasterJSONDS->nRasterYSize = nYSize;
  poRasterJSONDS->nBands = nBands;
  for( int b = 1; b <= nBands; b++ ) {
    GDALRasterBand *poBand = poSrcDS->GetRasterBand(b);
    poRasterJSONDS->SetBand(b, new RasterJSONRasterBand(poRasterJSONDS, b, poBand->GetRasterDataType()));
  }

  // full progression
  pfnProgress(1.0, NULL, pProgressData);

#ifdef _RASTERJSON_DEBUG_ALL
fprintf(stderr, "CreateCopy done.\n");
#endif

  return poRasterJSONDS;

ERROR:
  if( poRasterJSONDS ) {
    delete poRasterJSONDS;
  }
  if( ter ) {
    TextEncoder_Free(ter);
  }
  return NULL;
}


int RasterJSONDataset::Identify(GDALOpenInfo * poOpenInfo) {
  JsonNode *node;
  JsonNode *node_type;
  wchar_t typetext[8];
  JsonParser *jpsr;

#ifdef _RASTERJSON_DEBUG_ALL
fprintf(stderr, "Identify\n");
#endif

  jpsr = JsonParser_New(NULL);
  if( jpsr == NULL ) {
    return FALSE;
  }

#ifdef _RASTERJSON_DEBUG_ALL
fprintf(stderr, "Identify: JsonParser_New() OK\n");
for( int n= 0; n < poOpenInfo->nHeaderBytes; n++ ) {
  fprintf(stderr, "%d\n", (poOpenInfo->pabyHeader)[n]);
}
fprintf(stderr, "fin\n");
#endif

  if( jpsr->jsonpull != NULL && jpsr->jsonpull->decoder != NULL ) {
    jpsr->jsonpull->decoder->hide_error_message = 1;
  }

  JsonParser_AppendText(jpsr, poOpenInfo->pabyHeader, poOpenInfo->nHeaderBytes);

  if( jpsr->jsonpull != NULL && jpsr->jsonpull->decoder != NULL ) {
    jpsr->jsonpull->decoder->hide_error_message = 0;
  }


#ifdef _RASTERJSON_DEBUG_ALL
fprintf(stderr, "AppendText done.\n", jpsr->jsonpull->decoder->mb_buff->length);
fflush(stderr);
fprintf(stderr, "length of buffer in textdecoder is %d.\n", jpsr->jsonpull->decoder->wc_buff->length);
fflush(stderr);
#endif

  // JSONPARSER_MODE_PRINT_ERROR is not set.
  JsonParser_Parse(
    jpsr,
    &node,
    JSONPARSER_MODE_BUILD_ON_ERROR
  );

#ifdef _RASTERJSON_DEBUG_ALL
fprintf(stderr, "Parse done.\n");
#endif


  JsonParser_Free(jpsr);

#ifdef _RASTERJSON_DEBUG_ALL
fprintf(stderr, "Free(jpsr) done.\n");
#endif

  if( node != NULL ) {

#ifdef _RASTERJSON_DEBUG_ALL
fprintf(stderr, "node is not null.\n");
#endif

    int ret = FALSE;
    if( (node_type = JsonNode_ObjectGet(node, L"type")) != NULL ) {

#ifdef _RASTERJSON_DEBUG_ALL
fprintf(stderr, "node(type) is not null.\n");
#endif

#ifdef _RASTERJSON_DEBUG_ALL
fprintf(stderr, "isprimitive %d\n", JsonNode_IsPrimitive(node_type));
fprintf(stderr, "getstring %d\n", JsonNode_GetString(node_type, typetext, 7));
fprintf(stderr, "wcscmp %d\n", Jwcscmp(typetext, L"raster"));
#endif


      if( JsonNode_IsPrimitive(node_type) &&
          JsonNode_GetString(node_type, typetext, 7) > 0 &&
          wcscmp(typetext, L"raster") == 0
        ) {
        ret = TRUE;
      }
    }
    JsonNode_Free(node);

#ifdef _RASTERJSON_DEBUG_ALL
fprintf(stderr, "Identify done. returned value is %d.\n", ret);
#endif


    return ret;
  }

#ifdef _RASTERJSON_DEBUG_ALL
fprintf(stderr, "Identify error.\n");
#endif


  return FALSE;
}

/* ------------------------------------------------ RasetJSONRasterBand */
RasterJSONRasterBand::RasterJSONRasterBand(RasterJSONDataset *poDS, int nBand, GDALDataType eDataType) {
#ifdef _RASTERJSON_DEBUG_ALL
fprintf(stderr, "RasterJSONRasterBand\n");
#endif

  this->poDS = poDS;
  this->nBand = nBand;
  bNoDataSet = FALSE;
  dNoDataValue = -9999.0;
  this->eDataType = eDataType;
  nBlockXSize = poDS->nRasterXSize;
  nBlockYSize = 1;

#ifdef _RASTERJSON_DEBUG_ALL
fprintf(stderr, "RasterJsonRasterBand done.\n");
#endif

}


CPLErr RasterJSONRasterBand::IReadBlock(int nBlockXOff, int nBlockYOff, void * pImage) {
#ifdef _RASTERJSON_DEBUG_ALL
fprintf(stderr, "IReadBlock(%d, %d, %p) (no \"done\" message)\n", nBlockXOff, nBlockYOff, pImage);
#endif
  return ((RasterJSONDataset *)poDS)->ReadBlock(
      nBand, nBlockXOff, nBlockYOff, pImage, this->GetRasterDataType()
    );
}


double RasterJSONRasterBand::GetNoDataValue(int *pbSuccess) {
  if( pbSuccess != NULL ) {
    *pbSuccess = bNoDataSet;
  }
  return dNoDataValue;
}


CPLErr RasterJSONRasterBand::SetNoDataValue(double dfNoData) {
  bNoDataSet = TRUE;
  dNoDataValue = dfNoData;
  return CE_None;
}

/* ------------------------------------------------ registration */
void GDALRegister_RasterJSON() {
#ifdef _RASTERJSON_DEBUG_ALL
fprintf(stderr, "RegisterJSON\n");
#endif

  GDALDriver  *poDriver;
  if( GDALGetDriverByName( "RasterJSON" ) == NULL ) {
    // Register
#ifdef _RASTERJSON_DEBUG_ALL
fprintf(stderr, "RegisterJSON really registered.\n");
#endif
    poDriver = new GDALDriver();
    poDriver->SetDescription( "RasterJSON" );
    poDriver->SetMetadataItem( GDAL_DMD_LONGNAME, "Raster JSON" );
    poDriver->SetMetadataItem( GDAL_DMD_EXTENSION, "json" );
    poDriver->SetMetadataItem( GDAL_DMD_CREATIONDATATYPES, "Byte UInt16 Int16 Int32 Float32" );
    poDriver->SetMetadataItem( GDAL_DCAP_VIRTUALIO, "YES" );
    poDriver->SetMetadataItem (GDAL_DMD_CREATIONOPTIONLIST,
      "<CreateOptionList>"
        "<Option name='PRECISION' type='int' description='Fraction length. If not positive, set by 14. Affects only if band value is float.' />"
        "<Option name='F_FORMAT' type='boolean' description='If set by true and band value type is real, uses %f format.' />"
      "</CreateOptionList>"
    );

    poDriver->pfnOpen = RasterJSONDataset::Open;
    poDriver->pfnIdentify = RasterJSONDataset::Identify;
    poDriver->pfnCreateCopy = RasterJSONDataset::CreateCopy;
    GetGDALDriverManager()->RegisterDriver(poDriver);
  }

#ifdef _RASTERJSON_DEBUG_ALL
fprintf(stderr, "RegisterJSON done.\n");
#endif

}
