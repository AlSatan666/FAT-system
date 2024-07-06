#include <stdio.h>
#include <string.h>
#include "file_system.h"

int main() {
    // Initialize file system
    if (initialize_fs("filesystem.img", 1024 * 1024) != 0) {
        fprintf(stderr, "Failed to initialize file system.\n");
        return 1;
    }
    printf("File system initialized.\n");

    // Create a file
    if (create_file("testfile.txt") != 0) {
        fprintf(stderr, "Failed to create file.\n");
        return 1;
    }
    printf("File created successfully.\n");

    // Write to the file
    const char *data = "Hello, FAT file system!";
    if (write_file("testfile.txt", 0, data, strlen(data)) != 0) {
        fprintf(stderr, "Failed to write to file.\n");
        return 1;
    }
    printf("Data written to file successfully.\n");

    // Erase the file
    if (erase_file("testfile.txt") != 0) {
        fprintf(stderr, "Failed to erase file.\n");
        return 1;
    }
    printf("File erased successfully.\n");

    // Uninitialize file system
    uninitialize_fs();

    return 0;
}

