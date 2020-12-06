#define _XOPEN_SOURCE  600 

#include "copy.h"

#define MAX_PATH_SIZE 1024

#define ERROR(error) fprintf(stderr, "Error in line %d, func %s: %s\n", __LINE__, __func__, strerror(error))


const char daemon_path[] = "home/stanislav/Documents/MIPT/3rd_semester/Computer_technologies/Reserv_copy/log_daemon.txt";
FILE* log_file;
FILE* log_daemon;
#define LOG(expr, ...) log_file = fopen("log.txt", "a"); \
                  assert(log_file); \
                  fprintf(log_file, expr, __VA_ARGS__); \
                  fflush(log_file); \
                  fclose(log_file);

#define LOG_D(expr, ...) log_file = fopen(daemon_path, "a"); \
                  assert(log_file); \
                  fprintf(log_file, expr, __VA_ARGS__); \
                  fflush(log_file); \
                  fclose(log_file);

void daemon_stop(int signum) {
    LOG_D("STOPPING DAEMON, logs are in %s\n", daemon_path);
    exit(EXIT_SUCCESS);
}

void daemon_stop(int signum) {
    LOG_D("PRINTING LOGS, logs are in %s\n", daemon_path);
    exit(EXIT_SUCCESS);
}

void init_daemon(const char* src, const char* dst) {
    pid_t pid = fork();

    if (pid < 0) {
        ERROR(errno);
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    umask(0);

    pid_t sid = setsid();

    if (sid < 0) {
        LOG_D("Error setting sid: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    if (chdir("/") < 0) {
        LOG_D("Error changing dir: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    pid_t daemon_pid = getpid();

    int fd = open("reserv_copy.pid", O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0) {
        LOG_D("Error opening pid file dir: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    int n_write = write(fd, &daemon_pid, sizeof(pid_t));
    assert(n_write == sizeof(pid_t));

    close(fd);

    LOG_D("Daemon initialized at %s\n", daemon_path);

    int time = 0;
    while(1) {
        
        signal(SIGUSR1, daemon_stop);
        signal(SIGUSR2, daemon_print);

        LOG_D("\n\n\nDaemon running %d second\n\n\n", time);
        sleep(10);

        char* src_name = src;
        char* dst_name = dst;

        int initial_indent = 1;

        init_dest_dir(dst_name);
        traverse(src_name, dst_name, initial_indent);

        time += 5;
    }

}

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
            LOG_D("Enrty %s exists\n", entry->d_name);
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

void change_time(char* dest_name) {

    char* argv[3];
    argv[0] = "-m";
    argv[1] = dest_name;
    argv[2] = NULL;

    int status = 0;

    int pid = fork();
    if (pid == -1) {
        ERROR(errno);
    }
    if (pid == 0) {
        execvp("touch", argv);
        ERROR(errno);
    }

    wait(&status);
}

void init_dest_dir(const char* dst_name) {
    mkdir(dst_name, 0777);
    if (errno != EEXIST) {
       LOG_D("Creating new backup directory: %s\n", strerror(errno));
    }
    
}

void traverse(char* src_name, char* dest_name, int indent) {

    char dest_path[MAX_PATH_SIZE];
    strcpy(dest_path, dest_name);

    struct dirent* entry = NULL;


    DIR* dir = opendir(src_name);
    if (dir == NULL) {
        LOG_D("Failed opening src directory, %s\n", strerror(errno));   
        exit(-1);
    }

    while ((entry = readdir(dir)) != NULL) {
        assert(entry);

        int df = dirfd(dir);
        if (df < 0) {
            LOG_D("Failed opening fd of  src directory, %s\n", strerror(errno));   
            exit(-1);
        }

        if (entry->d_type == DT_REG) {

            struct stat reg_info;
            fstatat(df, entry->d_name, &reg_info, 0);

            LOG_D("%*s File %s, Time since last modification: %ld sec\n", indent, \
                    "", entry->d_name, \
                    reg_info.st_mtime);

            // if file source_name not found in dest_name directory, copy it
            int exists = lookup(entry->d_name, dest_name);
            if (!exists) {

                char source_name[MAX_PATH_SIZE];
                snprintf(source_name, sizeof(source_name), "%s/%s", src_name, entry->d_name);
                LOG_D("NOT BACKUPED File %s, copying to %s\n", source_name, dest_name);
                copy(source_name, dest_name,  DT_REG);

            }

            // if it has been found, compare last modified times, if they differ copy again
            struct stat dest_info;

            DIR* dst_dir = opendir(dest_name);
            if (dst_dir < 0) {
                LOG_D("Failed opening dst directory, %s\n", strerror(errno));   
                exit(-1);
            }

            int dstf = dirfd(dst_dir);
            if (dstf < 0) {
                LOG_D("Failed opening fd of dst directory, %s\n", strerror(errno));   
                exit(-1);
            }
            
            fstatat(dstf, entry->d_name, &dest_info, 0);
            LOG_D("%*s File(in destination directory) %s, Time since last modification: %ld sec (compared with %ld in source)\n", indent, \
                    "", entry->d_name, \
                    dest_info.st_mtime, reg_info.st_mtime);
            closedir(dst_dir);

            if (dest_info.st_mtime < reg_info.st_mtime) {
                char source_name[MAX_PATH_SIZE];
                snprintf(source_name, sizeof(source_name), "%s/%s", src_name, entry->d_name);
                //LOG_D("Updating file : %s to %s\n", source_name, dest_name);
                LOG_D("UPDATING file %s\n", source_name);
                copy(source_name, dest_name,  DT_REG);
                change_time(dest_name);
            }
                                    
        } else if (entry->d_type == DT_DIR) {

            char src_path[MAX_PATH_SIZE];
            struct stat dir_info;
            fstatat(df, entry->d_name, &dir_info, 0);

            // skip . and .. directories
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            LOG_D("%*s Dir %s, Time since last modification: %ld\n", indent, \
                    "", entry->d_name, dir_info.st_mtime);

            // searching for directory d_name in dest_name, if does not exist - copy recursively
            int exists = lookup(entry->d_name, dest_name);

            if (!exists) {
                char source_name[MAX_PATH_SIZE];
                snprintf(source_name, sizeof(source_name), "%s/%s", src_name, entry->d_name);
                LOG_D("NOT BACKUPED Dir %s, copying to %s\n", source_name, dest_name);
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