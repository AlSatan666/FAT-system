#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#define MAX_FILENAME_LENGTH 256
#define MAX_FILE_SIZE 1024

typedef struct Directory Directory;//autoreferenziale

typedef struct DirectoryEntry {
    char name[MAX_FILENAME_LENGTH];
    int position;
    int is_dir; // Flag per directory
    Directory* subdir; // Puntatore alle subdiretory
} DirectoryEntry;

struct Directory {
    DirectoryEntry* entries;
    int num_entries;
    Directory* parent; //Puntatore alle diretory
};

typedef struct {
    char* buffer;
    int buffer_size;
    Directory* root;
    Directory* current_directory;
    int next_free_position;
    int* free_positions;
    int num_free_positions;
} FileSystem;

void createFile(FileSystem* fs, const char* filename);
void eraseFile(FileSystem* fs, const char* filename);
void write(FileSystem* fs, const char* filename, const char* data);
void read(FileSystem* fs, const char* filename, char* data, int size);
void seek(FileSystem* fs, const char* filename);
void createDir(FileSystem* fs, const char* dirname);
void eraseDir(FileSystem* fs, const char* dirname);
void changeDir(FileSystem* fs, const char* dirname);
void listDir(FileSystem* fs);
void listFiles(FileSystem* fs);
void printCurrentDir(FileSystem* fs);

#endif
