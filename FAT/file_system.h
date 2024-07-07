#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#define BLOCK_SIZE 512
#define BLOCKS_PER_CLUSTER 1
#define TOTAL_BLOCKS 65536

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

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int bytes_per_block;
    int fat_entries;
    int cluster_size;
    int fat_size;
    int data_size;
    int total_blocks;
} FileSystem;

typedef struct DirectoryEntry {
    char name[8];
    char extension[3];
    char is_dir;
    struct DirectoryEntry* parent;
    int first_cluster;
    int size;
    int entry_count;
} __attribute__((packed)) DirectoryEntry;

int fs_initialize(const char* fat_path, const char* data_path);
DirectoryEntry* get_current_dir();
FileSystem* get_fs();
int get_free_cluster();
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
int read_file_content(const char* name, const char* ext, char* buffer);

#endif
