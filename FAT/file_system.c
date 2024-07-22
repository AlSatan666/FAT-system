#include "file_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>

FileSystem *fs;
DirectoryEntry *current_dir;
int *fat_table;
char *data_blocks;
FILE *file_system_file;

int fs_initialize(const char* file_path) {
    int fd = open(file_path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        printf("Error opening file system file\n");
        return INIT_ERROR;
    }

    if (ftruncate(fd, FILE_SYSTEM_SIZE) == -1) {
        printf("Error setting file size\n");
        close(fd);
        return INIT_ERROR;
    }

    void* mapped = mmap(NULL, FILE_SYSTEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped == MAP_FAILED) {
        printf("Error mapping file\n");
        close(fd);
        return INIT_ERROR;
    }

    file_system_file = fdopen(fd, "wb+");
    if (!file_system_file) {
        printf("Error creating file system file\n");
        munmap(mapped, FILE_SYSTEM_SIZE);
        close(fd);
        return INIT_ERROR;
    }

    fs = (FileSystem*)mapped;
    fs->bytes_per_block = BLOCK_SIZE;
    fs->total_blocks = TOTAL_BLOCKS;
    fs->fat_entries = TOTAL_BLOCKS;
    fs->fat_size = fs->fat_entries * sizeof(int);
    fs->data_size = FILE_SYSTEM_SIZE - fs->fat_size - sizeof(FileSystem);
    strcpy(fs->current_directory, "ROOT");

    fat_table = (int*)((char*)mapped + sizeof(FileSystem));
    for (int i = 0; i < fs->fat_entries; i++) {
        fat_table[i] = FAT_UNUSED;
    }

    data_blocks = (char*)mapped + sizeof(FileSystem) + fs->fat_size;
    memset(data_blocks, 0x00, fs->data_size);

    current_dir = (DirectoryEntry*)data_blocks;
    current_dir->first_block = 0;
    strncpy(current_dir->name, "ROOT", 8);
    memset(current_dir->extension, 0, 3);
    current_dir->entry_count = 0;
    current_dir->is_dir = 1;
    current_dir->parent = NULL;
    fat_table[0] = FAT_END;

    printf("fs_initialize: Created new file system: PASSED\n");

    return 0;
}

int fs_load(const char* file_path) {
    int fd = open(file_path, O_RDWR);
    if (fd == -1) {
        printf("Error opening file system file\n");
        return INIT_ERROR;
    }

    void* mapped = mmap(NULL, FILE_SYSTEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped == MAP_FAILED) {
        printf("Error mapping file\n");
        close(fd);
        return INIT_ERROR;
    }

    file_system_file = fdopen(fd, "rb+");
    if (!file_system_file) {
        printf("Error opening file system file\n");
        munmap(mapped, FILE_SYSTEM_SIZE);
        close(fd);
        return INIT_ERROR;
    }

    fs = (FileSystem*)mapped;
    fat_table = (int*)((char*)mapped + sizeof(FileSystem));
    data_blocks = (char*)mapped + sizeof(FileSystem) + fs->fat_size;
    current_dir = (DirectoryEntry*)data_blocks;

    printf("fs_load: PASSED\n");
    printf("fs_load: Loaded file system from DATATICUS file.\n");

    return 0;
}

int fs_save() {
    if (!file_system_file) {
        printf("fs_save: File system file not open\n");
        return FILE_WRITE_ERROR;
    }

    if (msync(fs, FILE_SYSTEM_SIZE, MS_SYNC) == -1) {
        printf("fs_save: Failed to sync memory to file\n");
        return FILE_WRITE_ERROR;
    }

    printf("fs_save: PASSED\n");
    printf("fs_save: Successfully saved file system to DATATICUS file.\n");

    return 0;
}



DirectoryEntry* get_current_dir() {
    return current_dir;
}

FileSystem* get_fs() {
    return fs;
}

