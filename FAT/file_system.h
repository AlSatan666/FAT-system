#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <stdint.h>

int initialize_fs(const char *filename, uint32_t size);
int create_file(const char *filename);
void uninitialize_fs();

#endif // FILE_SYSTEM_H
