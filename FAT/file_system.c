#include "file_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // for access()

FileSystem *fs;
DirectoryEntry *current_dir;
int *fat_table;
char *data_blocks;
FILE *file_system_file;

int fs_initialize(const char* file_path) {
    if (access(file_path, F_OK) != -1) {
        printf("File already exists. Attempting to load the existing file system.\n");
        int load_result = fs_load(file_path);
        if (load_result == 0) {
            printf("fs_initialize: Loading existing file system: PASSED\n");
            return 0;
        } else {
            printf("fs_initialize: Loading existing file system: FAILED\n");
            return INIT_ERROR;
        }
    }

    file_system_file = fopen(file_path, "wb+");
    if (!file_system_file) {
        printf("Error creating file system file\n");
        return INIT_ERROR;
    }

    fs = (FileSystem*)malloc(sizeof(FileSystem));
    fs->bytes_per_block = BLOCK_SIZE;
    fs->total_blocks = TOTAL_BLOCKS;
    fs->cluster_size = BLOCK_SIZE * BLOCKS_PER_CLUSTER;
    fs->fat_entries = (fs->total_blocks) * fs->bytes_per_block / fs->cluster_size;
    fs->fat_size = fs->fat_entries * 4;
    fs->data_size = (fs->total_blocks - (fs->fat_size / fs->bytes_per_block)) * fs->bytes_per_block - sizeof(FileSystem);
    strcpy(fs->current_directory, "ROOT");

    fat_table = (int*)malloc(fs->fat_size);
    memset(fat_table, FAT_UNUSED, fs->fat_size);

    data_blocks = (char*)malloc(fs->data_size);
    memset(data_blocks, 0x00, fs->data_size);

    current_dir = (DirectoryEntry*)data_blocks;
    current_dir->first_cluster = 0;
    strncpy(current_dir->name, "ROOT", 8);
    memset(current_dir->extension, 0, 3);
    current_dir->entry_count = 0;
    current_dir->is_dir = 1;
    current_dir->parent = NULL;
    fat_table[0] = FAT_END;

    fwrite(fs, sizeof(FileSystem), 1, file_system_file);
    fwrite(fat_table, fs->fat_size, 1, file_system_file);
    fwrite(data_blocks, fs->data_size, 1, file_system_file);

    printf("fs_initialize: Created new file system: PASSED\n");

    return 0;
}

int fs_load(const char* file_path) {
    file_system_file = fopen(file_path, "rb+");
    if (!file_system_file) {
        printf("Error opening file system file\n");
        return INIT_ERROR;
    }

    fs = (FileSystem*)malloc(sizeof(FileSystem));
    fread(fs, sizeof(FileSystem), 1, file_system_file);

    fat_table = (int*)malloc(fs->fat_size);
    fread(fat_table, fs->fat_size, 1, file_system_file);

    data_blocks = (char*)malloc(fs->data_size);
    fread(data_blocks, fs->data_size, 1, file_system_file);

    current_dir = (DirectoryEntry*)data_blocks;

    return 0;
}


int fs_save() {
    if (!file_system_file) {
        printf("fs_save: File system file not open\n");
        return FILE_WRITE_ERROR;
    }

    fseek(file_system_file, 0, SEEK_SET);
    size_t fs_write_size = fwrite(fs, sizeof(FileSystem), 1, file_system_file);
    if (fs_write_size != 1) {
        printf("fs_save: Failed to write FileSystem struct to file\n");
        return FILE_WRITE_ERROR;
    }

    size_t fat_write_size = fwrite(fat_table, fs->fat_size, 1, file_system_file);
    if (fat_write_size != 1) {
        printf("fs_save: Failed to write FAT table to file\n");
        return FILE_WRITE_ERROR;
    }

    size_t data_write_size = fwrite(data_blocks, fs->data_size, 1, file_system_file);
    if (data_write_size != 1) {
        printf("fs_save: Failed to write data blocks to file\n");
        return FILE_WRITE_ERROR;
    }

    return 0;
}



