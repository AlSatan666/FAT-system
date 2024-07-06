#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <stdint.h>

// Costanti
#define BLOCK_SIZE 4096
#define FAT_ENTRIES_PER_BLOCK (BLOCK_SIZE / sizeof(uint32_t))
#define MAX_FILENAME_LENGTH 255

// Strutture
typedef struct {
    char name[MAX_FILENAME_LENGTH + 1];
    uint32_t size;
    uint32_t first_block;
} FileEntry;

// Dichiarazioni delle funzioni
int initialize_fs(const char *filename, uint32_t size);
int create_file(const char *filename);
int erase_file(const char *filename);
int write_file(const char *filename, uint32_t offset, const void *data, uint32_t size);
int read_file(const char *filename, uint32_t offset, void *buffer, uint32_t size);
void uninitialize_fs();

#endif // FILE_SYSTEM_H
