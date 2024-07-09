#include <stdint.h>
#include <stdbool.h>

#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

// Dimensione di un blocco in byte
#define BLOCK_SIZE 512
// Numero di blocchi per cluster
#define BLOCKS_PER_CLUSTER 1
// Numero totale di blocchi nel file system
#define TOTAL_BLOCKS 65536

// Definizione di valori per la FAT
#define FAT_UNUSED 0x00000000   // Blocco non usato
#define FAT_END 0x0FFFFFF8      // Fine della catena di blocchi
#define FAT_OCCUPIED 0xFFFFFFFF // Blocco occupato

// Dimensione di una voce di directory in byte
#define DIR_ENTRY_SIZE 32
// Valore che indica una voce di directory cancellata
#define DELETED_ENTRY 0xE5

// Codici di errore
#define DIR_CREATE_ERROR -1
#define FILE_CREATE_ERROR -2
#define INIT_ERROR -3
#define FILE_NOT_FOUND -4
#define FILE_READ_ERROR -5
#define DIR_NOT_EMPTY -6
#define FAT_FULL -7
#define FILE_WRITE_ERROR -8

// Struttura che rappresenta il file system
typedef struct {
    int bytes_per_block;     
    int fat_entries;         // Numero di voci nella FAT
    int cluster_size;        
    int fat_size;            
    int data_size;           // Dimensione dell'area dati
    int total_blocks;        
    char current_directory[25]; 
} FileSystem;

// Struttura che rappresenta una voce di directory
typedef struct DirectoryEntry {
    char name[25];               
    char extension[3];           
    char is_dir;                 // Flag che indica se è una directory
    struct DirectoryEntry* parent; // Puntatore alla directory padre
    int first_cluster;           
    int size;                    
    int entry_count;             // Numero di voci nella directory (se directory)
} __attribute__((packed)) DirectoryEntry;

// Struttura che rappresenta un handle per un file aperto
typedef struct FileHandle {
    DirectoryEntry* file_entry; // Puntatore alla voce di directory del file
    int position;               // Posizione corrente nel file
} FileHandle;

// Funzioni di gestione del file system
int fs_initialize(const char* fat_path, const char* data_path);
DirectoryEntry* get_current_dir();
FileSystem* get_fs();
int get_free_cluster();
DirectoryEntry* find_empty_dir_entry();
int cd(const char* dir_name);
void ls();
int create_dir(const char* name);
int create_file(const char* name, const char* ext, int size, const char* data);
DirectoryEntry* locate_file(const char* name, const char* ext, char is_dir);
int remove_file(const char* name, const char* ext);
int remove_empty_dir(DirectoryEntry* dir);
bool is_dir_empty(DirectoryEntry* dir);
int remove_dir(const char* name, int recursive);
void display_fs_image(unsigned int max_bytes);
int read_file_content(FileHandle *handle, char *buffer, int size);
int write_file_content(const char* name, const char* ext, const char* data, int offset, int size);
int seek_file(FileHandle *handle, int offset, int origin);

#endif