int get_free_block() {
    for (int i = 1; i < fs->fat_entries; i++) {
        if (fat_table[i] == FAT_UNUSED) {
            return i;
        }
    }
    return FAT_FULL;
}

DirectoryEntry* find_empty_dir_entry() {
    int block = current_dir->first_block;
    while (block != FAT_END) {
        DirectoryEntry* dir = (DirectoryEntry*)&data_blocks[block * fs->bytes_per_block];
        for (int i = 0; i < fs->bytes_per_block / sizeof(DirectoryEntry); i++) {
            DirectoryEntry* entry = &dir[i];
            if (entry->name[0] == 0x00 || (unsigned char)entry->name[0] == DELETED_ENTRY) {
                return entry;
            }
        }
        block = fat_table[block];
    }
    return NULL;
}

int cd(const char* dir_name) {
    printf("Changing to directory: %s\n", dir_name);

    if (strcmp(dir_name, ".") == 0) {
        return INVALID_DIRECTORY; 
    }

    if (strcmp(dir_name, "..") == 0) {
        if (current_dir->parent != NULL) {
            current_dir = current_dir->parent;
            strcpy(fs->current_directory, current_dir->name);
        }
        return 0;
    }

    int block = current_dir->first_block;
    while (block != FAT_END) {
        DirectoryEntry* dir = (DirectoryEntry*)&data_blocks[block * fs->bytes_per_block];
        for (int i = 0; i < fs->bytes_per_block / sizeof(DirectoryEntry); i++) {
            DirectoryEntry* entry = &dir[i];
            if (strcmp(entry->name, dir_name) == 0 && entry->is_dir) {
                current_dir = (DirectoryEntry*)&data_blocks[entry->first_block * fs->bytes_per_block];
                current_dir->parent = dir;
                strcpy(fs->current_directory, entry->name);
                return 0;
            }
        }
        block = fat_table[block];
        if (block < 0 || block >= fs->fat_entries) {
            printf("Error: Block index out of bounds: %d\n", block);
            return FILE_NOT_FOUND;
        }
    }
    return FILE_NOT_FOUND;
}

void ls() {
    if (current_dir == NULL) {
        printf("Error: current_dir is NULL\n");
        return;
    }

    int block = current_dir->first_block;
    printf("Contents of directory (%s):\n", fs->current_directory);
    while (block != FAT_END) {
        DirectoryEntry* dir = (DirectoryEntry*)&data_blocks[block * fs->bytes_per_block];
        for (int i = 0; i < fs->bytes_per_block / sizeof(DirectoryEntry); i++) {
            DirectoryEntry* entry = &dir[i];
            if (entry->name[0] == 0x00) {
                continue;
            }
            if ((unsigned char)entry->name[0] == DELETED_ENTRY) {
                continue;
            }
            if (entry->is_dir) {
                printf("%.25s/\t", entry->name);
            } else {
                printf("%.25s.%.3s\t", entry->name, entry->extension);
            }
        }
        int next_block = fat_table[block];
        if (next_block == FAT_UNUSED || next_block == FAT_END || next_block < 0 || next_block >= fs->fat_entries) {
            break;
        }
        block = next_block;
    }
    printf("\n");
}

int create_dir(const char* name) {
    printf("Creating directory: %s\n", name);

    DirectoryEntry* entry = find_empty_dir_entry();
    if (entry == NULL) {
        printf("Error: No empty directory entry found\n");
        return DIR_CREATE_ERROR;
    }

    memset(entry->name, ' ', sizeof(entry->name));
    strncpy(entry->name, name, 24);
    entry->name[24] = '\0';
    entry->size = 0;
    entry->is_dir = 1;

    int block = get_free_block();
    if (block == FAT_FULL) {
        printf("Error: No free block available\n");
        return DIR_CREATE_ERROR;
    }

    printf("Allocating block %d for directory %s\n", block, name);
    fat_table[block] = FAT_END;
    DirectoryEntry* new_dir = (DirectoryEntry*)&data_blocks[block * fs->bytes_per_block];
    memset(new_dir, 0, fs->bytes_per_block);

    entry->first_block = block;
    strncpy(new_dir[0].name, ".", 8);
    new_dir[0].first_block = block;
    new_dir[0].is_dir = 1;
    new_dir[0].size = 0;
    new_dir[0].parent = current_dir;

    strncpy(new_dir[1].name, "..", 8);
    new_dir[1].first_block = current_dir->first_block;
    new_dir[1].is_dir = 1;
    new_dir[1].size = 0;
    new_dir[1].parent = current_dir;

    fs_save();

    printf("Directory created: %s at block %d\n", entry->name, block);

    return 0;
}

