#include <stdio.h>
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

    // Uninitialize file system
    uninitialize_fs();

    return 0;
}
