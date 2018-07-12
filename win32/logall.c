#include <stdint.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

#include "bmp.h"
#include "nmea.h"

RGBQUAD palette[256] = {
#include "pallete.c"
};

int time_pos = -1;
int lat_pos = -1;
int lon_pos = -1;
int speed_pos = -1;
int rpm_pos = -1;
int temp_eng_pos = -1;
int temp_at_pos = -1;
int temp_air_pos = -1;
int fuel_pos = -1;
int dist_pos = -1;

int speed_max = 0;
int rpm_max = 0;
int temp_eng_max = -100;
int temp_eng_min = 100;
int temp_at_max = -100;
int temp_at_min = 100;
int temp_air_max = -100;
int temp_air_min = 100;

double dist = 0.0f;
double fuel = 0.0f;
double fuel_cur = 0.0f;
int time = 0;
int time_ig = 0;
int time_move = 0;

int rpm_2000 = 0;
int rpm_3000 = 0;
int rpm_3500 = 0;
int rpm_4000 = 0;

int at_40 = 0;
int at_50 = 0;
int at_80 = 0;
int at_90 = 0;
int at_100 = 0;
int at_110 = 0;

bmp_struct_t bmp;

typedef struct {
	int id;
	char alias[32];
	int size_x;
	int size_y;
	double center_x;
	double center_y;
	double rad_x;
	double rad_y;
} place_t;

int id = 4;

place_t places[] = {
	{  1, "Sochi",     4096, 4096, 39.487844f, 43.777452f, 0.9f, 0.6f, },
	{  2, "M4 Krim",   2048, 8192, 37.619020f, 55.753960f, 5.0f, 20.0f },
	{  3, "Krim",      2048, 2048, 34.091941f, 45.240079f, 1.0f, 0.75f },
	{  4, "Center",    2048, 2048, 37.619020f, 55.753960f, 4.0f, 3.0f  },
	{  5, "NN",        2048, 2048, 43.994927f, 56.328581f, 0.15f, 0.1f },
	{  6, "Maslovo",   2048, 2048, 37.701503f, 57.278049f, (0.075f/2), (0.05f/2) },
	{  7, "Moscow",    2048, 2048, 37.619020f, 55.753960f, 0.3f, 0.2f  },
	{  8, "Nerl",      2048, 2048, 37.991979f, 57.042471f, 0.15f, 0.1f },
	{  9, "Nerl 2",    2048, 2048, 37.787425f, 57.082049f, 0.3f, 0.2f  },
	{  0, "0",         0,    0,    0.0f,       0.0f,       0.0f, 0.0f  },
};

#define ABS(a) ((a)>0?(a):-(a))

void put_point(double lat, double lon, int id) {
	int x;
	int y;
	if (ABS(lat-places[id].center_x) < places[id].rad_x && ABS(lon-places[id].center_y) < places[id].rad_y) {
		x = (int)((lat-places[id].center_x)/places[id].rad_x*places[id].size_x/2 + places[id].size_x/2);
		y = (int)((places[id].center_y-lon)/places[id].rad_y*places[id].size_y/2 + places[id].size_y/2);
		bmp_putpixel(&bmp, x, y+1, 0xFF);
	} else {
		//printf("ERROR: lon = %.5f, lat = %.5f\r\n", lon, lat);
	}
}

int fgetline(FILE *fp, char *line, int maxsize) {
	int offset = 0;
	while (offset < maxsize) {
		int ch = getc(fp);
		if (ch == EOF) {
			line[offset] = 0;
			if (offset == 0) {
				return EOF;
			} else {
				return offset;
			}
		} else
		if (ch == '\r' || ch == '\n') {
			if (offset == 0) {
				continue;
			}
			line[offset] = 0;
			return offset;
		} else {
			line[offset] = ch;
			offset++;
		}
	}
	return EOF;
}

int csv_findpos(char *line, int size, char *mark) {
	char *cpos;
	int pos = 0;
	if ((cpos = strstr(line, mark)) == NULL) {
		return -1;
	}
	while (cpos > line) {
		cpos--;
		if (*cpos == ';') {
			pos++;
		}
	}
	return pos;
}

