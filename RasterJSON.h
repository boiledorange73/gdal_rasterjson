#ifndef _RASTERJSON_HEAD_
#define _RASTERJSON_HEAD_

#ifndef _RASTERJSON_EXTERN
#define _RASTERJSON_EXTERN extern
#endif

#include "jsonparser.h"


class RasterJSONRasterBand;

class CPL_DLL RasterJSONDataset : public GDALPamDataset {
  friend class RasterJSONRasterBand;

  char **papszPrj;
  char *pszProjection;

  JsonNode *pJsonRoot;
  JsonNode *pJsonValues;

protected:
  double adfGeoTransform[6];


public:
  RasterJSONDataset();
  ~RasterJSONDataset();

  CPLErr ReadBlock(int nBand, int nBlockXOff, int nBlockYOff, void *pImage, GDALDataType eDataType);

  static GDALDataset *Open( GDALOpenInfo * );
  static int Identify( GDALOpenInfo * );
  static GDALDataset *CreateCopy(
      const char * pszFilename, GDALDataset *poSrcDS,
      int bStrict, char ** papszOptions,
      GDALProgressFunc pfnProgress, void * pProgressData
    );

//    virtual char **GetFileList(void);
//    static CPLErr       Delete( const char *pszFilename );
//    static CPLErr       Remove( const char *pszFilename, int bRepError );

    virtual CPLErr GetGeoTransform( double * );
    virtual const char *GetProjectionRef(void);
};

class RasterJSONRasterBand : public GDALPamRasterBand {
  friend class RasterJSONDataset;

protected:
  int bNoDataSet;
  double dNoDataValue;

public:
  RasterJSONRasterBand(RasterJSONDataset *poDS, int nBand, GDALDataType eDataType);
//  virtual ~RasterJSONRasterBand();
  virtual double GetNoDataValue(int *pbSuccess);
  virtual CPLErr SetNoDataValue(double dfNoData);
  virtual CPLErr IReadBlock( int, int, void * );
};

#endif /* RasterJson.h */