int create_file(const char* name, const char* ext, int size, const char* data) {
    DirectoryEntry* entry = find_empty_dir_entry();
    if (entry == NULL) {
        return FILE_CREATE_ERROR;
    }

    memset(entry->name, ' ', sizeof(entry->name));
    memset(entry->extension, ' ', sizeof(entry->extension));

    strncpy(entry->name, name, 24);
    entry->name[24] = '\0';
    strncpy(entry->extension, ext, 3);
    entry->extension[3] = '\0';
    entry->size = size;
    entry->is_dir = 0;

    int block = get_free_block();
    if (block == FAT_FULL) {
        return FILE_CREATE_ERROR;
    }

    entry->parent = current_dir;
    entry->first_block = block;
    int block_size = fs->bytes_per_block;
    int blocks_needed = (size + block_size - 1) / block_size;
    int current_block = block;

    for (int i = 0; i < blocks_needed; i++) {
        if (current_block >= fs->fat_entries) {
            printf("Error: Invalid block allocation.\n");
            return FILE_CREATE_ERROR;
        }
        memcpy(&data_blocks[current_block * block_size], &data[i * block_size], block_size);
        int next_block = get_free_block();
        if (i == blocks_needed - 1) {
            fat_table[current_block] = FAT_END;
        } else {
            fat_table[current_block] = next_block;
        }
        current_block = next_block;
    }

    fs_save();

    return 0;
}

DirectoryEntry* locate_file(const char* name, const char* ext, char is_dir) {
    int block = current_dir->first_block;
    printf("locate_file: Searching for %s.%s in directory %s\n", name, ext, current_dir->name);
    while (block != FAT_END) {
        DirectoryEntry* dir = (DirectoryEntry*)&data_blocks[block * fs->bytes_per_block];
        for (int i = 0; i < fs->bytes_per_block / sizeof(DirectoryEntry); i++) {
            DirectoryEntry* entry = &dir[i];
            printf("locate_file: Checking entry %.25s.%.3s\n", entry->name, entry->extension);
            if (strncmp(entry->name, name, 24) == 0 && strncmp(entry->extension, ext, 3) == 0 && entry->is_dir == is_dir) {
                printf("locate_file: Found %.25s.%.3s\n", name, ext);
                return entry;
            }
        }
        block = fat_table[block];
    }
    printf("locate_file: %s.%s not found\n", name, ext);
    return NULL;
}

int is_directory_empty(DirectoryEntry* dir) {
    int block = dir->first_block;
    while (block != FAT_END) {
        DirectoryEntry* d = (DirectoryEntry*)&data_blocks[block * fs->bytes_per_block];
        for (int i = 0; i < fs->bytes_per_block / sizeof(DirectoryEntry); i++) {
            DirectoryEntry* entry = &d[i];
            if (entry->name[0] != 0x00 && (unsigned char)entry->name[0] != DELETED_ENTRY && strcmp(entry->name, ".") != 0 && strcmp(entry->name, "..") != 0) {
                return 0; 
            }
        }
        block = fat_table[block];
    }
    return 1; 
}

