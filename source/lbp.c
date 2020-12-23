/* This is a source file for LBackup program with code for all functions */


/* This is defined for some sys calls */
#define _XOPEN_SOURCE  600
#define _GNU_SOURCE

#include "lbp.h"

#define MAX_PATH_SIZE 1024

#define LINKS_NO_DEREF 0
#define LINKS_DEREF 1

#define ERROR(error) fprintf(stderr, "Error in line %d, func %s: %s\n", __LINE__, __func__, strerror(error))

int lnk_type = 0;

const char daemon_path[] = "/var/log/reserv_copy.log";
FILE* log_file;
FILE* log_daemon;


#define LOG(expr, ...)  do { \
                  log_file = fopen("log.txt", "a"); \
                  assert(log_file); \
                  fprintf(log_file, expr, __VA_ARGS__); \
                  fflush(log_file); \
                  fclose(log_file); \
                } while (0);


#define LOG_D(expr, ...)  do { \
                  log_file = fopen(daemon_path, "a"); \
                  assert(log_file); \
                  fprintf(log_file, expr, __VA_ARGS__); \
                  fflush(log_file); \
                  fclose(log_file);\
                } while (0);


// --------------------------------------------------------
void daemon_stop() {
    LOG_D("STOPPING DAEMON, logs are in %s\n", daemon_path);
    exit(EXIT_SUCCESS);
}

//----------------------------------------------------------------------
int check_args(int argc, char** argv) {

    const char links_shallow[] = "-H";
    const char links_deep[] = "-L";
    char* links_type = NULL;

    int deref = -1;
    int no_deref = -1;

    if (argc == 3) { /* no info about links dereferencing, shallow copy by default */
        lnk_type = LINKS_NO_DEREF;
    } else if (argc == 4) { /* check if the argument for links is a correct value */
        links_type = argv[3];
        no_deref = strcmp(links_type, links_shallow);
        deref = strcmp(links_type, links_deep);
        if (deref != 0 && no_deref != 0) {

                printf("\033[0;31m"); /* set color to red */
                printf("error: ");
                printf("\033[0m"); /* reset color */
                printf("unkown option: %s\n", links_type); 
                return -1;
        }

        /* set links type, lnk_type is a global variable defined in lbp.c */
        if (deref == 0) {
            lnk_type = LINKS_DEREF; 
            printf("Links have been set to dereferencing\n");
        } else {
            lnk_type = LINKS_NO_DEREF; 
            printf("Links have been set to no-dereferencing\n");
        }

    } else if (argc < 3 || argc > 4) {
        printf("usage: ./lbp <src> <dest> <links_type>\n");
        return -1;
    }

    return 0;
}

//----------------------------------------------------------------------
int check_dest_dir(char* src, char* dst) {

    int src_len = strlen(src);
    int compare = strncmp(src, dst, src_len);

    if (compare == 0)  {
        return 1;
    }

    return 0;
}

//----------------------------------------------------------------------
int check_source_dir(char* src) {
    int fd = open(src, O_RDONLY);

    if (fd < 0) {
        return -1;
    }
    close(fd);

    return 0;
}

