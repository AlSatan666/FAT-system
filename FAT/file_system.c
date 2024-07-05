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
}  

void read(FileSystem* fs, const char* filename, char* data, int size) {
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
}   

void eraseDir(FileSystem* fs, const char* dirname) {
}   

void changeDir(FileSystem* fs, const char* dirname) {
}

void listDir(FileSystem* fs) {
}
   
void listFiles(FileSystem* fs) {
}    

void printCurrentDir(FileSystem* fs) {
 }
