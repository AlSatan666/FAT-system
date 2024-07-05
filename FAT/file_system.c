#include "file_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void createFile(FileSystem* fs, const char* filename) {
    printf("Attempting to create file: %s\n", filename);
    DirectoryEntry* entries = fs->current_directory->entries;
    int num_entries = fs->current_directory->num_entries;

    for (int i = 0; i < num_entries; ++i) {
        if (strcmp(entries[i].name, filename) == 0) {
            printf("\nFile already exists.\n");
            return;
        }
    }

    int position = -1;
    if (fs->num_free_positions > 0) {
        // Usa uno degli spazi liberi
        position = fs->free_positions[--fs->num_free_positions];
    } else if (fs->next_free_position + MAX_FILE_SIZE <= fs->buffer_size) {
        // Usa la prossima posizione libera
        position = fs->next_free_position;
        fs->next_free_position += MAX_FILE_SIZE;
    } else {
        printf("\nNo free space available.\n");
        return;
    }

    DirectoryEntry new_entry;
    strcpy(new_entry.name, filename);
    new_entry.position = position;
    new_entry.is_dir = 0;
    new_entry.subdir = NULL;

    entries = realloc(entries, (num_entries + 1) * sizeof(DirectoryEntry));
    if (entries == NULL) {
        printf("\nMemory allocation failed.\n");
        return;
    }
    entries[num_entries] = new_entry;

    fs->current_directory->entries = entries;
    fs->current_directory->num_entries++;
    printf("Successfully created file: %s\n", filename);
}
  

void eraseFile(FileSystem* fs, const char* filename) {
    printf("Attempting to erase file: %s\n", filename);
    DirectoryEntry* entries = fs->current_directory->entries;
    int num_entries = fs->current_directory->num_entries;

    for (int i = 0; i < num_entries; ++i) {
        if (strcmp(entries[i].name, filename) == 0 && !entries[i].is_dir) {
            memset(fs->buffer + entries[i].position, 0, MAX_FILE_SIZE);

            fs->free_positions = realloc(fs->free_positions, (fs->num_free_positions + 1) * sizeof(int));
            fs->free_positions[fs->num_free_positions++] = entries[i].position;

            memmove(&entries[i], &entries[i + 1], (num_entries - i - 1) * sizeof(DirectoryEntry));
            entries = realloc(entries, (num_entries - 1) * sizeof(DirectoryEntry));
            if (entries == NULL && num_entries > 1) {
                printf("\nMemory reallocation failed.\n");
                return;
            }

            fs->current_directory->entries = entries;
            fs->current_directory->num_entries--;
            printf("Successfully erased file: %s\n", filename);
            return;
        }
    }

    printf("\nFile not found.\n");
}

void write(FileSystem* fs, const char* filename, const char* data) {
    printf("Attempting to write to file: %s\n", filename);
    DirectoryEntry* entries = fs->current_directory->entries;
    int num_entries = fs->current_directory->num_entries;

    for (int i = 0; i < num_entries; ++i) {
        if (strcmp(entries[i].name, filename) == 0 && !entries[i].is_dir) {
            int data_length = strlen(data);
            if (data_length > MAX_FILE_SIZE) {
                data_length = MAX_FILE_SIZE;
            }
            strncpy(fs->buffer + entries[i].position, data, data_length);
            printf("Successfully wrote to file: %s\n", filename);
            return;
        }
    }

    printf("\nFile not found.\n");
}

void read(FileSystem* fs, const char* filename, char* data, int size) {
    printf("Attempting to read from file: %s\n", filename);
    DirectoryEntry* entries = fs->current_directory->entries;
    int num_entries = fs->current_directory->num_entries;

    for (int i = 0; i < num_entries; ++i) {
        if (strcmp(entries[i].name, filename) == 0 && !entries[i].is_dir) {
            strncpy(data, fs->buffer + entries[i].position, size);
            printf("Successfully read from file: %s\n", filename);
            return;
        }
    }

    printf("\nFile not found.\n");
}
 

