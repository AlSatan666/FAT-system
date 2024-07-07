#include "file_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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

    strncpy(entry->name, name, 8);
    strncpy(entry->extension, ext, 3);
    entry->size = size;
    entry->is_dir = 0;

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

    return 0;
}

DirectoryEntry* locate_file(const char* name, const char* ext, char is_dir) {
    int cluster = current_dir->first_cluster;
    while (cluster != FAT_END) {
        DirectoryEntry* dir = (DirectoryEntry*)&data_blocks[cluster * fs->cluster_size];
        for (int i = 0; i < fs->cluster_size / sizeof(DirectoryEntry); i++) {
            DirectoryEntry* entry = &dir[i];
            if (strcmp(entry->name, name) == 0 && strcmp(entry->extension, ext) == 0 && entry->is_dir == is_dir) {
                return entry;
            }
        }
        cluster = fat_table[cluster];
    }
    return NULL;
}

int remove_file(const char* name, const char* ext) {
    DirectoryEntry* file = locate_file(name, ext, 0);
    if (file == NULL) {
        return FILE_NOT_FOUND;
    }

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

bool is_dir_empty(DirectoryEntry* dir) {
    int cluster = dir->first_cluster;
    while (cluster != FAT_END) {
        DirectoryEntry* d = (DirectoryEntry*)&data_blocks[cluster * fs->cluster_size];
        for (int i = 0; i < fs->cluster_size / sizeof(DirectoryEntry); i++) {
            DirectoryEntry* entry = &d[i];
            if (entry->name[0] != 0x00 && (unsigned char)entry->name[0] != DELETED_ENTRY && strcmp(entry->name, ".") != 0 && strcmp(entry->name, "..") != 0) {
                return false;
            }
        }
        cluster = fat_table[cluster];
    }
    return true;
}

int remove_dir(const char* name, int recursive) {
    DirectoryEntry* dir = locate_file(name, "", 1);
    if (dir == NULL) {
        return FILE_NOT_FOUND;
    }

    if (is_dir_empty(dir)) {
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
        return remove_empty_dir(dir);
    } else {
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

