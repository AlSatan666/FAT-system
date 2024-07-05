#include "file_system.h"

int main() {
    initialize_filesystem("filesystem.dat");

    FileHandle *fh = createFile("file1.txt");
    if (fh != NULL) {
        const char *text = "Hello, File System!";
        writeFile(fh, text, strlen(text));
    }

    createDir("dir1");
    changeDir("dir1");
    createFile("file2.txt");

    changeDir("/");

    listDir();

    return 0;
}
