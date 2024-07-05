#include "file_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void createFile(FileSystem* fs, const char* filename) {
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
}   

void write(FileSystem* fs, const char* filename, const char* data) {
}  

void read(FileSystem* fs, const char* filename, char* data, int size) {
}  

void seek(FileSystem* fs, const char* filename, int position) {
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