DirectoryEntry* get_current_dir() {
    return current_dir;
}

FileSystem* get_fs() {
    return fs;
}

int get_free_cluster() {
    for (int i = 1; i < fs->fat_entries; i++) {
        if (fat_table[i] == FAT_UNUSED) {
            return i;
        }
    }
    return FAT_FULL;
}

DirectoryEntry* find_empty_dir_entry() {
    int cluster = current_dir->first_cluster;
    while (cluster != FAT_END) {
        DirectoryEntry* dir = (DirectoryEntry*)&data_blocks[cluster * fs->cluster_size];
        for (int i = 0; i < fs->cluster_size / sizeof(DirectoryEntry); i++) {
            DirectoryEntry* entry = &dir[i];
            if (entry->name[0] == 0x00 || (unsigned char)entry->name[0] == DELETED_ENTRY) {
                return entry;
            }
        }
        cluster = fat_table[cluster];
    }
    return NULL;
}

int cd(const char* dir_name) {
    printf("Changing to directory: %s\n", dir_name);
    if (strcmp(dir_name, "..") == 0) {
        if (current_dir->parent != NULL) {
            current_dir = current_dir->parent;
            strcpy(fs->current_directory, current_dir->name);
        }
        return 0;
    }

    int cluster = current_dir->first_cluster;
    while (cluster != FAT_END) {
        DirectoryEntry* dir = (DirectoryEntry*)&data_blocks[cluster * fs->cluster_size];
        for (int i = 0; i < fs->cluster_size / sizeof(DirectoryEntry); i++) {
            DirectoryEntry* entry = &dir[i];
            if (strcmp(entry->name, dir_name) == 0 && entry->is_dir) {
                current_dir = (DirectoryEntry*)&data_blocks[entry->first_cluster * fs->cluster_size];
                current_dir->parent = dir;
                strcpy(fs->current_directory, entry->name);
                return 0;
            }
        }
        cluster = fat_table[cluster];
    }
    return FILE_NOT_FOUND;
}

