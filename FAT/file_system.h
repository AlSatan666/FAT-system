#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>

#define BLOCK_SIZE 512
#define BLOCKS_PER_CLUSTER 1
#define TOTAL_BLOCKS 65536
#define FILE_SYSTEM_SIZE (TOTAL_BLOCKS * BLOCK_SIZE)

#define FAT_UNUSED 0x00000000
#define FAT_END 0x0FFFFFF8
#define FAT_OCCUPIED 0xFFFFFFFF

#define DIR_ENTRY_SIZE 32
#define DELETED_ENTRY 0xE5

#define DIR_CREATE_ERROR -1
#define FILE_CREATE_ERROR -2
#define INIT_ERROR -3
#define FILE_NOT_FOUND -4
#define FILE_READ_ERROR -5
#define DIR_NOT_EMPTY -6
#define FAT_FULL -7
#define FILE_WRITE_ERROR -8
#define INVALID_DIRECTORY -9

typedef struct {
    int bytes_per_block;
    int fat_entries;
    int cluster_size;
    int fat_size;
    int data_size;
    int total_blocks;
    char current_directory[25];
} FileSystem;

typedef struct DirectoryEntry {
    char name[25];
    char extension[3];
    char is_dir;
    struct DirectoryEntry* parent;
    int first_block;
    int size;
    int entry_count;
} __attribute__((packed)) DirectoryEntry;

typedef struct FileHandle {
    DirectoryEntry* file_entry;
    int position;
} FileHandle;

extern FileSystem *fs;

int fs_initialize(const char* file_path);
int fs_load(const char* file_path);
int fs_save();

DirectoryEntry* get_current_dir();
FileSystem* get_fs();
int get_free_block();
DirectoryEntry* find_empty_dir_entry();
int cd(const char* dir_name);
void ls();
int create_dir(const char* name);
int create_file(const char* name, const char* ext, int size, const char* data);
DirectoryEntry* locate_file(const char* name, const char* ext, char is_dir);
int remove_file(const char* name, const char* ext);
int remove_empty_dir(DirectoryEntry* dir);
bool is_dir_empty(DirectoryEntry* dir);
int remove_dir(const char* name, int recursive);
void display_fs_image(unsigned int max_bytes);
int read_file_content(FileHandle *handle, char *buffer, int size);
int write_file_content(const char* name, const char* ext, const char* data, int offset, int size);
int seek_file(FileHandle *handle, int offset, int origin);
int copy2fs(const char* host_path, const char* fs_name, const char* fs_ext);
int copy2host(const char* fs_name, const char* fs_ext, const char* host_path);

#endif
