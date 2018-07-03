#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bmp.h"

#ifdef TEST
	#define DBG printf
#else
	#define DBG(...)
#endif

int bmp_create(pbmp_struct_t bmp, int h, int w, unsigned int bpp)
{
	int i;
	memset(bmp, 0x00, sizeof(bmp_struct_t));

	if( (w < 4) || (h < 4) )
	{
		return 1;
	}

	bmp->h = h;
	bmp->w = w;
	
	bmp->line = (bpp / 8) * w;
	if (bmp->line & 0x03)
	{
		bmp->line = (bmp->line & ~0x0003) + 4;
	}
	
	switch(bpp)
	{
/*
	case 32:
		bmp->palette_size = 0;
		bmp->pElementRGB = NULL;
		bmp->rect_size = w * h * bpp / 8;
		bmp->byte_per_pixel = 4;
		break;
*/
	case 24:
		bmp->palette_size = 0;
		bmp->pElementRGB = NULL;
		bmp->rect_size = bmp->line * h;
		bmp->byte_per_pixel = 3;
		break;
	case 8:
		bmp->palette_size = 256 * sizeof(RGBQUAD);
		bmp->pElementRGB = (RGBQUAD*) calloc (bmp->palette_size / 4, sizeof(RGBQUAD));
		bmp->rect_size = bmp->line * h;
		bmp->byte_per_pixel = 1;
		break;
/*
	case 4:
		bmp->palette_size = 16 * sizeof(RGBQUAD);
		bmp->pElementRGB = (RGBQUAD*) calloc (bmp->palette_size / 4, sizeof(RGBQUAD));
		bmp->rect_size = w * h * bpp / 8;
		bmp->byte_per_pixel = -1;
		break;
	case 1:
		bmp->palette_size = 2 * sizeof(RGBQUAD);
		bmp->pElementRGB = (RGBQUAD*) calloc (bmp->palette_size / 4, sizeof(RGBQUAD));
		bmp->rect_size = w * h * bpp / 8;
		bmp->byte_per_pixel = -1;
		break;
*/
	default:
		return 2;
		break;
	};

	bmp->rect_offset = BMP_HEADERS_SIZE + bmp->palette_size;
	bmp->rect = (char*)malloc(bmp->rect_size);
	if (bmp->rect == NULL)
	{
		return 1;
	}

	bmp->pElementRGB = (RGBQUAD*)malloc(bmp->palette_size * sizeof(RGBQUAD));
	if (bmp->rect == NULL)
	{
		free(bmp->rect);
		return 2;
	}

	bmp->BitMapHeader.bfType        = 0x4d42;
//	bmp->BitMapHeader.bfSize        = bmp->rect_offset + bmp->h * bmp->w;
	bmp->BitMapHeader.bfSize        = bmp->rect_offset + bmp->BitMapInfo.biSizeImage;
	bmp->BitMapHeader.bfReserved1   = 0;
	bmp->BitMapHeader.bfReserved2   = 0;
	bmp->BitMapHeader.bfOffBits     = bmp->rect_offset;
	
	bmp->BitMapInfo.biSize          = sizeof(BITMAPINFOHEADER);
	bmp->BitMapInfo.biWidth         = bmp->w;
	bmp->BitMapInfo.biHeight        = bmp->h;
	bmp->BitMapInfo.biPlanes        = 1;
//	bmp->BitMapInfo.biBitCount      = 8 * bmp->byte_per_pixel;
	bmp->BitMapInfo.biBitCount      = bpp;
	bmp->BitMapInfo.biCompression   = 0;
//	bmp->BitMapInfo.biSizeImage     = 0;
	bmp->BitMapInfo.biSizeImage     = bmp->rect_size;
	bmp->BitMapInfo.biXPelsPerMeter = 0;
	bmp->BitMapInfo.biYPelsPerMeter = 0;
	bmp->BitMapInfo.biClrUsed       = 0;
	bmp->BitMapInfo.biClrImportant  = 0;
	
	// Create B/W palitte:
	for (i = 0; i < bmp->palette_size; i++)
	{
		bmp->pElementRGB[i].rgbBlue     = i;
		bmp->pElementRGB[i].rgbGreen    = i;
		bmp->pElementRGB[i].rgbRed      = i;
		bmp->pElementRGB[i].rgbReserved = 0x00;
	}
	
	return 0;
}