void ls() {
    int cluster = current_dir->first_cluster;
    printf("Contents of directory (%s):\n", fs->current_directory);
    while (cluster != FAT_END) {
        DirectoryEntry* dir = (DirectoryEntry*)&data_blocks[cluster * fs->cluster_size];
        for (int i = 0; i < fs->cluster_size / sizeof(DirectoryEntry); i++) {
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
        cluster = fat_table[cluster];
    }
    printf("\n");
}

int create_dir(const char* name) {
    DirectoryEntry* entry = find_empty_dir_entry();
    if (entry == NULL) {
        return DIR_CREATE_ERROR;
    }

    memset(entry->name, ' ', sizeof(entry->name));
    strncpy(entry->name, name, 24);
    entry->name[24] = '\0';
    entry->size = 0;
    entry->is_dir = 1;

    int cluster = get_free_cluster();
    if (cluster == FAT_FULL) {
        return DIR_CREATE_ERROR;
    }

    fat_table[cluster] = FAT_END;
    DirectoryEntry* new_dir = (DirectoryEntry*)&data_blocks[cluster * fs->cluster_size];
    memset(new_dir, 0, fs->cluster_size);

    entry->first_cluster = cluster;
    strncpy(new_dir[0].name, ".", 8);
    new_dir[0].first_cluster = cluster;
    new_dir[0].is_dir = 1;
    new_dir[0].size = 0;
    new_dir[0].parent = current_dir;

    strncpy(new_dir[1].name, "..", 8);
    new_dir[1].first_cluster = current_dir->first_cluster;
    new_dir[1].is_dir = 1;
    new_dir[1].size = 0;
    new_dir[1].parent = current_dir;

    fs_save(); // Salva lo stato dopo la modifica

    printf("Directory created: %s\n", entry->name);

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

    printf("create_file: Creating file %.25s.%.3s\n", entry->name, entry->extension);

    int cluster = get_free_cluster();
    if (cluster == FAT_FULL) {
        return FILE_CREATE_ERROR;
    }

    entry->parent = current_dir;
    entry->first_cluster = cluster;
    int cluster_size = fs->cluster_size;
    int clusters_needed = (size + cluster_size - 1) / cluster_size;
    int current_cluster = cluster;

    for (int i = 0; i < clusters_needed; i++) {
        memcpy(&data_blocks[current_cluster * cluster_size], &data[i * cluster_size], cluster_size);
        int next_cluster = get_free_cluster();
        if (i == clusters_needed - 1) {
            next_cluster = FAT_END;
        }
        fat_table[current_cluster] = next_cluster;
        current_cluster = next_cluster;
    }

    fs_save(); // Salva lo stato dopo la modifica

    printf("create_file: File %.25s.%.3s created successfully\n", entry->name, entry->extension);

    return 0;
}

DirectoryEntry* locate_file(const char* name, const char* ext, char is_dir) {
    int cluster = current_dir->first_cluster;
    printf("locate_file: Searching for %s.%s in directory %s\n", name, ext, current_dir->name);
    while (cluster != FAT_END) {
        DirectoryEntry* dir = (DirectoryEntry*)&data_blocks[cluster * fs->cluster_size];
        for (int i = 0; i < fs->cluster_size / sizeof(DirectoryEntry); i++) {
            DirectoryEntry* entry = &dir[i];
            printf("locate_file: Checking entry %.25s.%.3s\n", entry->name, entry->extension);
            if (strncmp(entry->name, name, 24) == 0 && strncmp(entry->extension, ext, 3) == 0 && entry->is_dir == is_dir) {
                printf("locate_file: Found %.25s.%.3s\n", name, ext);
                return entry;
            }
        }
        cluster = fat_table[cluster];
    }
    printf("locate_file: %s.%s not found\n", name, ext);
    return NULL;
}

int is_directory_empty(DirectoryEntry* dir) {
    int cluster = dir->first_cluster;
    while (cluster != FAT_END) {
        DirectoryEntry* d = (DirectoryEntry*)&data_blocks[cluster * fs->cluster_size];
        for (int i = 0; i < fs->cluster_size / sizeof(DirectoryEntry); i++) {
            DirectoryEntry* entry = &d[i];
            if (entry->name[0] != 0x00 && (unsigned char)entry->name[0] != DELETED_ENTRY && strcmp(entry->name, ".") != 0 && strcmp(entry->name, "..") != 0) {
                return 0; // Non vuota
            }
        }
        cluster = fat_table[cluster];
    }
    return 1; // Vuota
}

int remove_file(const char* name, const char* ext) {
    printf("Attempting to remove file: %s.%s\n", name, ext);
    DirectoryEntry* file = locate_file(name, ext, 0);
    if (file == NULL) {
        printf("File not found: %s.%s\n", name, ext);
        return FILE_NOT_FOUND;
    }

    printf("Removing file: %s.%s\n", name, ext);
    int current_cluster = file->first_cluster;
    while (current_cluster != FAT_END) {
        memset(&data_blocks[current_cluster * fs->cluster_size], 0x00, fs->cluster_size);
        int next_cluster = fat_table[current_cluster];
        fat_table[current_cluster] = FAT_UNUSED;
        current_cluster = next_cluster;
    }

    file->name[0] = DELETED_ENTRY;

    fs_save(); // Salva lo stato dopo la modifica

    printf("File removed: %s.%s\n", name, ext);
    return 0;
}

int remove_empty_dir(DirectoryEntry* dir) {
    int current_cluster = dir->first_cluster;
    while (current_cluster != FAT_END) {
        memset(&data_blocks[current_cluster * fs->cluster_size], 0x00, fs->cluster_size);
        int next_cluster = fat_table[current_cluster];
        fat_table[current_cluster] = FAT_UNUSED;
        current_cluster = next_cluster;
    }
    dir->name[0] = DELETED_ENTRY;

    fs_save(); // Salva lo stato dopo la modifica

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

        int cluster = dir->first_cluster;
        while (cluster != FAT_END) {
            DirectoryEntry* d = (DirectoryEntry*)&data_blocks[cluster * fs->cluster_size];
            for (int i = 0; i < fs->cluster_size / sizeof(DirectoryEntry); i++) {
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
            cluster = fat_table[cluster];
        }

        current_dir = temp;
        printf("Directory removed: %s\n", name);
        return remove_empty_dir(dir);
    } else {
        printf("Directory not empty and recursive flag not set: %s\n", name);
        return DIR_NOT_EMPTY;
    }
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
    DirectoryEntry* file = handle->file_entry;
    if (file == NULL) {
        return FILE_READ_ERROR;
    }

    int current_cluster = file->first_cluster;
    int cluster_size = fs->cluster_size;
    int clusters_to_skip = handle->position / cluster_size;
    int byte_offset = handle->position % cluster_size;

    for (int i = 0; i < clusters_to_skip; i++) {
        current_cluster = fat_table[current_cluster];
        if (current_cluster == FAT_END) {
            return FILE_READ_ERROR;
        }
    }

    int bytes_read = 0;
    int file_size = file->size - handle->position;

    while (current_cluster != FAT_END && file_size > 0 && size > 0) {
        int bytes_to_read = (file_size > cluster_size - byte_offset) ? cluster_size - byte_offset : file_size;
        bytes_to_read = (bytes_to_read > size) ? size : bytes_to_read;

        memcpy(buffer + bytes_read, &data_blocks[current_cluster * cluster_size + byte_offset], bytes_to_read);
        bytes_read += bytes_to_read;
        file_size -= bytes_to_read;
        size -= bytes_to_read;
        byte_offset = 0;

        current_cluster = fat_table[current_cluster];
    }

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

    int current_cluster = file->first_cluster;
    int cluster_size = fs->cluster_size;
    int clusters_to_skip = offset / cluster_size;
    int byte_offset = offset % cluster_size;

    printf("write_file_content: Writing to %s.%s at offset %d, size %d\n", name, ext, offset, size);
    printf("write_file_content: clusters_to_skip %d, byte_offset %d\n", clusters_to_skip, byte_offset);

    for (int i = 0; i < clusters_to_skip; i++) {
        current_cluster = fat_table[current_cluster];
        if (current_cluster == FAT_END) {
            return FILE_WRITE_ERROR; // Trying to write beyond the end of the file
        }
    }

    int bytes_written = 0;
    while (size > 0 && current_cluster != FAT_END) {
        int bytes_to_write = (size + byte_offset > cluster_size) ? cluster_size - byte_offset : size;
        printf("write_file_content: Writing %d bytes at cluster %d, byte offset %d\n", bytes_to_write, current_cluster, byte_offset);
        memcpy(&data_blocks[current_cluster * cluster_size + byte_offset], data + bytes_written, bytes_to_write);

        size -= bytes_to_write;
        bytes_written += bytes_to_write;
        byte_offset = 0;

        if (size > 0) {
            int next_cluster = fat_table[current_cluster];
            if (next_cluster == FAT_END) {
                next_cluster = get_free_cluster();
                if (next_cluster == FAT_FULL) {
                    return FILE_WRITE_ERROR; // No more space left in the file system
                }
                fat_table[current_cluster] = next_cluster;
                fat_table[next_cluster] = FAT_END;
            }
            current_cluster = next_cluster;
        }
    }

    file->size = offset + bytes_written > file->size ? offset + bytes_written : file->size;

    fs_save(); // Salva lo stato dopo la modifica

    printf("write_file_content: Total bytes written %d\n", bytes_written);

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
        return -1; // Invalid origin
    }

    if (new_position < 0 || new_position > handle->file_entry->size) {
        return -1; // Out of bounds
    }

    handle->position = new_position;
    return 0; // Success
}
