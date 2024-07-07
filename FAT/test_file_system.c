#include "file_system.h"
#include <stdio.h>

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: ./myfs <path-to-fat-file> <path-to-data-file>\n");
        return -1;
    }

    char* fat_path = argv[1];
    char* data_path = argv[2];
    int res = fs_initialize(fat_path, data_path);
    if (res) {
        printf("Error initializing filesystem\n");
        return res;
    }

    res = create_dir("TEST");
    if (res) {
        printf("Error creating directory 'TEST'\n");
        goto cleanup;
    }

    ls();

    res = cd("TEST");
    if (res) {
        printf("Error changing to directory 'TEST'\n");
        goto cleanup;
    }
    char file_content[12] = "helloworld1";

    res = create_file("HELLO", "TXT", 12, file_content);
    if (res) {
        printf("Error creating file 'HELLO.TXT'\n");
        goto cleanup;
    }

    ls();

    res = create_file("HELLO2", "IMG", 12, file_content);
    if (res) {
        printf("Error creating file 'HELLO2.IMG'\n");
        goto cleanup;
    }

    res = create_dir("DELME");
    if (res) {
        printf("Error creating directory 'DELME'\n");
        goto cleanup;
    }

    res = cd("DELME");
    if (res) {
        printf("Error changing to directory 'DELME'\n");
        goto cleanup;
    }

    res = create_file("HELLO3", "IMG", 12, file_content);
    if (res) {
        printf("Error creating file 'HELLO3.IMG'\n");
        goto cleanup;
    }
    res = create_file("HELLO4", "IMG", 12, file_content);
    if (res) {
        printf("Error creating file 'HELLO4.IMG'\n");
        goto cleanup;
    }

    ls();

    res = cd("..");
    if (res) {
        printf("Error changing to parent directory\n");
        goto cleanup;
    }

    ls();

    res = cd("..");
    if (res) {
        printf("Error changing to root directory\n");
        goto cleanup;
    }

    res = remove_dir("TEST", 1);
    if (res) {
        printf("Error erasing directory 'TEST'\n");
        goto cleanup;
    }

    ls();

cleanup:
    if (res == DIR_CREATE_ERROR) {
        printf("Cannot create new directory in %s\n", get_current_dir()->name);
        return -1;
    }
    if (res == FILE_CREATE_ERROR) {
        printf("Cannot create file in directory %s \n", get_current_dir()->name);
        return -1;
    }
    if (res == FILE_NOT_FOUND) {
        printf("File/Directory not found in directory %s \n", get_current_dir()->name);
    }
    if (res == INIT_ERROR) {
        printf("Initialize error\n");
        return -1;
    }

    return 0;
}