// atoi()
// atof()

int csv_getpos(char *line, int size, int pos, char *value) {
	int offset = 0;
	int cnt = 0;
	int len = 0;
	if (pos == -1) {
		//printf("csv_getpos(): position error!\r\n");
	}
	while (offset < size) {
		if (cnt == pos) {
			cnt = 0;
			// сохранение значения:
			while (line[offset+cnt] != ';' && line[offset+cnt] != 0) {
				value[cnt] = line[offset+cnt];
				cnt++;
			}
			value[cnt] = 0;
			return cnt;
		} else
		if (line[offset] == ';') {
			cnt++;
		}
		offset++;
	}
	return 0;
}

uint8_t h2b(uint8_t _ascii) {
	if((_ascii >= '0') && (_ascii <= '9')) return(_ascii - '0'); 
	//if((_ascii >= 'a') && (_ascii <= 'f')) return(_ascii - 'a' + 10); 
	//if((_ascii >= 'A') && (_ascii <= 'F')) return(_ascii - 'A' + 10); 
	return 0x00;
}

//$GPRMC,141444.000,A,5549.6591,N,03736.0486,E,0.01,292.59,100618,,,D*6C
//N5555.1501:E03745.9223
double gpstof(char *str) {
	int i;
	int offset = 0;
	int minus = 0;
	double res = 0.0f;
	if (str[offset] == 'N' || str[offset] == 'E') {
		minus = 0;
		offset++;
	} if (str[offset] == 'S' || str[offset] == 'W') {
		minus = 1;
		offset++;
	}
	if (str[offset+4] == '.') {
		for ( i = 0; i < 2; i++ ) {
			res = res * 10.0f + (double)h2b(str[offset+i]);
		}
		offset += 2;
	}
	if (str[offset+5] == '.') {
		for ( i = 0; i < 3; i++ ) {
			res = res * 10.0f + (double)h2b(str[offset+i]);
		}
		offset += 3;
	}
	res += atof(&str[offset]) / 60.0f;
	if (minus) {
		res *= -1.0f;
	}
	return res;
}