int bmp_load(pbmp_struct_t bmp, char *filename)
{
	FILE *f;
	int i;
	int size;
	memset(bmp, 0x00, sizeof(bmp_struct_t));
	
	f = fopen(filename, "rb");
	if (f == NULL)
	{
		return 1;
	}
	// Пишем заголовоки:
	fseek(f, 0, SEEK_SET);
	fread(&bmp->BitMapHeader, sizeof(BITMAPFILEHEADER), 1, f);
	fread(&bmp->BitMapInfo, sizeof(BITMAPINFOHEADER), 1, f);
	
	bmp->h              = bmp->BitMapInfo.biHeight;
	bmp->w              = bmp->BitMapInfo.biWidth;

	bmp->byte_per_pixel = (bmp->BitMapInfo.biBitCount / 8);
	
	bmp->line = bmp->byte_per_pixel * bmp->w;
	if (bmp->line & 0x03)
	{
		bmp->line = (bmp->line & ~0x0003) + 4;
	}
	//printf("bmp->line = %d\n",bmp->line);
	
	bmp->rect_size      = bmp->h * bmp->line;
	bmp->palette_size   = 0;
	bmp->rect_offset    = bmp->BitMapHeader.bfOffBits;
	
//	printf("bmp_load: h:%d w:%d, bbp:%d rects: %d recto:%02x\n", 
//			bmp->h, bmp->w, bmp->byte_per_pixel, bmp->rect_size, bmp->rect_offset);
	
	bmp->rect = (char*)malloc(bmp->rect_size*sizeof(uint8_t));
	if (bmp->rect == NULL)
	{
		return 1;
	}	
	
	fseek(f, bmp->rect_offset, SEEK_SET);
	fread(bmp->rect, bmp->rect_size, 1, f);
	
//	for (i=0; i<bmp->rect_size; i++)
//		printf("%02x ", bmp->rect[i]);
	
	return 0;
}

int bmp_save(pbmp_struct_t bmp, char *filename)
{
	FILE *f;
	//printf("bmp_save(): Open|Create file...\r\n");
	// Cоздаем BMP-файл:
	//if (fopen_s(&f, filename, "wb"))
	f = fopen(filename, "wb");
	if (f == NULL)
	{
		printf("bmp_save(): fopen return %d\r\n", f);
		return 1;
	}
	//printf("bmp_save(): Header Write...\r\n");
	// Пишем заголовоки:
	fwrite(&bmp->BitMapHeader, sizeof(BITMAPFILEHEADER), 1, f);
	fwrite(&bmp->BitMapInfo, sizeof(BITMAPINFOHEADER), 1, f);
	fwrite(bmp->pElementRGB, sizeof(RGBQUAD), bmp->palette_size, f);
	//printf("bmp_save(): Header Data...\r\n");
	// Пишем данные:
	fseek(f, bmp->rect_offset, SEEK_SET);
	fwrite(bmp->rect, 1, bmp->rect_size, f);
	//printf("bmp_save(): Close File...\r\n");
	// Закрываем файл:
	fclose(f);
	//printf("bmp_save(): end...\r\n");
	return 0;
}

