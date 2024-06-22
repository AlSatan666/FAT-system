#include "file_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void createFile(FileSystem* fs, const char* filename) {
    DirectoryEntry* entries = fs->current_directory->entries;
    int num_entries = fs->current_directory->num_entries;

    for (int i = 0; i < num_entries; ++i) {
        if (strcmp(entries[i].name, filename) == 0) {
            printf("\nFile already exists.\n");
            return;
        }
    }

    int position = -1;
    for (int i = 0; i < fs->buffer_size; i += MAX_FILE_SIZE) {
        int is_free = 1;
        for (int j = 0; j < MAX_FILE_SIZE; ++j) {
            if (fs->buffer[i + j] != '\0') {
                is_free = 0;
                break;
            }
        }
        if (is_free) {
            position = i;
            break;
        }
    }

    if (position == -1) {
        printf("\nNo free space available.\n");
        return;
    }

    DirectoryEntry new_entry;
    strcpy(new_entry.name, filename);
    new_entry.position = position;
    new_entry.is_dir = 0;
    new_entry.subdir = NULL;

    entries = realloc(entries, (num_entries + 1) * sizeof(DirectoryEntry));
    entries[num_entries] = new_entry;

    fs->current_directory->entries = entries;
    fs->current_directory->num_entries++;
}

void eraseFile(FileSystem* fs, const char* filename) {
    DirectoryEntry* entries = fs->current_directory->entries;
    int num_entries = fs->current_directory->num_entries;

    for (int i = 0; i < num_entries; ++i) {
        if (strcmp(entries[i].name, filename) == 0 && !entries[i].is_dir) {
            memset(fs->buffer + entries[i].position, 0, MAX_FILE_SIZE);

            memmove(&entries[i], &entries[i + 1], (num_entries - i - 1) * sizeof(DirectoryEntry));
            entries = realloc(entries, (num_entries - 1) * sizeof(DirectoryEntry));

            fs->current_directory->entries = entries;
            fs->current_directory->num_entries--;
            return;
        }
    }
}

void write(FileSystem* fs, const char* filename, const char* data) {
    DirectoryEntry* entries = fs->current_directory->entries;
    int num_entries = fs->current_directory->num_entries;

    for (int i = 0; i < num_entries; ++i) {
        if (strcmp(entries[i].name, filename) == 0 && !entries[i].is_dir) {
            int data_length = strlen(data);
            if (data_length > MAX_FILE_SIZE) {
                data_length = MAX_FILE_SIZE;
            }
            strncpy(fs->buffer + entries[i].position, data, data_length);
            return;
        }
    }

    printf("\nFile not found.\n");
}

void read(FileSystem* fs, const char* filename, char* data, int size) {
    DirectoryEntry* entries = fs->current_directory->entries;
    int num_entries = fs->current_directory->num_entries;

    for (int i = 0; i < num_entries; ++i) {
        if (strcmp(entries[i].name, filename) == 0 && !entries[i].is_dir) {
            strncpy(data, fs->buffer + entries[i].position, size);
            return;
        }
    }

    printf("\nFile not found.\n");
}

void seek(FileSystem* fs, const char* filename, int position) {
    DirectoryEntry* entries = fs->current_directory->entries;
    int num_entries = fs->current_directory->num_entries;

    for (int i = 0; i < num_entries; ++i) {
        if (strcmp(entries[i].name, filename) == 0 && !entries[i].is_dir) {
            if (position >= 0 && position < MAX_FILE_SIZE) {
                entries[i].position = position;
            } else {
                printf("\nInvalid position.\n");
            }
            return;
        }
    }

    printf("\nFile not found.\n");
}

void createDir(FileSystem* fs, const char* dirname) {
    DirectoryEntry* entries = fs->current_directory->entries;
    int num_entries = fs->current_directory->num_entries;

    for (int i = 0; i < num_entries; ++i) {
        if (strcmp(entries[i].name, dirname) == 0) {
            printf("\nDirectory already exists.\n");
            return;
        }
    }

    DirectoryEntry new_entry;
    strcpy(new_entry.name, dirname);
    new_entry.is_dir = 1;
    new_entry.subdir = malloc(sizeof(Directory));
    new_entry.subdir->entries = NULL;
    new_entry.subdir->num_entries = 0;
    new_entry.subdir->parent = fs->current_directory;

    entries = realloc(entries, (num_entries + 1) * sizeof(DirectoryEntry));
    entries[num_entries] = new_entry;

    fs->current_directory->entries = entries;
    fs->current_directory->num_entries++;
}

void eraseDir(FileSystem* fs, const char* dirname) {
    DirectoryEntry* entries = fs->current_directory->entries;
    int num_entries = fs->current_directory->num_entries;

    for (int i = 0; i < num_entries; ++i) {
        if (strcmp(entries[i].name, dirname) == 0 && entries[i].is_dir) {
            if (entries[i].subdir->num_entries > 0) {
                printf("\nDirectory is not empty.\n");
                return;
            }

            free(entries[i].subdir);

            memmove(&entries[i], &entries[i + 1], (num_entries - i - 1) * sizeof(DirectoryEntry));
            entries = realloc(entries, (num_entries - 1) * sizeof(DirectoryEntry));

            fs->current_directory->entries = entries;
            fs->current_directory->num_entries--;
            return;
        }
    }

    printf("\nDirectory not found.\n");
}

void changeDir(FileSystem* fs, const char* dirname) {
    if (strcmp(dirname, "..") == 0) {
        if (fs->current_directory->parent) {
            fs->current_directory = fs->current_directory->parent;
        } else {
            printf("\nAlready in the root directory.\n");
        }
        return;
    }

    DirectoryEntry* entries = fs->current_directory->entries;
    int num_entries = fs->current_directory->num_entries;

    for (int i = 0; i < num_entries; ++i) {
        if (strcmp(entries[i].name, dirname) == 0 && entries[i].is_dir) {
            fs->current_directory = entries[i].subdir;
            return;
        }
    }

    printf("\nDirectory not found.\n");
}

void listDir(FileSystem* fs) {
    DirectoryEntry* entries = fs->current_directory->entries;
    int num_entries = fs->current_directory->num_entries;

    printf("Directories:\n");
    for (int i = 0; i < num_entries; ++i) {
        if (entries[i].is_dir) {
            if (entries[i].subdir == fs->current_directory) {
                printf("- %s (tu sei qui)\n", entries[i].name);
            } else {
                printf("- %s\n", entries[i].name);
            }
        }
    }
}

void listFiles(FileSystem* fs) {
    DirectoryEntry* entries = fs->current_directory->entries;
    int num_entries = fs->current_directory->num_entries;

    printf("Files:\n");
    for (int i = 0; i < num_entries; ++i) {
        if (!entries[i].is_dir) {
            printf("- %s\n", entries[i].name);
        }
    }
}

void printCurrentDir(FileSystem* fs) {
    Directory* dir = fs->current_directory;
    char path[1024] = "";
    while (dir != fs->root) {
        DirectoryEntry* parent_entries = dir->parent->entries;
        for (int i = 0; i < dir->parent->num_entries; ++i) {
            if (parent_entries[i].is_dir && parent_entries[i].subdir == dir) {
                char temp[1024];
                snprintf(temp, sizeof(temp), "/%s%s", parent_entries[i].name, path);
                strcpy(path, temp);
                break;
            }
        }
        dir = dir->parent;
    }
    printf("Current Directory: /CASA%s\n", path);
}
