#include "file_system.h"
#include <stdio.h>
#include <string.h>

void test_create_file();
void test_erase_file();
void test_write_read_file();
void test_seek_file();
void test_create_dir();
void test_erase_dir();
void test_change_dir();
void test_list_dir();

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

    test_create_file();
    test_erase_file();
    test_write_read_file();
    test_seek_file();
    test_create_dir();
    test_erase_dir();
    test_change_dir();
    test_list_dir();

    return 0;
}

void test_create_file() {
    printf("\nRunning test: create_file\n");
    char file_content[12] = "helloworld1";
    int res = create_file("TEST", "TXT", 12, file_content);
    if (res == 0) {
        printf("File 'TEST.TXT' created successfully.\n");
    } else {
        printf("Error creating file 'TEST.TXT'.\n");
    }
}

void test_erase_file() {
    printf("\nRunning test: erase_file\n");
    int res = remove_file("TEST", "TXT");
    if (res == 0) {
        printf("File 'TEST.TXT' erased successfully.\n");
    } else {
        printf("Error erasing file 'TESTFILE.TXT'.\n");
    }
}

void test_write_read_file() {
    printf("\nRunning test: write_read_file\n");
    char file_content[20] = "Hello FAT World!";
    int res = create_file("WRITE", "TXT", 17, file_content);
    if (res == 0) {
        printf("File 'WRITE.TXT' created successfully.\n");
    } else {
        printf("Error creating file 'WRITEFILE.TXT'.\n");
    }

    char buffer[20];
    int bytes_read = read_file_content("WRITEFILE", "TXT", buffer);
    if (bytes_read == 17) {
        buffer[bytes_read] = '\0';
        printf("Read from 'WRITEFILE.TXT': %s\n", buffer);
    } else {
        printf("Error reading file 'WRITEFILE.TXT'.\n");
    }
}

void test_seek_file() {
    printf("\nRunning test: seek_file\n");
    FileHandle file_handle;
    file_handle.file_entry = locate_file("WRITEFILE", "TXT", 0);
    if (file_handle.file_entry == NULL) {
        printf("File 'WRITEFILE.TXT' not found.\n");
        return;
    }
    file_handle.position = 5;

    char buffer[20];
    memset(buffer, 0, sizeof(buffer));
    int bytes_read = read_file_content(file_handle.file_entry->name, file_handle.file_entry->extension, buffer);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        printf("Read from position %d in 'WRITE.TXT': %s\n", file_handle.position, buffer + file_handle.position);
    } else {
        printf("Error seeking in file 'WRITE.TXT'.\n");
    }
}


void test_create_dir() {
    printf("\nRunning test: create_dir\n");
    int res = create_dir("TESTDIR");
    if (res == 0) {
        printf("Directory 'TESTDIR' created successfully.\n");
    } else {
        printf("Error creating directory 'TESTDIR'.\n");
    }
}

void test_erase_dir() {
    printf("\nRunning test: erase_dir\n");
    int res = remove_dir("TESTDIR", 0);
    if (res == 0) {
        printf("Directory 'TESTDIR' erased successfully.\n");
    } else {
        printf("Error erasing directory 'TESTDIR'.\n");
    }
}

void test_change_dir() {
    printf("\nRunning test: change_dir\n");
    int res = create_dir("DIRCHANGE");
    if (res == 0) {
        printf("Directory 'DIRCHANGE' created successfully.\n");
    } else {
        printf("Error creating directory 'DIRCHANGE'.\n");
    }

    res = cd("DIRCHANGE");
    if (res == 0) {
        printf("Changed to directory 'DIRCHANGE' successfully.\n");
    } else {
        printf("Error changing to directory 'DIRCHANGE'.\n");
    }

    res = cd("..");
    if (res == 0) {
        printf("Changed to parent directory successfully.\n");
    } else {
        printf("Error changing to parent directory.\n");
    }
}

void test_list_dir() {
    printf("\nRunning test: list_dir\n");
    printf("Contents of root directory:\n");
    ls();
}


