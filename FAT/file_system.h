#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_FILE_NAME 255
#define BLOCK_SIZE 512
#define MAX_BLOCKS 1024
#define MAX_FILES 100

typedef struct {
    char name[MAX_FILE_NAME];
    int size;
    int start_block;
} FileEntry;

typedef struct {
    char name[MAX_FILE_NAME];
    int start_block;
    int size;
} DirectoryEntry;

typedef struct {
    FileEntry files[MAX_FILES];
    DirectoryEntry dirs[MAX_FILES];
    int file_count;
    int dir_count;
    int current_dir; // New field for current directory index
} FAT;

typedef struct {
    int block_index;
    int position;
} FileHandle;

void initialize_filesystem(const char *filepath);
FileHandle *createFile(const char *filename);
void eraseFile(const char *filename);
void writeFile(FileHandle *handle, const void *buffer, int size);
void readFile(FileHandle *handle, void *buffer, int size);
void seekFile(FileHandle *handle, int position);
void createDir(const char *dirname);
void eraseDir(const char *dirname);
void changeDir(const char *dirname);
void listDir();

#endif // FILE_SYSTEM_H
