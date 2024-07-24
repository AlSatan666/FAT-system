#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file_system.h"

#define MAX_INPUT_SIZE 256000
#define MAX_ARGS 10
#define DATATICUS_FILE "DATATICUS.dat"

// Funzioni di utilità

void print_help() {
    printf("Commands:\n");
    printf("  mkfs                                     Initialize file system\n");
    printf("  loadfs                                   Load file system\n");
    printf("  savefs                                   Save file system\n");
    printf("  mkdir <name>                             Create directory\n");
    printf("  rmdir <name>                             Remove directory\n");
    printf("  mkfile <name>.<ext>                      Create file\n");
    printf("  rmfile <name>.<ext>                      Remove file\n");
    printf("  cd <name>                                Change directory\n");
    printf("  ls                                       List directory contents\n");
    printf("  write <name>.<ext> <offset> <data>       Write to file\n");
    printf("  read <name>.<ext>                        Read from file\n");
    printf("  seek <name>.<ext> <offset>               Seek within file\n");
    printf("  copy2fs <host> <fs>                      Copia un file dal sistema host al file system FAT.");
    printf("  copy2host  <fs> host>                    Copia un file dal file system FAT al sistema host.");
    printf("  exit                                     Exit the shell\n");
    printf("  help                                     Display this help message\n");
}

void parse_command(char* input, char** args) {
    int i = 0;
    char* token = strtok(input, " ");
    while (token != NULL && i < MAX_ARGS - 1) {
        args[i++] = token;
        if (i == 3 && strcmp(args[0], "write") == 0) {
            args[i++] = strtok(NULL, "\0"); 
            break;
        }
        token = strtok(NULL, " ");
    }
    args[i] = NULL;
}

