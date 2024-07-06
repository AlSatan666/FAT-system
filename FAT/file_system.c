#include "file_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#define BLOCK_SIZE 4096
#define FAT_ENTRIES_PER_BLOCK (BLOCK_SIZE / sizeof(uint32_t))
#define MAX_FILENAME_LENGTH 255

typedef struct {
    char name[MAX_FILENAME_LENGTH + 1];
    uint32_t size;
    uint32_t first_block;
} FileEntry;

static int fs_fd = -1;
static void *fs_base = NULL;
static uint32_t *fat = NULL;
static FileEntry *root_dir = NULL;
static uint32_t fs_size = 0;

int initialize_fs(const char *filename, uint32_t size) {
    fs_fd = open(filename, O_RDWR | O_CREAT, 0644);
    if (fs_fd < 0) {
        return -1;
    }

    if (ftruncate(fs_fd, size) < 0) {
        close(fs_fd);
        return -1;
    }

    fs_base = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fs_fd, 0);
    if (fs_base == MAP_FAILED) {
        close(fs_fd);
        return -1;
    }

    fat = (uint32_t *)fs_base;
    root_dir = (FileEntry *)(fs_base + BLOCK_SIZE);
    fs_size = size;

    // Initialize FAT
    memset(fat, 0xFF, FAT_ENTRIES_PER_BLOCK * sizeof(uint32_t));
    for (int i = 0; i < FAT_ENTRIES_PER_BLOCK; i++) {
        fat[i] = 0xFFFFFFFF;
    }
    printf("FAT initialized.\n");

    // Calculate root directory entries
    int root_dir_entries = (size - BLOCK_SIZE) / sizeof(FileEntry);
    memset(root_dir, 0, root_dir_entries * sizeof(FileEntry));
    printf("Root directory initialized with %d entries.\n", root_dir_entries);

    return 0;
}

int create_file(const char *filename) {
    if (strlen(filename) > MAX_FILENAME_LENGTH) {
        printf("Filename too long.\n");
        return -1;
    }

    int root_dir_entries = (fs_size - BLOCK_SIZE) / sizeof(FileEntry);

    // Check if file already exists
    for (int i = 0; i < root_dir_entries; i++) {
        if (strncmp(root_dir[i].name, filename, MAX_FILENAME_LENGTH) == 0) {
            printf("File already exists.\n");
            return -1;  // File already exists
        }
    }

    // Find an empty entry
    for (int i = 0; i < root_dir_entries; i++) {
        if (root_dir[i].name[0] == '\0') {
            strncpy(root_dir[i].name, filename, MAX_FILENAME_LENGTH);
            root_dir[i].size = 0;
            root_dir[i].first_block = 0xFFFFFFFF;
            printf("File created: %s\n", filename);
            return 0;
        }
    }

    printf("No space left in root directory.\n");
    return -1;  // No space left in root directory
}

int erase_file(const char *filename) {
    int root_dir_entries = (fs_size - BLOCK_SIZE) / sizeof(FileEntry);

    // Find the file
    for (int i = 0; i < root_dir_entries; i++) {
        if (strncmp(root_dir[i].name, filename, MAX_FILENAME_LENGTH) == 0) {
            // Free FAT blocks
            uint32_t current_block = root_dir[i].first_block;
            while (current_block != 0xFFFFFFFF) {
                uint32_t next_block = fat[current_block];
                fat[current_block] = 0xFFFFFFFF;
                current_block = next_block;
            }

            // Clear directory entry
            memset(&root_dir[i], 0, sizeof(FileEntry));
            printf("File erased: %s\n", filename);
            return 0;
        }
    }

    printf("File not found: %s\n", filename);
    return -1;  // File not found
}

void uninitialize_fs() {
    if (fs_base != NULL) {
        munmap(fs_base, fs_size);
        fs_base = NULL;
    }

    if (fs_fd >= 0) {
        close(fs_fd);
        fs_fd = -1;
    }
}

