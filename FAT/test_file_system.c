#include <stdio.h>
#include <string.h>
#include "file_system.h"

#define FAT_FILE "test_fat_file.dat"
#define DATA_FILE "test_data_file.dat"


void test_fs_initialize() {
    int res = fs_initialize(FAT_FILE, DATA_FILE);
    if (res == 0) {
        printf("fs_initialize: PASSED\n");
    } else {
        printf("fs_initialize: FAILED\n");
    }
}

void test_create_dir() {
    int res = create_dir("TESTDIR");
    if (res == 0) {
        printf("create_dir: PASSED\n");
    } else {
        printf("create_dir: FAILED\n");
    }
}

void test_create_file() {
    char content[] = "Hello, World!";
    int res = create_file("TEST", "TXT", strlen(content), content);
    if (res == 0) {
        printf("create_file: PASSED\n");
    } else {
        printf("create_file: FAILED\n");
    }
}

void test_cd() {
    int res = cd("TESTDIR");
    if (res == 0) {
        printf("cd: PASSED\n");
    } else {
        printf("cd: FAILED\n");
    }
}

void test_cd_root() {
    int res = cd("..");
    if (res == 0) {
        printf("cd to root: PASSED\n");
    } else {
        printf("cd to root: FAILED\n");
    }
}

void test_ls() {
    printf("ls: ");
    ls();
}

void test_remove_file() {
    int res = remove_file("TEST", "TXT");
    if (res == 0) {
        printf("remove_file: PASSED\n");
    } else {
        printf("remove_file: FAILED\n");
    }
}

void test_remove_dir() {
    int res = remove_dir("TESTDIR", 1);
    if (res == 0) {
        printf("remove_dir: PASSED\n");
    } else {
        printf("remove_dir: FAILED\n");
    }
}

void test_write_file() {
    printf("\nRunning test: write_file\n");

   
    char additional_content[20] = "spazio sprecato";
    printf("test_write_file: Writing 'spazio sprecato' to 'TEST.TXT' at the end\n");
    int bytes_written = write_file_content("TEST", "TXT", additional_content, -1, 15);
    if (bytes_written == 15) {
        printf("Written to 'TEST.TXT' successfully.\n");
    } else {
        printf("Error writing to 'TEST.TXT'.\n");
    }
}

void test_read_file(const char* name, const char* ext) {
    printf("\nRunning test: read_file\n");
    char buffer[50];
    int bytes_read = read_file_content(name, ext, buffer);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        printf("Read from '%s.%s': %s\n", name, ext, buffer);
    } else {
        printf("Error reading file '%s.%s'.\n", name, ext);
    }
}

int main(int argc, char** argv) {
    test_fs_initialize();
    test_create_dir();
    test_create_file();
    test_read_file("TEST", "TXT");
    test_write_file();
    test_read_file("TEST", "TXT");
    test_ls();
    test_cd();
    test_ls();
    test_cd_root();
    test_remove_file();
    test_remove_dir();
    return 0;
}

