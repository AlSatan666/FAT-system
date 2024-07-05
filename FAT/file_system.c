#include "file_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>

void initialize_filesystem(FileSystem *fs, const char *file_path, int num_blocks) {
    fs->num_blocks = num_blocks;
    fs->fat_size = num_blocks * sizeof(FATEntry);

    fs->fd = open(file_path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fs->fd == -1) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(fs->fd, num_blocks * BLOCK_SIZE) == -1) {
        perror("Error setting file size");
        exit(EXIT_FAILURE);
    }

    fs->data = mmap(NULL, num_blocks * BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fs->fd, 0);
    if (fs->data == MAP_FAILED) {
        perror("Error mapping file");
        exit(EXIT_FAILURE);
    }

    fs->fat = (FATEntry *)fs->data;
    fs->root_dir = (DirectoryEntry *)(fs->data + fs->fat_size);

    // Initialize FAT and root directory if necessary
    memset(fs->fat, -1, fs->fat_size);
    memset(fs->root_dir, 0, BLOCK_SIZE - fs->fat_size);
}

int create_file(FileSystem *fs, const char *name) {
    int first_block = -1;
    for (int i = 0; i < fs->num_blocks; i++) {
        if (fs->fat[i].next_block == -1) {
            first_block = i;
            fs->fat[i].next_block = 0;
            break;
        }
    }
    if (first_block == -1) return -1;

    DirectoryEntry *dir = fs->root_dir;
    for (int i = 0; i < (BLOCK_SIZE - fs->fat_size) / sizeof(DirectoryEntry); i++) {
        if (dir[i].first_block == 0) {
            strncpy(dir[i].name, name, MAX_FILENAME_LENGTH);
            dir[i].is_directory = 0;
            dir[i].first_block = first_block;
            return 0;
        }
    }

    fs->fat[first_block].next_block = -1;
    return -1;
}

int erase_file(FileSystem *fs, const char *name) {
    DirectoryEntry *dir = fs->root_dir;
    for (int i = 0; i < (BLOCK_SIZE - fs->fat_size) / sizeof(DirectoryEntry); i++) {
        if (dir[i].first_block != 0 && strncmp(dir[i].name, name, MAX_FILENAME_LENGTH) == 0) {
            int block = dir[i].first_block;
            while (block != -1) {
                int next_block = fs->fat[block].next_block;
                fs->fat[block].next_block = -1;
                block = next_block;
            }
            dir[i].first_block = 0;
            return 0;
        }
    }
    return -1;
}

ssize_t write_file(FileSystem *fs, FileHandle *fh, const void *data, size_t size) {
    size_t total_written = 0;
    while (size > 0) {
        int block_offset = fh->position % BLOCK_SIZE;
        int block = fh->block_index;
        
        if (block == -1) return -1;

        int to_write = BLOCK_SIZE - block_offset;
        if (to_write > size) to_write = size;

        memcpy(fs->data + block * BLOCK_SIZE + block_offset, data + total_written, to_write);

        fh->position += to_write;
        total_written += to_write;
        size -= to_write;

        if (block_offset + to_write == BLOCK_SIZE) {
            int next_block = fs->fat[block].next_block;
            if (next_block == 0) {
                next_block = -1;
                for (int i = 0; i < fs->num_blocks; i++) {
                    if (fs->fat[i].next_block == -1) {
                        next_block = i;
                        fs->fat[i].next_block = 0;
                        fs->fat[block].next_block = next_block;
                        break;
                    }
                }
                if (next_block == -1) return total_written;
            }
            fh->block_index = next_block;
        }
    }
    return total_written;
}

ssize_t read_file(FileSystem *fs, FileHandle *fh, void *buffer, size_t size) {
    size_t total_read = 0;
    while (size > 0) {
        int block_offset = fh->position % BLOCK_SIZE;
        int block = fh->block_index;
        
        if (block == -1) return total_read;

        int to_read = BLOCK_SIZE - block_offset;
        if (to_read > size) to_read = size;

        memcpy(buffer + total_read, fs->data + block * BLOCK_SIZE + block_offset, to_read);

        fh->position += to_read;
        total_read += to_read;
        size -= to_read;

        if (block_offset + to_read == BLOCK_SIZE) {
            block = fs->fat[block].next_block;
            if (block == 0) return total_read;
            fh->block_index = block;
        }
    }
    return total_read;
}

int seek_file(FileSystem *fs, FileHandle *fh, int32_t position) {
    int block_index = fh->block_index;
    while (position >= BLOCK_SIZE) {
        if (block_index == -1 || fs->fat[block_index].next_block == 0) {
            return -1;
        }
        block_index = fs->fat[block_index].next_block;
        position -= BLOCK_SIZE;
    }
    fh->block_index = block_index;
    fh->position = position;
    return 0;
}

int create_dir(FileSystem *fs, const char *name) {
    int first_block = -1;
    for (int i = 0; i < fs->num_blocks; i++) {
        if (fs->fat[i].next_block == -1) {
            first_block = i;
            fs->fat[i].next_block = 0;
            break;
        }
    }
    if (first_block == -1) return -1;

    DirectoryEntry *dir = fs->root_dir;
    for (int i = 0; i < (BLOCK_SIZE - fs->fat_size) / sizeof(DirectoryEntry); i++) {
        if (dir[i].first_block == 0) {
            strncpy(dir[i].name, name, MAX_FILENAME_LENGTH);
            dir[i].is_directory = 1;
            dir[i].first_block = first_block;
            return 0;
        }
    }

    fs->fat[first_block].next_block = -1;
    return -1;
}

int erase_dir(FileSystem *fs, const char *name) {
    DirectoryEntry *dir = fs->root_dir;
    for (int i = 0; i < (BLOCK_SIZE - fs->fat_size) / sizeof(DirectoryEntry); i++) {
        if (dir[i].first_block != 0 && strncmp(dir[i].name, name, MAX_FILENAME_LENGTH) == 0) {
            if (dir[i].is_directory) {
                int block = dir[i].first_block;
                while (block != -1) {
                    int next_block = fs->fat[block].next_block;
                    fs->fat[block].next_block = -1;
                    block = next_block;
                }
                dir[i].first_block = 0;
                return 0;
            }
        }
    }
    return -1;
}

int change_dir(FileSystem *fs, const char *path) {
    DirectoryEntry *dir = fs->root_dir;
    for (int i = 0; i < (BLOCK_SIZE - fs->fat_size) / sizeof(DirectoryEntry); i++) {
        if (dir[i].first_block != 0 && strncmp(dir[i].name, path, MAX_FILENAME_LENGTH) == 0) {
            if (dir[i].is_directory) {
                fs->root_dir = (DirectoryEntry *)(fs->data + dir[i].first_block * BLOCK_SIZE);
                return 0;
            }
        }
    }
    return -1;
}

void list_dir(FileSystem *fs) {
    DirectoryEntry *dir = fs->root_dir;
    for (int i = 0; i < (BLOCK_SIZE - fs->fat_size) / sizeof(DirectoryEntry); i++) {
        if (dir[i].first_block != 0) {
            printf("%s%s\n", dir[i].name, dir[i].is_directory ? "/" : "");
        }
    }
}