int remove_file(const char* name, const char* ext) {
    printf("Attempting to remove file: %s.%s\n", name, ext);
    DirectoryEntry* file = locate_file(name, ext, 0);
    if (file == NULL) {
        printf("File not found: %s.%s\n", name, ext);
        return FILE_NOT_FOUND;
    }

    printf("Removing file: %s.%s\n", name, ext);

    int current_block = file->first_block;
    int next_block;

    next_block = -1;//non valido

    while (current_block != FAT_END && current_block != FAT_UNUSED) {
        printf("Clearing block %d\n", current_block);

        next_block = fat_table[current_block];

        if (next_block < 0 || next_block >= fs->fat_entries) {
            printf("Error: Invalid next block %d from block %d\n", next_block, current_block);
            return FILE_WRITE_ERROR;
        }

        fat_table[current_block] = FAT_UNUSED;
        memset(&data_blocks[current_block * fs->bytes_per_block], 0x00, fs->bytes_per_block);

        current_block = next_block;
    }

    file->name[0] = DELETED_ENTRY;

    if (fs_save() != 0) {
        printf("Error saving file system state\n");
        return FILE_WRITE_ERROR;
    }

    printf("File removed: %s.%s\n", name, ext);
    return 0;
}


int remove_dir(const char* name, int recursive) {
    printf("Attempting to remove directory: %s\n", name);
    DirectoryEntry* dir = locate_file(name, "", 1);
    if (dir == NULL) {
        printf("Directory not found: %s\n", name);
        return FILE_NOT_FOUND;
    }

    if (is_directory_empty(dir)) {
        printf("Directory is empty: %s\n", name);
        return remove_empty_dir(dir);
    } else if (recursive == 1) {
        DirectoryEntry* temp = current_dir;
        current_dir = dir;

        int block = dir->first_block;
        while (block != FAT_END) {
            DirectoryEntry* d = (DirectoryEntry*)&data_blocks[block * fs->bytes_per_block];
            for (int i = 0; i < fs->bytes_per_block / sizeof(DirectoryEntry); i++) {
                DirectoryEntry* entry = &d[i];
                if (entry->name[0] == 0x00 || entry->name[0] == DELETED_ENTRY || strcmp(entry->name, ".") == 0 || strcmp(entry->name, "..") == 0) {
                    continue;
                }
                if (entry->is_dir) {
                    int res = remove_dir(entry->name, recursive);
                    if (res != 0) return res;
                } else {
                    int res = remove_file(entry->name, entry->extension);
                    if (res != 0) return res;
                }
            }
            block = fat_table[block];
        }

        current_dir = temp;
        printf("Directory removed: %s\n", name);
        return remove_empty_dir(dir);
    } else {
        printf("Directory not empty and recursive flag not set: %s\n", name);
        return DIR_NOT_EMPTY;
    }
}

int remove_empty_dir(DirectoryEntry* dir) {
    int current_block = dir->first_block;
    while (current_block != FAT_END) {
        printf("Clearing block %d\n", current_block);
        memset(&data_blocks[current_block * fs->bytes_per_block], 0x00, fs->bytes_per_block);
        int next_block = fat_table[current_block];
        fat_table[current_block] = FAT_UNUSED;
        current_block = next_block;
    }

    memset(dir, 0x00, sizeof(DirectoryEntry));

    fs_save();
    printf("Directory removed.\n");
    return 0;
}


void display_fs_image(unsigned int max_bytes) {
    if (max_bytes > fs->bytes_per_block * fs->total_blocks) {
        max_bytes = fs->bytes_per_block * fs->total_blocks;
    }
    for (int i = 0; i < max_bytes; i++) {
        printf(" <%02x> ", *(fat_table + i));
    }
    printf("\n");
}