int csv_proc(FILE *fp) {
	char line[1024];
	char value[1024];
	int ret = 0;
	int cnt = 0;
	
	while (ret != EOF) {
		int size;
		size = fgetline(fp, line, sizeof(line));
		if (size == EOF) {
			break;
		} else
		if (size == 0) {
			continue;
		} else
		if (cnt == 0) {
			time_pos = csv_findpos(line, size, "time");
			speed_pos = csv_findpos(line, size, "speed");
			rpm_pos = csv_findpos(line, size, "rpm");
			temp_eng_pos = csv_findpos(line, size, "t_eng");
			temp_at_pos = csv_findpos(line, size, "t_akpp");
			temp_air_pos = csv_findpos(line, size, "t_ext");
			fuel_pos = csv_findpos(line, size, "dfuel");
			lat_pos = csv_findpos(line, size, "lat");
			lon_pos = csv_findpos(line, size, "lon");
			dist_pos = csv_findpos(line, size, "dist");
			
			// Fuel:
			fuel += fuel_cur;
			fuel_cur = 0;
		} else {
			double lat, lon;
			// Проверка строки
			ret = csv_getpos(line, size, 0, value);
			if (value[0] == '$') {
				// NMEA parse:
				ret = nmea_parce(value);
				if (ret == 0) {
					// convert
					extern bcomp_t bcomp;
					lat = gpstof(bcomp.gps_val_lon);
					lon = gpstof(bcomp.gps_val_lat);
					
					//printf("lon = \"%s\", lat = \"%s\"\r\n", bcomp.gps_val_lon, bcomp.gps_val_lat);
					//printf("lon = %f, lat = %f\r\n", lon, lat);
					
					// Добавление точки на картинку:
					put_point(lat, lon, id);
					
					if (speed_max < (int)bcomp.gps_speed) {
						speed_max = (int)bcomp.gps_speed;
					}
					dist += ((double)bcomp.gps_speed) / 3600 * 1000;
				}
			} else
			if (strcmp(value, "VIN") == 0) {
				// NOP
			} else
			if (strcmp(value, "WAYPOINT") == 0) {
				// NOP
			} else
			if (strcmp(value, "ERROR") == 0) {
				// NOP
			} else {
				time_ig++;
				// Парсинг, поиск значения
				ret = csv_getpos(line, size, time_pos, value);
				if (ret) {
					// ...
				}
				ret = csv_getpos(line, size, temp_eng_pos, value);
				if (ret) {
					if (temp_eng_max < atoi(value)) temp_eng_max = atoi(value);
					if (temp_eng_min > atoi(value)) temp_eng_min = atoi(value);
				}
				ret = csv_getpos(line, size, temp_at_pos, value);
				if (ret) {
					if (atoi(value) > 116 ||
						atoi(value) < -30) {
						continue;
					}
					if (atoi(value) > 110) {
						at_110++;
					}
					if (atoi(value) > 100) {
						at_100++;
					}
					if (atoi(value) > 90) {
						at_90++;
					}
					if (atoi(value) > 80) {
						at_80++;
					}
					if (atoi(value) < 50) {
						at_50++;
					}
					if (atoi(value) < 40) {
						at_40++;
					}
					if (temp_at_max < atoi(value)) temp_at_max = atoi(value);
					if (temp_at_min > atoi(value)) temp_at_min = atoi(value);
				}
				ret = csv_getpos(line, size, temp_air_pos, value);
				if (ret) {
					if (temp_air_max < atoi(value)) temp_air_max = atoi(value);
					if (temp_air_min > atoi(value)) temp_air_min = atoi(value);
				}
				ret = csv_getpos(line, size, fuel_pos, value);
				if (ret) {
					fuel_cur = atof(value);
				}
				// Координаты:
				ret = csv_getpos(line, size, lat_pos, value);
				lat = atof(value);
				ret = csv_getpos(line, size, lon_pos, value);
				lon = atof(value);
				if (ret) {
					// Добавление точки на картинку:
					put_point(lat, lon, id);
				}
				// Скорость:
				ret = csv_getpos(line, size, speed_pos, value);
				if (ret) {
					if (speed_max < atoi(value)) speed_max = atoi(value);
					if (atoi(value)) {
						// speed != 0
						time_move++;
					}
					dist += ((double)atoi(value)) / 3600 * 1000;
				}
				// rpm-статистика:
				ret = csv_getpos(line, size, rpm_pos, value);
				if (ret) {
					if (atoi(value) == 4616 ||
						atoi(value) == 4486 || 
						atoi(value) == 4247) {
					 continue;
					}
					if (rpm_max < atoi(value)) rpm_max = atoi(value);
					if (atoi(value)) {
						time++;
					}
					if (atoi(value) > 2000) {
						rpm_2000++;
					}
					if (atoi(value) > 3000) {
						rpm_3000++;
					}
					if (atoi(value) > 3500) {
						rpm_3500++;
					}
					if (atoi(value) > 4000) {
						rpm_4000++;
					}
				}
			}
		}
		
		cnt++;
	}
	
	return 0;
}

void list_txt(char *dir) {
	DIR *dpdf;
	struct dirent *epdf;
	char fulldir[2048*2];
	dpdf = opendir(dir);
	if (dpdf != NULL) {
		while (epdf = readdir(dpdf)) {
			if (strcmp(epdf->d_name, ".") && strcmp(epdf->d_name, "..")) {
				int size = 0;
				FILE *fp;
				strcpy(fulldir, dir);
				strcat(fulldir, epdf->d_name);
				size = strlen(epdf->d_name);
				// Попытка открытия файла:
				fp = fopen(fulldir, "rb");
				// Проверка что это файл, а не директория:
				if (fp != NULL) {
					if ((epdf->d_name[size-1] == 't' || epdf->d_name[size-1] == 'T') &&
						(epdf->d_name[size-2] == 'x' ||  epdf->d_name[size-2] == 'X') &&
						(epdf->d_name[size-3] == 't' ||  epdf->d_name[size-3] == 'T') &&
						epdf->d_name[size-4] == '.') {
						// txt-file (proc)!
						printf("file: %s\r\n", epdf->d_name);
						csv_proc(fp);
					}
					// Файл, требуется чтение и обработка данных из файла.
					fclose(fp);
				} else {
					// Это директория!
					// Рекурсивный проход!
					printf("enter dir: %s\r\n", fulldir);
#if 1 // RECURSIVE
					strcat(fulldir, "/");
					list_txt(fulldir);
#endif
				}
			} else {
				// ".", ".."
			}
		}
		closedir(dpdf);
	}
}

