#include "copy.h"
#define MAX_PATH_SIZE 1024

#define ERROR(error) printf("Error in line %d, func %s: %s\n", __LINE__, __func__, strerror(error))

int lookup(const char* name, const char* dir_name) { // for regular file in dir for now

    DIR* dir = opendir(dir_name);
    if (dir == NULL) {
        ERROR(errno);
        exit(-1);
    }

    struct dirent* entry = NULL;
    while ((entry = readdir(dir)) != NULL) {
        assert(entry);

        if (entry->d_type == DT_REG) {
            if (strcmp(entry->d_name, name) == 0) {
                printf("File exists\n");
                return 1;
            }
        }

        if (entry->d_type == DT_DIR) {
            if (strcmp(entry->d_name, name) == 0) {
                printf("Dir exists\n");
                return 1;
            }
        }

    }
    
    closedir(dir);
    return 0;
}

void copy(const char* src, const char* dst) {
    char cmd[] = "cp";

    char* argv[4];
    argv[0] = cmd;
    argv[1] = src;
    argv[2] = dst;
    argv[3] = NULL;

    int pid = fork();
    if (pid == 0) {
        execvp(cmd, argv);
        ERROR(errno);
    }

}


void traverse(const char* name, int indent, const char* dest_name) {

    char dest_path[MAX_PATH_SIZE];
    snprintf(dest_path, sizeof(dest_path), "%s/", dest_name);
    
    struct dirent* entry = NULL;

    DIR* dir = opendir(name);
    if (dir == NULL) {
        ERROR(errno);   
        exit(-1);
    }

    while ((entry = readdir(dir)) != NULL) {
        assert(entry);

        int df = dirfd(dir);
        assert(df > 0);

        if (entry->d_type == DT_REG) {

            struct stat reg_info;
            fstatat(df, entry->d_name, &reg_info, 0);
            //printf("%*s File %s, Time since last modification: %ld sec\n", indent, \
                                    "", entry->d_name, \
                                    reg_info.st_mtime); \
            
            int exists = lookup(entry->d_name, dest_path);
            if (!exists) {

                char fname_source[MAX_PATH_SIZE];
                snprintf(fname_source, sizeof(fname_source), "%s/%s", name, entry->d_name);
                printf("Copying file : %s to %s\n", fname_source, dest_name);
                copy(fname_source, dest_name);

            }
                                    
        } else if (entry->d_type == DT_DIR) {

            char path[MAX_PATH_SIZE];
            struct stat dir_info;
            fstatat(df, entry->d_name, &dir_info, 0);

            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            //printf("%*s Dir %s, Time since last modification: %ld\n", indent, \
                                    "", entry->d_name, dir_info.st_mtime);

            //update path
            snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);

            // int exists = lookup(entry->d_name, dest_path);

            // if (!exists) {
            //     char dname_source[MAX_PATH_SIZE];
            //     snprintf(dname_source, sizeof(dname_source), "%s/%s", name, entry->d_name);
            //     printf("Copying dir : %s to %s\n", dname_source, dest_name);
            //     copy(dname_source, dest_name);

            // }

            traverse(path, indent + 6, dest_path);
        } else {
            printf("Not regular file or directory\n");
        }
    }

    closedir(dir);
}