#include "util.h"
#include <time.h>

struct ImageInfoType {
    char  FileName     [120];
    time_t FileDateTime;
    unsigned FileSize;
    char  CameraMake   [32];
    char  CameraModel  [64];
    char  DateTime     [20];
    int   Height, Width;
    int   IsColor;
    int   FlashUsed;
    float FocalLength;
    float ExposureTime;
    float ApertureFNumber;
    float Distance;
    float CCDWidth;
    char  Comments[2000];
	double FocalplaneXRes;
	double FocalplaneUnits;
	int ExifImageWidth;
	int MotorolaOrder;
	int Orientation;
	char GPSinfo[48];
	int ISOspeed;
	char ExifVersion[16];
	char Copyright[32];
	char Software[32];
	char *Thumbnail;
	int ThumbnailSize;
	int ThumbnailOffset;
	/* Olympus vars */
	int SpecialMode;
	int JpegQual;
	int Macro;
	int DigiZoom;
	char SoftwareRelease[16];
	char PictInfo[64];
	char CameraId[64];
	/* End Olympus vars */
};

extern int read_jpeg_exif( struct ImageInfoType *ImageInfo, char *FileName, int ReadAll);
