#pragma once
#ifndef __BMP_H__
#define __BMP_H__
#include <efi.h>

#pragma pack(1)
typedef struct {
    // bmfh
    CHAR8    bfType[2];
    UINT32   bfSize;
    UINT16    bfReserved1;
    UINT16    bfReserved2;
    UINT32   bfOffBits;

    // bmih
    UINT32   biSize;
    INT32    biWidth;
    INT32    biHeight;
    UINT16    biPlanes;
    UINT16    biBitCount;
    UINT32   biCompression;
    UINT32   biSizeImage;
    INT32    biXPelsPerMeter;
    INT32    biYPelsPerMeter;
    UINT32   biClrUsed;
    UINT32   biClrImportant;
} BMP_IMAGE_HEADER;
#pragma pack()

#endif
