#include "file_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

// Variabili statiche
static int fs_fd = -1;
static void *fs_base = NULL;
static uint32_t *fat = NULL;
static FileEntry *root_dir = NULL;
static uint32_t fs_size = 0;

int initialize_fs(const char *filename, uint32_t size) {
    fs_fd = open(filename, O_RDWR | O_CREAT, 0644);
    if (fs_fd < 0) {
        return -1;
    }

    if (ftruncate(fs_fd, size) < 0) {
        close(fs_fd);
        return -1;
    }

    fs_base = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fs_fd, 0);
    if (fs_base == MAP_FAILED) {
        close(fs_fd);
        return -1;
    }

    fat = (uint32_t *)fs_base;
    root_dir = (FileEntry *)(fs_base + BLOCK_SIZE);
    fs_size = size;

    // Initialize FAT
    memset(fat, 0xFF, FAT_ENTRIES_PER_BLOCK * sizeof(uint32_t));
    for (int i = 0; i < FAT_ENTRIES_PER_BLOCK; i++) {
        fat[i] = 0xFFFFFFFF;
    }
    printf("FAT initialized.\n");

    // Calculate root directory entries
    int root_dir_entries = (size - BLOCK_SIZE) / sizeof(FileEntry);
    memset(root_dir, 0, root_dir_entries * sizeof(FileEntry));
    printf("Root directory initialized with %d entries.\n", root_dir_entries);

    return 0;
}

int create_file(const char *filename) {
    if (strlen(filename) > MAX_FILENAME_LENGTH) {
        printf("Filename too long.\n");
        return -1;
    }

    int root_dir_entries = (fs_size - BLOCK_SIZE) / sizeof(FileEntry);

    // Check if file already exists
    for (int i = 0; i < root_dir_entries; i++) {
        if (strncmp(root_dir[i].name, filename, MAX_FILENAME_LENGTH) == 0) {
            printf("File already exists.\n");
            return -1;  // File already exists
        }
    }

    // Find an empty entry
    for (int i = 0; i < root_dir_entries; i++) {
        if (root_dir[i].name[0] == '\0') {
            strncpy(root_dir[i].name, filename, MAX_FILENAME_LENGTH);
            root_dir[i].size = 0;
            root_dir[i].first_block = 0xFFFFFFFF;
            printf("File created: %s\n", filename);
            return 0;
        }
    }

    printf("No space left in root directory.\n");
    return -1;  // No space left in root directory
}

int erase_file(const char *filename) {
    int root_dir_entries = (fs_size - BLOCK_SIZE) / sizeof(FileEntry);

    printf("Trying to erase file: %s\n", filename);

    // Trova il file
    for (int i = 0; i < 10; i++) {  // Limitiamo a 10 entry per il debug
        printf("Checking file entry %d: %s\n", i, root_dir[i].name);
        if (strncmp(root_dir[i].name, filename, MAX_FILENAME_LENGTH) == 0) {
            printf("Erasing file: %s\n", filename);
            // Libera i blocchi della FAT
            uint32_t current_block = root_dir[i].first_block;
            while (current_block != 0xFFFFFFFF) {
                uint32_t next_block = fat[current_block];
                fat[current_block] = 0xFFFFFFFF;
                printf("Freed block: %u\n", current_block);
                current_block = next_block;
            }

            // Cancella l'entry della directory
            memset(&root_dir[i], 0, sizeof(FileEntry));
            printf("File erased: %s\n", filename);
            return 0;
        }
    }

    printf("File not found: %s\n", filename);
    return -1;  // File non trovato
}

