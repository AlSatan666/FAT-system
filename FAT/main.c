#include <stdio.h>
#include "file_system.h"

// Dichiarazioni delle funzioni di test
void test_fs_initialize();
void test_create_dir();
void test_cd();
void test_ls();
void test_create_file();
void test_remove_file();
void test_cd_root();
void test_remove_dir();
void test_write_file();
void test_read_file(const char* name, const char* ext);
void test_seek_file();

int main(int argc, char** argv) {
    test_fs_initialize();
    test_create_dir();
    test_cd();
    test_ls(); 
    test_create_file();
    test_ls();
    test_read_file("TEST", "TXT");
    test_write_file();
    test_read_file("TEST", "TXT");
    test_seek_file();
    test_cd_root();
    test_ls(); 
    test_remove_dir();
    return 0;
}



}