int main(int argc, char **argv) {
	int i = 0;
	char filename[128];
	if (argc > 1) {
		id = atoi(argv[1]);
		if (id == 0) {
			while (strcmpi(places[i].alias, argv[1])) {
				i++;
				if (places[i].id == 0) {
					break;
				}
			}
			id = places[i].id;
		}
	}
	printf("id = %d\r\n", id);
	i = -1;
	do {
		i++;
		if (places[i].id == 0) {
			printf("ERROR: unknown region!\r\n");
			exit(1);
		}
	} while (places[i].id != id);
	id = i;
	printf("Region: \"%s\"\r\n", places[id].alias);
	
	bmp_create(&bmp, places[id].size_y, places[id].size_x, 8);
	bmp_setpalette8(&bmp, palette);
	list_txt("./logs/");
	sprintf(filename, "out_%s.bmp", places[id].alias);
	bmp_save(&bmp, filename);
	bmp_close(&bmp);
	
	printf("Speed (max): %dkm/h\r\n", speed_max);
	printf("RPM (max):   %drmp\r\n", rpm_max);
	printf("Moto Time:   %ds / %dm / %dh\r\n", time, time/60, time/3600);
	printf("ING Time:   %ds / %dm / %dh\r\n", time_ig, time_ig/60, time_ig/3600);
	printf("Time in move:   %ds / %dm / %dh\r\n", time_move, time_move/60, time_move/3600);
	printf("Moto Time (RPM>2000): %ds / %dm / %dh\r\n", rpm_2000, rpm_2000/60, rpm_2000/3600);
	printf("Moto Time (RPM>3000): %ds / %dm / %dh\r\n", rpm_3000, rpm_3000/60, rpm_3000/3600);
	printf("Moto Time (RPM>3500): %ds / %dm / %dh\r\n", rpm_3500, rpm_3500/60, rpm_3500/3600);
	printf("Moto Time (RPM>4000): %ds / %dm / %dh\r\n", rpm_4000, rpm_4000/60, rpm_4000/3600);
	printf("Engine temp (max):   %d*C\r\n", temp_eng_max);
	printf("Engine temp (min):   %d*C\r\n", temp_eng_min);
	printf("A/T temp (max):   %d*C\r\n", temp_at_max);
	printf("A/T temp (min):   %d*C\r\n", temp_at_min);
	printf("A/T Time (T>110): %ds / %dm / %dh\r\n", at_110, at_110/60, at_110/3600);
	printf("A/T Time (T>100): %ds / %dm / %dh\r\n", at_100, at_100/60, at_100/3600);
	printf("A/T Time (T>90): %ds / %dm / %dh\r\n", at_90, at_90/60, at_90/3600);
	printf("A/T Time (T>80): %ds / %dm / %dh\r\n", at_80, at_80/60, at_80/3600);
	printf("A/T Time (T<50): %ds / %dm / %dh\r\n", at_50, at_50/60, at_50/3600);
	printf("A/T Time (T<40): %ds / %dm / %dh\r\n", at_40, at_40/60, at_40/3600);
	printf("Air temp (max):   %d*C\r\n", temp_air_max);
	printf("Air temp (min):   %d*C\r\n", temp_air_min);
	printf("Fuel:   %.1fL\r\n", fuel);
	printf("Dist:   %dkm\r\n", (int)(dist/1000));
	
	
	return 0;
}