// --------------------------------------------------------
void daemon_print(char* log_path) {

    char data[BUFSIZ];
    data[BUFSIZ - 1] = '\0';

    // open daemon logs for reading

    //LOG_D("log_path: %s\n", log_path);

    // open directory where logs should be printed to
    DIR* log_dir = opendir(log_path);
    if (log_dir == NULL) {
        LOG_D("Error opening log dir: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // get fd of log_path directory 
    int df = dirfd(log_dir);
    if (dirfd < 0) {
         LOG_D("Error opening log dir descriptor   : %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // create user log file at log_path directory, or open if one already exists
    int output = openat(df, "user_log.log", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (output < 0) {
        LOG_D("Error creating user log: %s\n", strerror(errno));
        output = openat(df, "user_log.log", O_WRONLY);
        if (output < 0) {
            LOG_D("Error opening user log: %s\n", strerror(errno));
        }
    }

    int log = open(daemon_path, O_RDONLY);
    if (log < 0) {
        LOG_D("Error opening daemon log file %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    int n_read = 0;
    // copy all data from daemon logs to created user log
    while ( (n_read = read(log, data, BUFSIZ / 2)) != 0) {
        if (n_read < 0) {
            //LOG_D("Error reading from daemon log file: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        int n_write = write(output, data, n_read);
        if (n_write < 0) {
            //LOG_D("Error writing to log file in cwd: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    LOG_D("PRINTING LOGS, logs are in %s\n", log_path);

    
    close(log);
    close(output);
    //exit(EXIT_SUCCESS);
}

// --------------------------------------------------------
// --------------------------------------------------------
void init_daemon(char* src, char* dst, int links_behaviour) {

    // process of initialization of daemon
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

    int fd = open("reserv_copy.pid", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        LOG_D("Error opening pid file dir: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    int n_write = write(fd, &daemon_pid, sizeof(pid_t));
    assert(n_write == sizeof(pid_t));

    close(fd);

    /* open fifo in non-blocking mode, check with poll until rc program interface
        transmits command */
    char* myfifo = "/tmp/reserv_fifo";

    int resop = mkfifo(myfifo, O_CREAT | 0666);
    if (resop < 0) {
        LOG_D("FIFO init error: %s\n", strerror(errno));
    }

    int fd_fifo = open(myfifo, O_RDONLY | O_NONBLOCK);
    if (fd_fifo < 0) {
        LOG_D("Error opening FIFO: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    lnk_type = links_behaviour;
    LOG_D("Daemon initialized at %s\n", daemon_path);
    if (links_behaviour == LINKS_NO_DEREF) {
        LOG_D("Links are NOT dereferenced, links_behaviour %d\n", links_behaviour);
    } else if (links_behaviour == LINKS_DEREF) {
        LOG_D("Links are SET FOR dereferencing, links_behaviour %d\n", links_behaviour);
    }

    int run_time = 0;

    while(1) {
        
        const int sleep_time = 10; // sleeping time in seconds

        char* src_name = NULL;
        char* dst_name = NULL;

        struct pollfd pfd;
        pfd.fd = fd_fifo;
        pfd.events = POLLIN;
        pfd.revents = 0;

        char data[BUFSIZ];
        data[BUFSIZ - 1] = '\0';

        int events = 0;

        events = poll(&pfd, 1, sleep_time * 1000);

        if (events > 0 && pfd.revents & POLLIN) {

            // reading command from fifo
            int n_read = read(fd_fifo, &data, sizeof(int));
            if (n_read != sizeof(int)) {
                LOG_D("Error reading from FIFO %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            LOG_D("Command read from pipe: %d\n", *((int*) data));

            // if command is 0, stop daemon
            if (*((int*) data) == 0) {
                close(fd_fifo);
                daemon_stop();
            } else if (*((int*) data) == 1) {

                // if command is 1, print daemon logs and stop daemon
                // also read log_path from fifo to data
                n_read = read(fd_fifo, &data, BUFSIZ);
                if (n_read == -1) {
                    LOG_D("Error reading from FIFO: %s\n", strerror(errno));
                }
                LOG_D("Bytes read: %d\n", n_read);
                LOG_D("Path transmitted: %s\n", data);
                close(fd_fifo);

                // reopen
                int fd_fifo = open(myfifo, O_RDONLY | O_NONBLOCK);
                if (fd_fifo < 0) {
                    LOG_D("Error opening FIFO: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                
                daemon_print(data);

                pfd.fd = fd_fifo;
                pfd.events = POLLIN;
                pfd.revents = 0;
                //pfd.events = POLLIN;
                //exit(EXIT_SUCCESS);
            } else if (*((int*) data) == 2) {

                char new_src[BUFSIZ];
                char new_dst[BUFSIZ];

                // if command is 2, set another file for backing up and another directory

                // read new src path
                n_read = read(fd_fifo, new_src, MAX_PATH_SIZE);
                if (n_read == -1) {
                    LOG_D("Error reading from FIFO: %s\n", strerror(errno));
                }
                LOG_D("Bytes read: %d\n", n_read);
                LOG_D("New src path transmitted: %s\n", new_src);

                // read new dst path
                n_read = read(fd_fifo, new_dst, MAX_PATH_SIZE);
                if (n_read == -1) {
                    LOG_D("Error reading from FIFO: %s\n", strerror(errno));
                }
                LOG_D("Bytes read: %d\n", n_read);
                LOG_D("New dst backup path transmitted: %s\n", new_dst);
                close(fd_fifo);

                src = new_src;
                dst = new_dst;

                fd_fifo = open(myfifo, O_RDONLY | O_NONBLOCK);
                if (fd_fifo < 0) {
                    LOG_D("Error opening FIFO: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }   
            }
        }


        LOG_D("\n\n\nDaemon running %d second\n\n\n", run_time);

        src_name = src;
        dst_name = dst;

        int initial_indent = 1;

        init_dest_dir(dst_name);

        int check = check_dest_dir(src_name, dst_name);
        if (check == 1) {
            exit(EXIT_FAILURE);
        }

        traverse(src_name, dst_name, initial_indent);

        run_time += sleep_time;
    }

}

// --------------------------------------------------------
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

// --------------------------------------------------------
void copy(char* src, char* dst, int type) {
    char cmd[] = "cp";
    char* argv[6];

    if (type == DT_DIR || type == LINKS_NO_DEREF) {
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
    } else if (type == LINKS_DEREF) {
        argv[0] = cmd;
        argv[1] = "-r";
        argv[2] = "-L";
        argv[3] = src;
        argv[4] = dst;
        argv[5] = NULL;
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

// --------------------------------------------------------
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

// --------------------------------------------------------
void init_dest_dir(const char* dst_name) {
    mkdir(dst_name, 0777);
    if (errno != EEXIST) {
       LOG_D("Creating new backup directory: %s\n", strerror(errno));
    }
}

// --------------------------------------------------------
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

        time_t rawtime;
        struct tm* timeinfo;

        time (&rawtime);
        timeinfo = localtime(&rawtime);

        if (entry->d_type == DT_REG) {

            struct stat reg_info;
            fstatat(df, entry->d_name, &reg_info, 0);


            LOG_D("%*s File %s, Time since last modification: %ld sec, log time: %s\n", indent, \
                    "", entry->d_name, \
                    reg_info.st_mtime, \
                    asctime (timeinfo));                 \

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
        } else if (entry->d_type == DT_LNK) {

            // get symlink info
            struct stat link_info;
            fstatat(df, entry->d_name, &link_info, 0);

            // skip . and .. directories may be unnecesary
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            LOG_D("%*s Symlink %s, Time since last modification: %ld\n", indent, \
                    "", entry->d_name, link_info.st_mtime);

            // searching for symlink d_name in dest_name, copy only link by default
            //int exists = lookup(entry->d_name, dest_name);

            char source_name[MAX_PATH_SIZE];
            snprintf(source_name, sizeof(source_name), "%s/%s", src_name, entry->d_name);

            // if links are not set for deref, only copy symlink
            if (lnk_type == LINKS_NO_DEREF) {
                LOG_D("COPYING Symlink %s, to %s\n", source_name, dest_name);
                copy(source_name, dest_name, LINKS_NO_DEREF);
            } else {
                LOG_D("COPYING ALL CONTENTS OF SYMLINK %s, to %s\n", source_name, dest_name);
                copy(source_name, dest_name, LINKS_DEREF);
            }
            // do not search in directory that has just been copied
            continue;
        } else {
            fprintf(stderr, "Not regular file or directory\n");
        }
    }

    closedir(dir);
}