int write_file(const char *filename, uint32_t offset, const void *data, uint32_t size) {
    int root_dir_entries = (fs_size - BLOCK_SIZE) / sizeof(FileEntry);

    // Trova il file nella directory principale
    for (int i = 0; i < root_dir_entries; i++) {
        if (strncmp(root_dir[i].name, filename, MAX_FILENAME_LENGTH) == 0) {
            // Scrivi dati sul file
            uint32_t current_block = root_dir[i].first_block;
            uint32_t current_offset = offset;
            const uint8_t *data_ptr = (const uint8_t *)data;
            uint32_t remaining_size = size;

            printf("Writing data to file '%s' at offset %u with size %u\n", filename, offset, size);

            while (remaining_size > 0) {
                // Alloca un nuovo blocco se necessario
                if (current_block == 0xFFFFFFFF) {
                    printf("Allocating new block for file '%s'\n", filename);
                    // Trova un blocco libero
                    for (uint32_t j = 0; j < FAT_ENTRIES_PER_BLOCK; j++) {
                        if (fat[j] == 0xFFFFFFFF) {
                            fat[j] = 0xFFFFFFFE; // Segna il blocco come utilizzato
                            current_block = j;
                            break;
                        }
                    }

                    // Nessun blocco libero disponibile
                    if (current_block == 0xFFFFFFFF) {
                        printf("No space left on disk.\n");
                        return -1;
                    }

                    // Aggiorna il primo blocco del file se questo è il primo blocco
                    if (root_dir[i].first_block == 0xFFFFFFFF) {
                        root_dir[i].first_block = current_block;
                    } else {
                        // Aggiorna il blocco precedente con il nuovo blocco
                        uint32_t prev_block = root_dir[i].first_block;
                        while (fat[prev_block] != 0xFFFFFFFF) {
                            prev_block = fat[prev_block];
                        }
                        fat[prev_block] = current_block;
                    }
                }

                // Calcola la posizione all'interno del blocco
                uint32_t block_offset = current_offset % BLOCK_SIZE;
                uint32_t block_size = BLOCK_SIZE - block_offset;
                uint32_t write_size = (remaining_size < block_size) ? remaining_size : block_size;

                // Scrivi dati nel blocco
                printf("Writing %u bytes to block %u at block offset %u\n", write_size, current_block, block_offset);
                memcpy((uint8_t *)fs_base + BLOCK_SIZE * (current_block + 1) + block_offset, data_ptr, write_size);

                // Aggiorna i puntatori e le dimensioni
                data_ptr += write_size;
                current_offset += write_size;
                remaining_size -= write_size;

                // Passa al blocco successivo se necessario
                if (current_offset % BLOCK_SIZE == 0) {
                    uint32_t next_block = fat[current_block];
                    if (next_block == 0xFFFFFFFF && remaining_size > 0) {
                        printf("Allocating new block for continuation\n");
                        // Alloca un nuovo blocco
                        for (uint32_t j = 0; j < FAT_ENTRIES_PER_BLOCK; j++) {
                            if (fat[j] == 0xFFFFFFFF) {
                                fat[j] = 0xFFFFFFFE; // Segna il blocco come utilizzato
                                next_block = j;
                                break;
                            }
                        }

                        // Nessun blocco libero disponibile
                        if (next_block == 0xFFFFFFFF) {
                            printf("No space left on disk.\n");
                            return -1;
                        }

                        // Aggiorna il blocco successivo
                        fat[current_block] = next_block;
                    }
                    current_block = next_block;
                }
            }

            // Aggiorna la dimensione del file se necessario
            uint32_t new_size = offset + size;
            if (new_size > root_dir[i].size) {
                root_dir[i].size = new_size;
            }

            // Log dell'entry della directory
            printf("File entry updated: %s, size: %u, first block: %u\n", root_dir[i].name, root_dir[i].size, root_dir[i].first_block);

            printf("Data written to file: %s\n", filename);
            return 0;
        }
    }

    printf("File not found: %s\n", filename);
    return -1;  // File non trovato
}

void uninitialize_fs() {
    if (fs_base != NULL) {
        munmap(fs_base, fs_size);
        fs_base = NULL;
    }

    if (fs_fd >= 0) {
        close(fs_fd);
        fs_fd = -1;
    }
}

