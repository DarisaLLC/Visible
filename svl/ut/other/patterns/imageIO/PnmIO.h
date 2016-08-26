#pragma once

int get_pnm_dim(const char * in_file_name, int * width, int * height);

int read_pgm_image_and_dim(const char * in_file_name, unsigned char * buffer, int buffsize, int * actual_width, int * actual_height);
int save_image_to_pgm_file(const char * file_name, const unsigned char * image, int width, int height);

int read_ppm_image_and_dim(const char * in_file_name, unsigned char * rgb, int buffsize, int * actual_width, int * actual_height);
int save_image_to_ppm_file(const char * file_name, const unsigned char * rgb, int width, int height);

int read_psm_image_and_dim(const char * in_file_name, unsigned short * buffer, int buffsize, int * actual_width, int * actual_height);
int save_image_to_psm_file(const char * file_name, const unsigned short * image, int width, int height, unsigned long maxVal = 0xffff);
