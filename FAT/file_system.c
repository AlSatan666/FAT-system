#include "file_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void createFile(FileSystem* fs, const char* filename) {
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