int read_file_content(FileHandle *handle, char *buffer, int size) {
    if (!handle || !handle->file_entry || !buffer || size <= 0) {
        printf("read_file_content: Invalid parameters\n");
        return FILE_READ_ERROR;
    }

    DirectoryEntry* file_entry = handle->file_entry;
    int bytes_read = 0;
    int total_size = file_entry->size;
    int current_block = file_entry->first_block;
    int byte_offset = handle->position % BLOCK_SIZE;

    printf("read_file_content: Reading from file '%s.%s', size %d, starting at position %d\n",
           file_entry->name, file_entry->extension, total_size, handle->position);

    if (handle->position >= total_size) {
        printf("read_file_content: Nothing to read (position beyond file size)\n");
        return 0;
    }

    for (int i = 0; i < handle->position / BLOCK_SIZE; i++) {
        current_block = fat_table[current_block];
        if (current_block == FAT_END) {
            printf("read_file_content: Reached end of file\n");
            return bytes_read;
        }
    }

    while (bytes_read < size && handle->position < total_size) {
        int bytes_to_copy = BLOCK_SIZE - byte_offset;
        if (bytes_read + bytes_to_copy > size) {
            bytes_to_copy = size - bytes_read;
        }
        if (handle->position + bytes_to_copy > total_size) {
            bytes_to_copy = total_size - handle->position;
        }

        printf("read_file_content: Reading %d bytes from block %d at offset %d\n", bytes_to_copy, current_block, byte_offset);
        
        memcpy(buffer + bytes_read, data_blocks + current_block * BLOCK_SIZE + byte_offset, bytes_to_copy);
        bytes_read += bytes_to_copy;
        handle->position += bytes_to_copy;
        byte_offset = 0;

        if (handle->position < total_size) {
            current_block = fat_table[current_block];
            if (current_block == FAT_END) {
                printf("read_file_content: Reached end of file\n");
                break;
            }
        }
    }

    if (bytes_read < size) {
        buffer[bytes_read] = '\0';
    } else {
        buffer[size - 1] = '\0';
    }

    printf("read_file_content: Total bytes read: %d\n", bytes_read);
    printf("read_file_content: Content read: \"");
    for (int i = 0; i < bytes_read; i++) {
        printf("%c", buffer[i]);
    }
    printf("\"\n");

    return bytes_read;
}


int write_file_content(const char* name, const char* ext, const char* data, int offset, int size) {
    DirectoryEntry* file = locate_file(name, ext, 0);
    if (file == NULL) {
        return FILE_NOT_FOUND;
    }

    if (offset == -1) {
        offset = file->size;
    }

    int current_block = file->first_block;
    int block_size = fs->bytes_per_block;
    int blocks_to_skip = offset / block_size;
    int byte_offset = offset % block_size;

    for (int i = 0; i < blocks_to_skip; i++) {
        current_block = fat_table[current_block];
        if (current_block == FAT_END || current_block == FAT_UNUSED || current_block >= fs->fat_entries) {
            return FILE_WRITE_ERROR;
        }
    }

    int bytes_written = 0;
    while (size > 0) {
        if (current_block == FAT_END) {
            int new_block = get_free_block();
            if (new_block == FAT_FULL) {
                return FILE_WRITE_ERROR;
            }
            fat_table[current_block] = new_block;
            fat_table[new_block] = FAT_END;
            current_block = new_block;
        }

        int bytes_to_write = (size + byte_offset > block_size) ? block_size - byte_offset : size;
        memcpy(&data_blocks[current_block * block_size + byte_offset], data + bytes_written, bytes_to_write);

        size -= bytes_to_write;
        bytes_written += bytes_to_write;
        byte_offset = 0;

        current_block = fat_table[current_block];
    }

    file->size = offset + bytes_written > file->size ? offset + bytes_written : file->size;

    fs_save();

    printf("write_file_content: Written %d bytes to file '%s.%s' starting at offset %d\n", bytes_written, name, ext, offset);
    return bytes_written;
}

int seek_file(FileHandle *handle, int offset, int origin) {
    int new_position = handle->position;

    if (origin == SEEK_SET) {
        new_position = offset;
    } else if (origin == SEEK_CUR) {
        new_position += offset;
    } else if (origin == SEEK_END) {
        new_position = handle->file_entry->size + offset; 
    } else {
        return -1;
    }

    if (new_position < 0 || new_position > handle->file_entry->size) {
        return -1; 
    }

    handle->position = new_position;
    return 0;
}

