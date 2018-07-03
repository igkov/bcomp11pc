#ifndef __BMP_H__
#define __BMP_H__

#include <stdint.h>

#ifndef _WINDOWS_H

#pragma pack(push,2)
typedef struct 
{
	uint16_t	bfType;
	uint32_t	bfSize;
	uint16_t	bfReserved1;
	uint16_t	bfReserved2;
	uint32_t	bfOffBits;
} BITMAPFILEHEADER,*LPBITMAPFILEHEADER,*PBITMAPFILEHEADER;
#pragma pack(pop)

typedef struct 
{
	uint32_t biSize;
	int32_t biWidth;
	int32_t biHeight;
	uint16_t biPlanes;
	uint16_t biBitCount;
	uint32_t biCompression;
	uint32_t biSizeImage;
	int32_t biXPelsPerMeter;
	int32_t biYPelsPerMeter;
	uint32_t biClrUsed;
	uint32_t biClrImportant;
} BITMAPINFOHEADER,*LPBITMAPINFOHEADER,*PBITMAPINFOHEADER;

typedef struct 
{
	uint8_t rgbBlue;
	uint8_t rgbGreen;
	uint8_t rgbRed;
	uint8_t rgbReserved;
} RGBQUAD,*LPRGBQUAD;

typedef struct 
{
	BITMAPINFOHEADER bmiHeader;
	RGBQUAD bmiColors[1];
} BITMAPINFO,*LPBITMAPINFO,*PBITMAPINFO; 

#endif // _WINDOWS_H

#define BMP_HEADERS_SIZE (sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER))

typedef struct
{
	uint32_t h;
	uint32_t w;
	uint32_t line;
	uint8_t *rect;
	BITMAPFILEHEADER BitMapHeader;
	BITMAPINFOHEADER BitMapInfo;
	RGBQUAD          *pElementRGB;
	uint32_t palette_size;
	uint32_t rect_size;
	uint32_t rect_offset;
	uint8_t byte_per_pixel;
} bmp_struct_t, *pbmp_struct_t;

int bmp_create(pbmp_struct_t bmp, int h, int w, unsigned int bpp);
//int bmp_create(pbmp_struct_t bmp, int h, int w);
int bmp_load(pbmp_struct_t bmp, char *filename);
int bmp_save(pbmp_struct_t bmp, char *filename);
int bmp_putpixel(pbmp_struct_t bmp, int x, int y, int value);
int bmp_getpixel(pbmp_struct_t bmp, int x, int y, int *value);
int bmp_putrect(pbmp_struct_t bmp, int x, int y, char *rect, int rect_x, int rect_y);
int bmp_fillrect(pbmp_struct_t bmp, int x, int y, int value, int rect_x, int rect_y);
int bmp_close(pbmp_struct_t bmp);
int bmp_setpalette8(pbmp_struct_t bmp, RGBQUAD *palette);

int bmp_putchar(pbmp_struct_t bmp, int x, int y, unsigned char sym, int value);
int bmp_putstr(pbmp_struct_t bmp, int x, int y, unsigned char *str, int value);

#endif //__BMP_H__