int bmp_putpixel(pbmp_struct_t bmp, int x, int y, int value)
{
	if (x >= bmp->w || y >= bmp->h)
	{
		return 1;
	}
	
	if (bmp->byte_per_pixel == 1)
	{
		if (bmp->rect != NULL)
		{
			bmp->rect[bmp->line*(bmp->h-y-1) + x] = value;
		}
	}
	else if (bmp->byte_per_pixel == 3)
	{
		if (bmp->rect != NULL)
		{
			bmp->rect[(bmp->line*(bmp->h-y-1) + x*3) + 0] = (value>>0) &0xFF;
			bmp->rect[(bmp->line*(bmp->h-y-1) + x*3) + 1] = (value>>8) &0xFF;
			bmp->rect[(bmp->line*(bmp->h-y-1) + x*3) + 2] = (value>>16)&0xFF;
		}
	}
	else
	{
		return 2;
	}
	
	return 0;
}

int bmp_getpixel(pbmp_struct_t bmp, int x, int y, int *value)
{
	if (x >= bmp->w || y >= bmp->h)
	{
		return 1;
	}
	
	if (bmp->byte_per_pixel == 1)
	{
		if (bmp->rect != NULL)
		{
			*value = bmp->rect[bmp->line*(bmp->h-y-1) + x];
		}
	}
	else if (bmp->byte_per_pixel == 3)
	{
		if (bmp->rect != NULL)
		{
			*value  = bmp->rect[(bmp->line*(bmp->h-y-1) + x*3) + 2];
			*value <<= 8;
			*value |= bmp->rect[(bmp->line*(bmp->h-y-1) + x*3) + 1];
			*value <<= 8;
			*value |= bmp->rect[(bmp->line*(bmp->h-y-1) + x*3) + 0];
		}
	}
	else
	{
		return 2;
	}
	
	return 0;
}

int bmp_putrect(pbmp_struct_t bmp, int x, int y, char *rect, int rect_x, int rect_y)
{
	int i, j;
	
	if (x > bmp->w || y > bmp->h)
	{
		return 1;
	}
	
	if (x + rect_x > bmp->w || y + rect_y > bmp->h)
	{
		return 2;
	}
	
	if (bmp->byte_per_pixel == 1)
	{
		for (j = y; j < y + rect_y; j++)
		{
			for (i = x; i < x + rect_x; i++)
			{
				bmp->rect[bmp->w*(bmp->h-j-1) + i] = rect[(j-y)*rect_x + (i-x)];
			}
		}
	}
	else
	{
		return 3;
	}
	
	return 0;
}

int bmp_fillrect(pbmp_struct_t bmp, int x, int y, int value, int rect_x, int rect_y)
{
	int i, j;
	
	if (x >= bmp->w || y >= bmp->h)
	{
		return 1;
	}
	
	if (x + rect_x > bmp->w || y + rect_y > bmp->h)
	{
		return 2;
	}
	
	if (bmp->byte_per_pixel == 1)
	{
		for (j = y; j < y + rect_y; j++)
		{
			for (i = x; i < x + rect_x; i++)
			{
				bmp->rect[bmp->w*(bmp->h-j-1) + i] = value;
			}
		}
	}
	else
	{
		return 3;
	}
	
	return 0;
}

int bmp_close(pbmp_struct_t bmp)
{
	free(bmp->pElementRGB);
	free(bmp->rect);
	memset(bmp, 0x00, sizeof(bmp_struct_t));
	return 0;
}

int bmp_setpalette8(pbmp_struct_t bmp, RGBQUAD *palette)
{
	if (bmp->byte_per_pixel = 1)
	{
		int i;
		for (i = 0; i < bmp->palette_size; i++)
		{
			bmp->pElementRGB[i].rgbBlue     = palette[i].rgbBlue;
			bmp->pElementRGB[i].rgbGreen    = palette[i].rgbGreen;
			bmp->pElementRGB[i].rgbRed      = palette[i].rgbRed;
			bmp->pElementRGB[i].rgbReserved = 0x00;
		}
	}
	else
	{
		return 1;
	}
	return 0;
}

#if 0

#include "bmp_tbl.c"

extern unsigned char symbol_table[256][12];

