#include <stdio.h>
#include <string.h>
#include "file_system.h"

#define DATATICUS_FILE "DATATICUS.dat"

// Dichiarazioni delle funzioni di test
void test_fs_initialize();
void test_fs_load();
void test_fs_save();
void test_create_dir();
void test_create_file();
void test_cd();
void test_cd_root();
void test_ls();
void test_remove_file();
void test_remove_dir();
void test_write_file();
void test_read_file(const char* name, const char* ext);
void test_seek_file();

int main() {
    test_fs_load();

    test_create_dir();
    test_create_file();

    test_cd();
    test_ls();
    test_cd_root();
    test_ls();

    test_remove_file();
    test_remove_dir();

    test_write_file();
    test_read_file("TEST", "TXT");
    test_seek_file();

    test_fs_save();

    return 0;
}