void execute_command(char** args) {
    if (strcmp(args[0], "mkfs") == 0) {
        printf("Initializing file system...\n");
        fs_initialize(DATATICUS_FILE);
        printf("File system initialized.\n");
    } else if (strcmp(args[0], "loadfs") == 0) {
        printf("Loading file system...\n");
        fs_load(DATATICUS_FILE);
        printf("File system loaded.\n");
    } else if (strcmp(args[0], "savefs") == 0) {
        printf("Saving file system...\n");
        fs_save();
        printf("File system saved.\n");
    } else if (strcmp(args[0], "mkdir") == 0) {
        if (args[1]) {
            printf("Creating directory: %s\n", args[1]);
            create_dir(args[1]);
            printf("Directory created.\n");
        } else {
            printf("Usage: mkdir <name>\n");
        }
    } else if (strcmp(args[0], "rmdir") == 0) {
        if (args[1]) {
            printf("Removing directory: %s\n", args[1]);
            remove_dir(args[1], 1);
            printf("Directory removed.\n");
        } else {
            printf("Usage: rmdir <name>\n");
        }
    } else if (strcmp(args[0], "mkfile") == 0) {
        if (args[1]) {
            char* name = strsep(&args[1], ".");
            char* ext = args[1];
            if (ext) {
                printf("Creating file: %s.%s\n", name, ext);
                create_file(name, ext, 0, "");
                printf("File created.\n");
            } else {
                printf("Usage: mkfile <name>.<ext>\n");
            }
        } else {
            printf("Usage: mkfile <name>.<ext>\n");
        }
    } else if (strcmp(args[0], "rmfile") == 0) {
        if (args[1]) {
            char* name = strsep(&args[1], ".");
            char* ext = args[1];
            if (ext) {
                printf("Removing file: %s.%s\n", name, ext);
                remove_file(name, ext);
                printf("File removed.\n");
            } else {
                printf("Usage: rmfile <name>.<ext>\n");
            }
        } else {
            printf("Usage: rmfile <name>.<ext>\n");
        }
    } else if (strcmp(args[0], "cd") == 0) {
        if (args[1]) {
            printf("Changing directory to: %s\n", args[1]);
            int res = cd(args[1]);
            if (res == FILE_NOT_FOUND) {
                printf("Error: Directory '%s' not found.\n", args[1]);
            } else if (res == INVALID_DIRECTORY) {
                printf("Error: Invalid directory '%s'.\n", args[1]);
            } else {
                printf("Changed directory to: %s\n", args[1]);
            }
        } else {
            printf("Usage: cd <name>\n");
        }
    } else if (strcmp(args[0], "ls") == 0) {
        ls();
    } else if (strcmp(args[0], "write") == 0) {
        if (args[1] && args[2] && args[3]) {
            char* name = strsep(&args[1], ".");
            char* ext = args[1];
            int offset = atoi(args[2]);
            char* data = args[3];
            if (ext) {
                write_file_content(name, ext, data, offset, strlen(data));
            } else {
                printf("Usage: write <name>.<ext> <offset> <data>\n");
            }
        } else {
            printf("Usage: write <name>.<ext> <offset> <data>\n");
        }
    } else if (strcmp(args[0], "read") == 0) {
        if (args[1]) {
            char* name = strsep(&args[1], ".");
            char* ext = args[1];
            if (ext) {
                char buffer[1024];
                FileHandle handle;
                handle.file_entry = locate_file(name, ext, 0);
                handle.position = 0;
                int bytes_read = read_file_content(&handle, buffer, sizeof(buffer));
                buffer[bytes_read] = '\0';
            } else {
                printf("Usage: read <name>.<ext>\n");
            }
        } else {
            printf("Usage: read <name>.<ext>\n");
        }
    } else if (strcmp(args[0], "seek") == 0) {
        if (args[1] && args[2]) {
            char* name = strsep(&args[1], ".");
            char* ext = args[1];
            int offset = atoi(args[2]);
            if (ext) {
                FileHandle handle;
                handle.file_entry = locate_file(name, ext, 0);
                if (handle.file_entry) {
                    handle.position = 0;
                    int result = seek_file(&handle, offset, SEEK_SET);
                    if (result == 0) {
                        char buffer[1024];
                        int bytes_read = read_file_content(&handle, buffer, sizeof(buffer));
                        buffer[bytes_read] = '\0';
                        printf("Read from %s.%s: \"%s\"\n", name, ext, buffer);
                    } else {
                        printf("Seek failed in file: %s.%s\n", name, ext);
                    }
                } else {
                    printf("File not found: %s.%s\n", name, ext);
                }
            } else {
                printf("Usage: seek <name>.<ext> <offset>\n");
            }
        } else {
            printf("Usage: seek <name>.<ext> <offset>\n");
        }
    } else if (strcmp(args[0], "copy2fs") == 0) {
        if (args[1] && args[2]) {
            char* host_path = args[1];
            char* fs_path = args[2];
            char* name = strsep(&fs_path, ".");
            char* ext = fs_path;
            if (copy2fs(host_path, name, ext) == 0) {
                printf("File copied to FAT file system.\n");
            } else {
                printf("Failed to copy file to FAT file system.\n");
            }
        } else {
            printf("Usage: copy2fs <host> <fs>\n");
        }
    } else if (strcmp(args[0], "copy2host") == 0) {
        if (args[1] && args[2]) {
            char* fs_path = args[1];
            char* host_path = args[2];
            char* name = strsep(&fs_path, ".");
            char* ext = fs_path;
            if (copy2host(name, ext, host_path) == 0) {
                printf("File copied to host file system.\n");
            } else {
                printf("Failed to copy file to host file system.\n");
            }
        } else {
            printf("Usage: copy2host <fs> <host>\n");
        }
    } else if (strcmp(args[0], "help") == 0) {
        print_help();
    } else if (strcmp(args[0], "exit") == 0) {
        printf("Exiting shell...\n");
        exit(0);
    } else {
        printf("Unknown command: %s\n", args[0]);
        print_help();
    }
}


int main() {
    char input[MAX_INPUT_SIZE];
    char* args[MAX_ARGS];

    printf("Welcome to the FAT File System Shell\n");
    print_help();

    while (1) {
        printf("> ");
        if (fgets(input, sizeof(input), stdin) != NULL) {
            input[strcspn(input, "\n")] = '\0';
            parse_command(input, args);
            execute_command(args);
        }
    }

    return 0;
}

