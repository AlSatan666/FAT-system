#include "file_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Variabili globali
FileSystem *fs;
DirectoryEntry *current_dir;
int *fat_table;
char *data_blocks;
FILE *fat_file;
FILE *data_file;

int fs_initialize(const char* fat_path, const char* data_path) {
    fat_file = fopen(fat_path, "wb+");
    data_file = fopen(data_path, "wb+");

    if (!fat_file || !data_file) {
        printf("Error creating or opening FAT/data files\n");
        return INIT_ERROR;
    }

    fs = (FileSystem*)malloc(sizeof(FileSystem));
    fs->bytes_per_block = BLOCK_SIZE;
    fs->total_blocks = TOTAL_BLOCKS;
    fs->cluster_size = BLOCK_SIZE * BLOCKS_PER_CLUSTER;
    fs->fat_entries = (fs->total_blocks) * fs->bytes_per_block / fs->cluster_size;
    fs->fat_size = fs->fat_entries * 4;
    fs->data_size = (fs->total_blocks - (fs->fat_size / fs->bytes_per_block)) * fs->bytes_per_block - sizeof(FileSystem);

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

    fwrite(fs, sizeof(FileSystem), 1, fat_file);
    fwrite(fat_table, fs->fat_size, 1, fat_file);
    fwrite(data_blocks, fs->data_size, 1, data_file);

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
    int cluster = current_dir->first_cluster;
    while (cluster != FAT_END) {
        DirectoryEntry* dir = (DirectoryEntry*)&data_blocks[cluster * fs->cluster_size];
        for (int i = 0; i < fs->cluster_size / sizeof(DirectoryEntry); i++) {
            DirectoryEntry* entry = &dir[i];
            if (strcmp(entry->name, dir_name) == 0) {
                if (entry->is_dir) {
                    printf("Found directory: %s\n", dir_name);
                    if (strcmp(dir_name, "..") == 0) {
                        current_dir = current_dir->parent;
                    } else {
                        current_dir = (DirectoryEntry*)&data_blocks[entry->first_cluster * fs->cluster_size];
                        current_dir->parent = dir;
                    }
                    return 0;
                } else {
                    printf("%s is not a directory\n", dir_name);
                    return FILE_NOT_FOUND;
                }
            }
        }
        cluster = fat_table[cluster];
    }
    printf("Directory %s not found\n", dir_name);
    return FILE_NOT_FOUND;
}

void ls() {
    int cluster = current_dir->first_cluster;
    printf("Contents of directory (%s):\n", current_dir->name);
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
                printf("%.8s/\t", entry->name);
            } else {
                printf("%.8s.%.3s\t", entry->name, entry->extension);
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

    strncpy(entry->name, name, 8);
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

    fseek(data_file, 0, SEEK_SET);
    fwrite(data_blocks, fs->data_size, 1, data_file);
    fseek(fat_file, sizeof(FileSystem), SEEK_SET);
    fwrite(fat_table, fs->fat_size, 1, fat_file);

    return 0;
}

int create_file(const char* name, const char* ext, int size, const char* data) {
    DirectoryEntry* entry = find_empty_dir_entry();
    if (entry == NULL) {
        return FILE_CREATE_ERROR;
    }

    memset(entry->name, ' ', sizeof(entry->name));
    memset(entry->extension, ' ', sizeof(entry->extension));

    strncpy(entry->name, name, 8);
    entry->name[7] = '\0';
    strncpy(entry->extension, ext, 3);
    entry->extension[3] = '\0';
    entry->size = size;
    entry->is_dir = 0;

    printf("create_file: Creating file %.8s.%.3s\n", entry->name, entry->extension);

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

    fseek(data_file, 0, SEEK_SET);
    fwrite(data_blocks, fs->data_size, 1, data_file);
    fseek(fat_file, sizeof(FileSystem), SEEK_SET);
    fwrite(fat_table, fs->fat_size, 1, fat_file);

    printf("create_file: File %.8s.%.3s created successfully\n", entry->name, entry->extension);

    return 0;
}

DirectoryEntry* locate_file(const char* name, const char* ext, char is_dir) {
    int cluster = current_dir->first_cluster;
    printf("locate_file: Searching for %s.%s in directory %s\n", name, ext, current_dir->name);
    while (cluster != FAT_END) {
        DirectoryEntry* dir = (DirectoryEntry*)&data_blocks[cluster * fs->cluster_size];
        for (int i = 0; i < fs->cluster_size / sizeof(DirectoryEntry); i++) {
            DirectoryEntry* entry = &dir[i];
            printf("locate_file: Checking entry %.8s.%.3s\n", entry->name, entry->extension);
            if (strncmp(entry->name, name, 8) == 0 && strncmp(entry->extension, ext, 3) == 0 && entry->is_dir == is_dir) {
                printf("locate_file: Found %.8s.%.3s\n", name, ext);
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

    fseek(data_file, 0, SEEK_SET);
    fwrite(data_blocks, fs->data_size, 1, data_file);
    fseek(fat_file, sizeof(FileSystem), SEEK_SET);
    fwrite(fat_table, fs->fat_size, 1, fat_file);

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

    fseek(data_file, 0, SEEK_SET);
    fwrite(data_blocks, fs->data_size, 1, data_file);
    fseek(fat_file, sizeof(FileSystem), SEEK_SET);
    fwrite(fat_table, fs->fat_size, 1, fat_file);

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

int read_file_content(const char* name, const char* ext, char* buffer) {
    DirectoryEntry* file = locate_file(name, ext, 0);
    if (file == NULL) {
        return FILE_READ_ERROR;
    }

    int current_cluster = file->first_cluster;
    int bytes_read = 0;
    int cluster_size = fs->cluster_size;
    int file_size = file->size;
    while (current_cluster != FAT_END && file_size > 0) {
        int bytes_to_read = (file_size > cluster_size) ? cluster_size : file_size;
        memcpy(buffer + bytes_read, &data_blocks[current_cluster * cluster_size], bytes_to_read);
        bytes_read += bytes_to_read;
        file_size -= bytes_to_read;
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

    fseek(data_file, 0, SEEK_SET);
    fwrite(data_blocks, fs->data_size, 1, data_file);
    fseek(fat_file, sizeof(FileSystem), SEEK_SET);
    fwrite(fat_table, fs->fat_size, 1, fat_file);

    printf("write_file_content: Total bytes written %d\n", bytes_written);

    return bytes_written;
}

