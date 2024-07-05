#include "file_system.h"

FAT *fat;
void *data_blocks;

void initialize_filesystem(const char *filepath) {
    int fd = open(filepath, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(fd, sizeof(FAT) + BLOCK_SIZE * MAX_BLOCKS) == -1) {
        perror("Failed to resize file");
        close(fd);
        exit(EXIT_FAILURE);
    }

    void *map = mmap(NULL, sizeof(FAT) + BLOCK_SIZE * MAX_BLOCKS, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        perror("Failed to map file");
        close(fd);
        exit(EXIT_FAILURE);
    }

    fat = (FAT *)map;
    data_blocks = map + sizeof(FAT);

    if (fat->file_count == 0 && fat->dir_count == 0) {
        // Initialize root directory if not already initialized
        strcpy(fat->dirs[0].name, "/");
        fat->dirs[0].start_block = 0;
        fat->dirs[0].size = 0;
        fat->dir_count = 1;
        fat->current_dir = 0;
    }

    close(fd);
}

FileHandle *createFile(const char *filename) {
    if (fat->file_count >= MAX_FILES) {
        printf("File limit reached.\n");
        return NULL;
    }

    for (int i = 0; i < fat->file_count; i++) {
        if (strcmp(fat->files[i].name, filename) == 0) {
            printf("File already exists.\n");
            return NULL;
        }
    }

    strcpy(fat->files[fat->file_count].name, filename);
    fat->files[fat->file_count].size = 0;
    fat->files[fat->file_count].start_block = -1;
    fat->file_count++;

    FileHandle *handle = malloc(sizeof(FileHandle));
    if (handle == NULL) {
        perror("Failed to allocate FileHandle");
        return NULL;
    }
    handle->block_index = fat->files[fat->file_count - 1].start_block;
    handle->position = 0;

    return handle;
}

void eraseFile(const char *filename) {
    for (int i = 0; i < fat->file_count; i++) {
        if (strcmp(fat->files[i].name, filename) == 0) {
            for (int j = i; j < fat->file_count - 1; j++) {
                fat->files[j] = fat->files[j + 1];
            }
            fat->file_count--;
            return;
        }
    }
    printf("File not found.\n");
}

void writeFile(FileHandle *handle, const void *buffer, int size) {
    if (handle->block_index == -1) {
        handle->block_index = fat->file_count - 1;
        fat->files[handle->block_index].start_block = handle->block_index;
    }

    if (handle->position + size > BLOCK_SIZE) {
        printf("Not enough space in block to write data.\n");
        return;
    }

    void *block = data_blocks + handle->block_index * BLOCK_SIZE + handle->position;
    memcpy(block, buffer, size);
    handle->position += size;
    fat->files[handle->block_index].size += size;
}

void readFile(FileHandle *handle, void *buffer, int size) {
    if (handle->position + size > fat->files[handle->block_index].size) {
        printf("Read exceeds file size.\n");
        return;
    }

    void *block = data_blocks + handle->block_index * BLOCK_SIZE + handle->position;
    memcpy(buffer, block, size);
    handle->position += size;
}

void seekFile(FileHandle *handle, int position) {
    if (position > fat->files[handle->block_index].size) {
        printf("Seek position exceeds file size.\n");
        return;
    }

    handle->position = position;
}

void createDir(const char *dirname) {
    if (fat->dir_count >= MAX_FILES) {
        printf("Directory limit reached.\n");
        return;
    }

    for (int i = 0; i < fat->dir_count; i++) {
        if (strcmp(fat->dirs[i].name, dirname) == 0) {
            printf("Directory already exists.\n");
            return;
        }
    }

    strcpy(fat->dirs[fat->dir_count].name, dirname);
    fat->dirs[fat->dir_count].size = 0;
    fat->dirs[fat->dir_count].start_block = -1;
    fat->dir_count++;
}

void eraseDir(const char *dirname) {
    for (int i = 0; i < fat->dir_count; i++) {
        if (strcmp(fat->dirs[i].name, dirname) == 0) {
            for (int j = i; j < fat->dir_count - 1; j++) {
                fat->dirs[j] = fat->dirs[j + 1];
            }
            fat->dir_count--;
            return;
        }
    }
    printf("Directory not found.\n");
}

void changeDir(const char *dirname) {
    for (int i = 0; i < fat->dir_count; i++) {
        if (strcmp(fat->dirs[i].name, dirname) == 0) {
            fat->current_dir = i;
            return;
        }
    }
    printf("Directory not found.\n");
}

void listDir() {
    printf("Listing contents of directory: %s\n", fat->dirs[fat->current_dir].name);
    printf("Directories:\n");
    for (int i = 0; i < fat->dir_count; i++) {
        printf("%s/\n", fat->dirs[i].name);
    }
    printf("Files:\n");
    for (int i = 0; i < fat->file_count; i++) {
        printf("%s\n", fat->files[i].name);
    }
}