int bmp_putchar(pbmp_struct_t bmp, int x, int y, unsigned char sym, int value)
{
	int i;
	int j;

	for (i = 0; i < 12; i++)
	{
		for (j = 0; j < 8; j++)
		{
			if ((symbol_table[sym][i] & (0x01 << j)) == 0)
			{
				bmp_putpixel(bmp, x+7-j, y+i, value);
			}
		}
	}

	return 0;
}

int bmp_putstr(pbmp_struct_t bmp, int x, int y, unsigned char *str, int value)
{
	int i;
	int j;

	while (*str != 0)
	{
		bmp_putchar(bmp, x, y, *str, value);
		x += 8;
		str++;
	}

	return 0;
}
#endif


#if defined( PUT_LINE_BEZ )
/* bmp_putline bezier */
typedef struct
{
	double x;
	double y;
} BEZIER_POINT;

void bezier(const int n, BEZIER_POINT *a, const double t, BEZIER_POINT *r)
{
	int i;
    int c;
    double p;

    c = 1;
    for(i = 0; i < n; i++)
    {
        a[i].x = a[i].x * c;
        a[i].y = a[i].y * c;
        c = (n - 1 - i) * c / (i + 1);
    }
	
    p = 1.0;
    for(i = 0; i < n; i++)
    {
        a[i].x = a[i].x * p;
        a[i].y = a[i].y * p;
        p = p * t;
    }
    
	p = 1.0;
    for(i = n - 1; i >= 0; i--)
    {
        a[i].x = a[i].x * p;
        a[i].y = a[i].y * p;
        p = p * (1.0 - t);
    }

    r->x = 0.0;
    r->y = 0.0;
    for(i = 0; i < n; i++)
    {
        r->x = r->x + a[i].x;
        r->y = r->y + a[i].y;
    }
}

/* ...... */

int bmp_putline(pbmp_struct_t bmp, int x1, int y1, int x2, int y2, int value)
{
	BEZIER_POINT pa[2];
	BEZIER_POINT r;
	int i;
	
	for (i = 0; i < 1001; i++)
	{
		pa[0].x = x1;
		pa[0].y = y1;
		pa[1].x = x2;
		pa[1].y = y2;
		
		bezier(2, pa, (double)i/1000, &r);
		bmp_putpixel(bmp, (int)r.x, (int)r.y, value);
	}
	
	return 0;
}
#endif

int bmp_print_file_header(pbmp_struct_t bmp)
{
	if(bmp == NULL)
	{
		return -1;
	}
	
	printf("\nsize of struct = %d",sizeof(BITMAPFILEHEADER));
	printf("\nbfType = %d",bmp->BitMapHeader.bfType);                //BM = 0x4d42 - формат BMP
	printf("\nbfSize = %d",bmp->BitMapHeader.bfSize);                //размер всего файла
	printf("\nbfReserved1 = %d",bmp->BitMapHeader.bfReserved1);      //зарезервированы и = 0
	printf("\nbfReserved2 = %d",bmp->BitMapHeader.bfReserved2);      //зарезервированы и = 0
	printf("\nbfOffBits = %d",bmp->BitMapHeader.bfOffBits);          //смещение до растровых данных
	
	return 0;
}