void seek(FileSystem* fs, const char* filename) {
    printf("Attempting to seek file: %s\n", filename);
    DirectoryEntry* entries = fs->current_directory->entries;
    int num_entries = fs->current_directory->num_entries;

    for (int i = 0; i < num_entries; ++i) {
        if (strcmp(entries[i].name, filename) == 0 && !entries[i].is_dir) {
            printf("The position of file '%s' is %d.\n", filename, entries[i].position);
            return;
        }
    }

    printf("\nFile not found.\n");
}

void createDir(FileSystem* fs, const char* dirname) {
    printf("Attempting to create directory: %s\n", dirname);
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
    if (new_entry.subdir == NULL) {
        printf("\nMemory allocation failed.\n");
        return;
    }
    new_entry.subdir->entries = NULL;
    new_entry.subdir->num_entries = 0;
    new_entry.subdir->parent = fs->current_directory;

    entries = realloc(entries, (num_entries + 1) * sizeof(DirectoryEntry));
    if (entries == NULL) {
        printf("\nMemory allocation failed.\n");
        free(new_entry.subdir);
        return;
    }
    entries[num_entries] = new_entry;

    fs->current_directory->entries = entries;
    fs->current_directory->num_entries++;
    printf("Successfully created directory: %s\n", dirname);
}

void eraseDirRecursive(Directory* dir) {
    for (int i = 0; i < dir->num_entries; ++i) {
        if (dir->entries[i].is_dir) {
            eraseDirRecursive(dir->entries[i].subdir);
            free(dir->entries[i].subdir);
        }
    }
    free(dir->entries);
}

void eraseDir(FileSystem* fs, const char* dirname) {
    printf("Attempting to erase directory: %s\n", dirname);
    DirectoryEntry* entries = fs->current_directory->entries;
    int num_entries = fs->current_directory->num_entries;

    for (int i = 0; i < num_entries; ++i) {
        if (strcmp(entries[i].name, dirname) == 0 && entries[i].is_dir) {
            if (entries[i].subdir->num_entries > 0) {
                char response[4];//(usa 4 array per memorizzare il yes/n )
                printf("\nDirectory is not empty. Do you want to delete and all its files? (yes/no): ");
                fgets(response, sizeof(response), stdin);
                response[strcspn(response, "\n")] = 0;

                if (strcmp(response, "yes") == 0) {
                    eraseDirRecursive(entries[i].subdir);
                } else {
                    printf("\nOperation cancelled.\n");
                    return;
                }
            }

            free(entries[i].subdir);

            memmove(&entries[i], &entries[i + 1], (num_entries - i - 1) * sizeof(DirectoryEntry));
            entries = realloc(entries, (num_entries - 1) * sizeof(DirectoryEntry));
            if (entries == NULL && num_entries > 1) {
                printf("\nMemory reallocation failed.\n");
                return;
            }

            fs->current_directory->entries = entries;
            fs->current_directory->num_entries--;
            printf("Successfully erase directory: %s\n", dirname);
            return;
        }
    }

    printf("\nDirectory not found.\n");
}

void changeDir(FileSystem* fs, const char* dirname) {
    printf("Attempting to change directory to: %s\n", dirname);
    printf("To return to the previous directory, use 'changeDir ..'\n");

    if (strcmp(dirname, "..") == 0) {
        if (fs->current_directory->parent) {
            fs->current_directory = fs->current_directory->parent;
            printf("Successfully changed to the parent directory.\n");
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
            printf("Successfully changed directory to: %s\n", dirname);
            return;
        }
    }

    printf("\nDirectory not found.\n");
}

void listDir(FileSystem* fs) {
    printf("Attempting to list directories\n");
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
    printf("Successfully listed directories\n");
}

void listFiles(FileSystem* fs) {
    printf("Attempting to list files\n");
    DirectoryEntry* entries = fs->current_directory->entries;
    int num_entries = fs->current_directory->num_entries;

    printf("Files:\n");
    for (int i = 0; i < num_entries; ++i) {
        if (!entries[i].is_dir) {
            printf("- %s\n", entries[i].name);
        }
    }
    printf("Successfully listed files\n");
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
    printf("Current Directory: %s\n", path);
}

