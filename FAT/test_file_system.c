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
    char additional_content[] = "spazio sprecato";
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

    FileHandle file_handle;
    file_handle.file_entry = locate_file(name, ext, 0);
    if (file_handle.file_entry == NULL) {
        printf("File '%s.%s' not found.\n", name, ext);
        return;
    }
    file_handle.position = 0;

    char buffer[50];
    int bytes_read = read_file_content(&file_handle, buffer, sizeof(buffer) - 1);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        printf("Read from '%s.%s': %s\n", name, ext, buffer);
    } else {
        printf("Error reading file '%s.%s'.\n", name, ext);
    }
}

void test_seek_file() {
    printf("\nRunning test: seek_file\n");

    FileHandle file_handle;
    file_handle.file_entry = locate_file("TEST", "TXT", 0);
    if (file_handle.file_entry == NULL) {
        printf("File 'TEST.TXT' not found.\n");
        return;
    }
    file_handle.position = 0;


    int res = seek_file(&file_handle, 5, SEEK_SET);
    if (res == 0) {
        printf("Seek to position 5 in 'TEST.TXT' successful. Current position: %d\n", file_handle.position);
    } else {
        printf("Seek to position 5 in 'TEST.TXT' failed.\n");
    }

    
    char buffer[40];
    memset(buffer, 0, sizeof(buffer));
    int bytes_read = read_file_content(&file_handle, buffer, sizeof(buffer));
    buffer[bytes_read] = '\0';
    printf("Reading from position %d in 'TEST.TXT': %s\n", file_handle.position, buffer);


    res = seek_file(&file_handle, 3, SEEK_CUR);
    if (res == 0) {
        printf("Seek forward by 3 in 'TEST.TXT' successful. Current position: %d\n", file_handle.position);
    } else {
        printf("Seek forward by 3 in 'TEST.TXT' failed.\n");
    }


    memset(buffer, 0, sizeof(buffer));
    bytes_read = read_file_content(&file_handle, buffer, sizeof(buffer));
    buffer[bytes_read] = '\0';
    printf("Reading from position %d in 'TEST.TXT': %s\n", file_handle.position, buffer);

   
    res = seek_file(&file_handle, -2, SEEK_CUR);
    if (res == 0) {
        printf("Seek backward by 2 in 'TEST.TXT' successful. Current position: %d\n", file_handle.position);
    } else {
        printf("Seek backward by 2 in 'TEST.TXT' failed.\n");
    }

    
    memset(buffer, 0, sizeof(buffer));
    bytes_read = read_file_content(&file_handle, buffer, sizeof(buffer));
    buffer[bytes_read] = '\0';
    printf("Reading from position %d in 'TEST.TXT': %s\n", file_handle.position, buffer);


    res = seek_file(&file_handle, 0, SEEK_END);
    if (res == 0) {
        printf("Seek to end of 'TEST.TXT' successful. Current position: %d\n", file_handle.position);
    } else {
        printf("Seek to end of 'TEST.TXT' failed.\n");
    }
    
    memset(buffer, 0, sizeof(buffer));
    bytes_read = read_file_content(&file_handle, buffer, sizeof(buffer));
    buffer[bytes_read] = '\0';
    printf("Reading from position %d in 'TEST.TXT': %s\n", file_handle.position, buffer);
}