int bmp_print_info_header(pbmp_struct_t bmp)
{
	if(bmp == NULL)
	{
		return -1;
	}
	
	printf("\nsize of struct = %d",sizeof(BITMAPINFOHEADER));
	printf("\nbiSize = %d",bmp->BitMapInfo.biSize);                        //
	printf("\nbiWidth = %d",bmp->BitMapInfo.biWidth);                      //Ширина изображения (Пиксел)
	printf("\nbiHeight = %d",bmp->BitMapInfo.biHeight);                    //Высота изображения (Пиксел)
	printf("\nbiPlanes = %d",bmp->BitMapInfo.biPlanes);                    //
	printf("\nbiBitCount = %d",bmp->BitMapInfo.biBitCount);                //
	printf("\nbiCompression = %d",bmp->BitMapInfo.biCompression);          //Для несжатых данных = 0
	printf("\nbiSizeImage = %d",bmp->BitMapInfo.biSizeImage);              //
	printf("\nbiXPelsPerMeter = %d",bmp->BitMapInfo.biXPelsPerMeter);      //
	printf("\nbiYPelsPerMeter = %d",bmp->BitMapInfo.biYPelsPerMeter);      //
	printf("\nbiClrUsed = %d",bmp->BitMapInfo.biClrUsed);                  //
	printf("\nbiClrImportant = %d",bmp->BitMapInfo.biClrImportant);        //
	
	return 0;
}

#ifdef TEST
int main(void)
{
	bmp_struct_t b;
	int i;
	int ret;
	
	printf("bmp module test...\n");
	
	printf("bmp_create()\n");
	if (bmp_create(&b, 100, 100, 8))
	{
		printf("bmp_create() error!\n");
		exit(1);
	}
	
	printf("bmp_save(\"bmp_test1.bmp\")...\n");
	if (ret = bmp_save(&b, "bmp_test1.bmp"))
	{
		printf("bmp_save() error: %d!\n", ret);
		exit(2);
	}
	
	if (bmp_fillrect(&b, 0, 0, 0x00, 100, 100))
	{
		printf("bmp_fillrect() error!\n");
		exit(3);
	}
	printf("bmp_save(\"bmp_test2.bmp\")...\n");
	if (bmp_save(&b, "bmp_test2.bmp"))
	{
		printf("bmp_save() error!\n");
		exit(4);
	}
	
	if (bmp_fillrect(&b, 0, 0, 0xFF, 100, 100))
	{
		printf("bmp_fillrect() error!\n");
		exit(5);
	}
	printf("bmp_save(\"bmp_test3.bmp\")...\n");
	if (bmp_save(&b, "bmp_test3.bmp"))
	{
		printf("bmp_save() error!\n");
		exit(6);
	}
	
	for (i = 0; i < 100; i++)
	{
		if (bmp_putpixel(&b, i, i, 0x80))
		{
			printf("bmp_putpixel() error!\n");
			exit(7);
		}
	}
	printf("bmp_save(\"bmp_test4.bmp\")...\n");
	if (bmp_save(&b, "bmp_test4.bmp"))
	{
		printf("bmp_save() error!\n");
		exit(8);
	}

	// Тест вывода текста
	bmp_putstr(&b, 0, 0, "Hello World!", 0);
	printf("bmp_save(\"bmp_test5.bmp\")...\n");
	if (bmp_save(&b, "bmp_test5.bmp"))
	{
		printf("bmp_save() error!\n");
		exit(8);
	}
	
	if (bmp_fillrect(&b, 20, 20, 0x80, 80, 80))
	{
		printf("bmp_fillrect() error!\n");
		exit(9);
	}
	printf("bmp_save(\"bmp_test6.bmp\")...\n");
	if (bmp_save(&b, "bmp_test6.bmp"))
	{
		printf("bmp_save() error!\n");
		exit(10);
	}
	
#if defined( PUT_LINE_BEZ )
	if (bmp_putline(&b, 20, 20, 40, 80, 0))
	{
		printf("bmp_putline() error!\n");
		exit(11);
	}
	if (bmp_putline(&b, 0, 80, 80, 0, 0))
	{
		printf("bmp_putline() error!\n");
		exit(11);
	}
	printf("bmp_save(\"bmp_test7.bmp\")...\n");
	if (bmp_save(&b, "bmp_test7.bmp"))
	{
		printf("bmp_save() error!\n");
		exit(12);
	}
	
	if (bmp_close(&b))
	{
		printf("bmp_close() error!\n");
		exit(100);
	}
#endif
	
	printf("bmp module test OK!\n");
	return 0;
}
#endif
