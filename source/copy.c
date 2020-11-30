#define _XOPEN_SOURCE  600 

#include "copy.h"

#define MAX_PATH_SIZE 1024

#define ERROR(error) fprintf(stderr, "Error in line %d, func %s: %s\n", __LINE__, __func__, strerror(error))

#define LOG(expr, ...) FILE* log = fopen("log.txt", "a"); \
                  assert(log); \
                  fprintf(log, expr, __VA_ARGS__); \
                  fflush(log);
                  fclose(log);

int lookup(const char* name, const char* dir) { 

    DIR* directory = opendir(dir);
    if (directory == NULL) {
        ERROR(errno);
        exit(-1);
    }

    struct dirent* entry = NULL;

    while ((entry = readdir(directory)) != NULL) {
        assert(entry);

        if (strcmp(entry->d_name, name) == 0) {
            LOG("Enrty %s exists\n", entry->d_name);
            closedir(directory);
            return 1;
        }
    }
    
    closedir(directory);
    return 0;
}

void copy(char* src, char* dst, int type) {
    char cmd[] = "cp";
    char* argv[5];

    if (type == DT_DIR) {
        argv[0] = cmd;
        argv[1] = "-r";
        argv[2] = src;
        argv[3] = dst;
        argv[4] = NULL;
    } else if (type == DT_REG) {
        argv[0] = cmd;
        argv[1] = src;
        argv[2] = dst;
        argv[3] = NULL;
    } else {
        fprintf(stderr, "No copy type recognized\n");
    }

    int status = 0;
    int pid = fork();
    if (pid == 0) {
        execvp(cmd, argv);
        ERROR(errno);
    }
    wait(&status);
}


void traverse(char* src_name, char* dest_name, int indent) {

    char dest_path[MAX_PATH_SIZE];
    strcpy(dest_path, dest_name);

    struct dirent* entry = NULL;

    DIR* dir = opendir(src_name);
    if (dir == NULL) {
        ERROR(errno);   
        exit(-1);
    }

    while ((entry = readdir(dir)) != NULL) {
        assert(entry);

        int df = dirfd(dir);
        if (df < 0) {
            ERROR(errno);
            exit(-1);
        }

        if (entry->d_type == DT_REG) {

            struct stat reg_info;
            fstatat(df, entry->d_name, &reg_info, 0);

            LOG("%*s File %s, Time since last modification: %ld sec\n", indent, \
                    "", entry->d_name, \
                    reg_info.st_mtime);

            // if file source_name not found in dest_name directory, copy it
            int exists = lookup(entry->d_name, dest_name);
            if (!exists) {

                char source_name[MAX_PATH_SIZE];
                snprintf(source_name, sizeof(source_name), "%s/%s", src_name, entry->d_name);
                LOG("Copying file : %s to %s\n", source_name, dest_name);
                copy(source_name, dest_name,  DT_REG);

            }
                                    
        } else if (entry->d_type == DT_DIR) {

            char src_path[MAX_PATH_SIZE];
            struct stat dir_info;
            fstatat(df, entry->d_name, &dir_info, 0);

            // skip . and .. directories
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            LOG("%*s Dir %s, Time since last modification: %ld\n", indent, \
                    "", entry->d_name, dir_info.st_mtime);

            // searching for directory d_name in dest_name, if does not exist - copy recursively
            int exists = lookup(entry->d_name, dest_name);

            if (!exists) {
                char source_name[MAX_PATH_SIZE];
                snprintf(source_name, sizeof(source_name), "%s/%s", src_name, entry->d_name);
                LOG("Copying dir : %s to %s\n", source_name, dest_name);
                copy(source_name, dest_name, DT_DIR);
                // do not search in directory that has just been copied
                continue;
            }

            //update path for seraching in subdirectory
            snprintf(src_path, sizeof(src_path), "%s/%s", src_name, entry->d_name);
            snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_name, entry->d_name);

            traverse(src_path, dest_path, indent + 5);
        } else {
            fprintf(stderr, "Not regular file or directory\n");
        }
    }

    closedir(dir);
}