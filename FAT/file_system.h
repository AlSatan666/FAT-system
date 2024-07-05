#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H
#ifndef _SSIZE_T_DEFINED
typedef long ssize_t;
#define _SSIZE_T_DEFINED
#endif

#include <stdint.h>
#include <stddef.h>

#define BLOCK_SIZE 4096
#define MAX_FILENAME_LENGTH 255

typedef struct {
    int32_t next_block;
} FATEntry;

typedef struct {
    char name[MAX_FILENAME_LENGTH];
    int8_t is_directory;
    int32_t first_block;
} DirectoryEntry;

typedef struct {
    int32_t block_index;
    int32_t position;
} FileHandle;

typedef struct {
    int fd;
    uint8_t *data;
    FATEntry *fat;
    DirectoryEntry *root_dir;
    int num_blocks;
    int fat_size;
} FileSystem;

void initialize_filesystem(FileSystem *fs, const char *file_path, int num_blocks);
int create_file(FileSystem *fs, const char *name);
int erase_file(FileSystem *fs, const char *name);
ssize_t write_file(FileSystem *fs, FileHandle *fh, const void *data, size_t size);
ssize_t read_file(FileSystem *fs, FileHandle *fh, void *buffer, size_t size);
int seek_file(FileSystem *fs, FileHandle *fh, int32_t position);
int create_dir(FileSystem *fs, const char *name);
int erase_dir(FileSystem *fs, const char *name);
int change_dir(FileSystem *fs, const char *path);
void list_dir(FileSystem *fs);

#endif
