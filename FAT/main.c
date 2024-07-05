#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "file_system.h"

int main() {
    FileSystem fs;
    initialize_filesystem(&fs, "filesystem.dat", 1024);

    create_file(&fs, "test.txt");

    FileHandle fh = {fs.root_dir[0].first_block, 0};
    const char *data = "Hello, World!";
    write_file(&fs, &fh, data, strlen(data));

    char buffer[20];
    fh.position = 0;
    read_file(&fs, &fh, buffer, strlen(data));
    buffer[strlen(data)] = '\0';
    printf("Read data: %s\n", buffer);

    create_dir(&fs, "docs");
    change_dir(&fs, "docs");
    list_dir(&fs);

    munmap(fs.data, fs.num_blocks * BLOCK_SIZE);
    close(fs.fd);
    return 0;
}
