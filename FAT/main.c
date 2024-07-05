#include "file_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void printMenu() {
    printf("File System Menu:\n");
    printf("1. createFile <filename>\n");
    printf("2. eraseFile <filename>\n");
    printf("3. write <filename> <data>\n");
    printf("4. read <filename>\n");
    printf("5. seek <filename> <position>\n");
    printf("6. createDir <dirname>\n");
    printf("7. eraseDir <dirname>\n");
    printf("8. changeDir <dirname>\n");
    printf("9. listDir\n");
    printf("10. listFiles\n");
    printf("11. pwd\n");
    printf("12. help\n");
    printf("13. exit\n");
}

int main() {
    // Inizzializzazione del file system
    FileSystem fs;
    fs.buffer_size = 10240; // buffer size augmented
    fs.buffer = malloc(fs.buffer_size);
    fs.root = malloc(sizeof(Directory));
    fs.root->entries = NULL;
    fs.root->num_entries = 0;
    fs.root->parent = NULL;

    // Cre la directory CASA nel quale inizio
    DirectoryEntry casa_entry;
    strcpy(casa_entry.name, "CASA");
    casa_entry.is_dir = 1;
    casa_entry.subdir = malloc(sizeof(Directory));
    casa_entry.subdir->entries = NULL;
    casa_entry.subdir->num_entries = 0;
    casa_entry.subdir->parent = fs.root;

    fs.root->entries = malloc(sizeof(DirectoryEntry));
    fs.root->entries[0] = casa_entry;
    fs.root->num_entries = 1;
    
    // Metto la directory 
    fs.current_directory = casa_entry.subdir;

    printCurrentDir(&fs);

    char command[256];
    char filename[MAX_FILENAME_LENGTH];
    char data[MAX_FILE_SIZE];
    int position;

    // Menù dei comandi
    printMenu();
    
    while (1) {
        printf("Enter command: ");
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = 0;  // Remove newline(accapo) character
        
        if (strncmp(command, "createFile", 10) == 0) { 
            sscanf(command, "createFile %s", filename);
            createFile(&fs, filename);
        } 
        
        else if (strncmp(command, "eraseFile", 9) == 0) {
            sscanf(command, "eraseFile %s", filename);
            eraseFile(&fs, filename);
        }
         
        else if (strncmp(command, "write", 5) == 0) {
            sscanf(command, "write %s %[^\n]", filename, data);//%[^\n] Legge  i caratteri fino a newline(accapo) e li registra in Data
            write(&fs, filename, data);
        }
        
        else if (strncmp(command, "read", 4) == 0) {
            sscanf(command, "read %s", filename);
            char read_data[MAX_FILE_SIZE] = {0};
            read(&fs, filename, read_data, sizeof(read_data));
            printf("Data: %s\n", read_data);
        }
        
        else if (strncmp(command, "seek", 4) == 0) {
            sscanf(command, "seek %s %d", filename, &position);
            seek(&fs, filename, position);
        }
        
        else if (strncmp(command, "createDir", 9) == 0) {
            sscanf(command, "createDir %s", filename);
            createDir(&fs, filename);
        }
        
        else if (strncmp(command, "eraseDir", 8) == 0) {
            sscanf(command, "eraseDir %s", filename);
            eraseDir(&fs, filename);
        } 
        
        else if (strncmp(command, "changeDir", 9) == 0) {
            sscanf(command, "changeDir %s", filename);
            changeDir(&fs, filename);
        } 
        
        else if (strncmp(command, "listDir", 7) == 0) {
            listDir(&fs);
        } 
        
        else if (strncmp(command, "listFiles", 9) == 0) {
            listFiles(&fs);
        } 
        
        else if (strncmp(command, "pwd", 3) == 0) {
            printCurrentDir(&fs);
        } 
        
        else if (strncmp(command, "help", 4) == 0) {
            printMenu();
        } 
        
        else if (strncmp(command, "exit", 4) == 0) {
            break;
        } 
        
        else {
            printf("Unknown command. Type 'help' for the list of commands.\n");
        }
    }

    // Free this memory
    free(fs.buffer);
    free(fs.root->entries);
    free(fs.root);

    return 0;
}
