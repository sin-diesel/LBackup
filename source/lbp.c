/* This is a source file for LBackup program with code for all functions */


/* This is defined for some sys calls */
#define _XOPEN_SOURCE 600
#define _GNU_SOURCE

#include "lbp.h"

#define MAX_PATH_SIZE 1024
#define INDENT_SIZE 8 /* this is value by which indent is increased in each log
                        file directory entry */

#define LINKS_NO_DEREF 0
#define LINKS_DEREF 1

#define COPY_RW

/* Error macros */
#define ERROR(error) fprintf(stderr, "Error in line %d, func %s: %s\n", __LINE__, __func__, strerror(error))
#define ERROR_CLR(error)  do {                                          \
                            printf("\033[0;31m"); /* set color to red */ \
                            printf("error: ");                          \
                            printf("\033[0m"); /* reset color */            \
                            printf("%s\n", #error);                         \
                        while (0);                                          \

static const char myfifo[] = "/tmp/lbp_fifo";
/* Logger macros */
FILE* log_file;
int lnk_type = 0; /* this specifies whether to dereference links or not to */
const char log_path[] = "/var/log/lbp.log";

#define LOG(expr, ...)  do { \
                  log_file = fopen(log_path, "a");    \
                  assert(log_file);                      \
                  fprintf(log_file, expr, __VA_ARGS__);  \
                  fflush(log_file);                      \
                  fclose(log_file);                      \
                } while (0);

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

//----------------------------------------------------------------------
void init_daemon(char* src, char* dst, int links_behaviour) {

    // process of initialization of daemon
    LOG("Initilization of daemon with logs at %s\n", log_path);
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
        LOG("Error setting sid: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    if (chdir("/") < 0) {
        LOG("Error changing dir: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    pid_t daemon_pid = getpid();

    int fd = open("lbp.pid", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        LOG("Error opening pid file dir: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    int n_write = write(fd, &daemon_pid, sizeof(pid_t));
    assert(n_write == sizeof(pid_t));
    close(fd);

    /* open fifo in non-blocking mode, check with poll until rc program interface
        transmits command */

    lnk_type = links_behaviour;

    LOG("Daemon log initialized at %s\n", log_path);
    if (links_behaviour == LINKS_NO_DEREF) {
        LOG("Links are NOT dereferenced, links_behaviour %d\n", links_behaviour);
    } else if (links_behaviour == LINKS_DEREF) {
        LOG("Links are SET FOR dereferencing, links_behaviour %d\n", links_behaviour);
    }

}

//----------------------------------------------------------------------
void run_backup(char* src, char* dst) {

    const int sleep_time = 10; // sleeping time in seconds
    int run_time = 0;
    struct pollfd pfd;
    int check = 0;

    char new_src[BUFSIZ];
    char new_dst[BUFSIZ];

    char data[BUFSIZ];
    data[BUFSIZ - 1] = '\0';

    int resop = mkfifo(myfifo, O_CREAT | 0666);
    if (resop < 0) {
        LOG("FIFO init error: %s\n", strerror(errno));
    }

    int fd_fifo = open(myfifo, O_RDONLY | O_NONBLOCK);
    if (fd_fifo < 0) {
        LOG("Error opening FIFO: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    pfd.fd = fd_fifo;
    pfd.events = POLLIN;
    pfd.revents = 0;

    while(1) {

        int events = 0;
        time_t log_time;
        events = poll(&pfd, 1, sleep_time * 1000); /* multiplied by 1000 for milliseconds */

        if (events > 0 && pfd.revents & POLLIN) { /* if there is data in fifo */
            /* reading command from fifo */
            int n_read = read(fd_fifo, &data, sizeof(int));
            if (n_read != sizeof(int)) {
                LOG("Error reading from FIFO %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            LOG("Command read from pipe: %d\n", *((int*) data));

            /* if command is 0, stop daemon */
            if (*((int*) data) == STOP) {
                close(fd_fifo);
                daemon_stop();
            } else if (*((int*) data) == PRINT) {

                /* if command is 1, print daemon logs and stop daemon
                 also read log_path from fifo to data */
                n_read = read(fd_fifo, &data, BUFSIZ);
                if (n_read == -1) {
                    LOG("Error reading from FIFO: %s\n", strerror(errno));
                }
                LOG("Bytes read: %d\n", n_read);
                LOG("Path transmitted: %s\n", data);

                close(fd_fifo);

                // reopen
                fd_fifo = open(myfifo, O_RDONLY | O_NONBLOCK);
                if (fd_fifo < 0) {
                    LOG("Error opening FIFO: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                daemon_print(data);

                pfd.fd = fd_fifo;
                pfd.events = POLLIN;
                pfd.revents = 0;
            } else if (*((int*) data) == SET) {
                /* if command is 2, set another file for backing up and another directory */

                /* read new src path */
                n_read = read(fd_fifo, new_src, MAX_PATH_SIZE);
                if (n_read == -1) {
                    LOG("Error reading from FIFO: %s\n", strerror(errno));
                }
                LOG("Bytes read: %d\n", n_read);
                LOG("New src path transmitted: %s\n", new_src);

                /* read new dst path */
                n_read = read(fd_fifo, new_dst, MAX_PATH_SIZE);
                if (n_read == -1) {
                    LOG("Error reading from FIFO: %s\n", strerror(errno));
                }
                LOG("Bytes read: %d\n", n_read);
                LOG("New dst backup path transmitted: %s\n", new_dst);
                close(fd_fifo);

                /* Check if directory lies within the source directory */
                check = check_dest_dir(new_src, new_dst);
                if (check == 1) {
                    LOG("Error: specified backup directory %s lies in source directory %s.\n",new_dst, new_src);
                } else {
                    /* Okay to switch directories, otherwise continue operating on previously
                    specified directories */
                    src = new_src;
                    dst = new_dst;
                }

                fd_fifo = open(myfifo, O_RDONLY | O_NONBLOCK);
                if (fd_fifo < 0) {
                    LOG("Error opening FIFO: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }   
            }
        }
        time(&log_time);

        LOG("\n\n\nLog time: %s\n", ctime(&log_time));
        LOG("Daemon running %d second\n\n\n", run_time);

        int initial_indent = 1; /* starting with 1 indent */

        traverse(src, dst, initial_indent);

        run_time += sleep_time;
    }

}


//----------------------------------------------------------------------
void traverse(char* src, char* dst, int indent) {

    char dst_path[MAX_PATH_SIZE]; /* destination path buffer */
    char src_path[MAX_PATH_SIZE];
    char link_path[MAX_PATH_SIZE];
    char buf[MAX_PATH_SIZE];
    char* name = NULL;
    buf[MAX_PATH_SIZE] = '\0';
    dst_path[MAX_PATH_SIZE - 1] = '\0';
    src_path[MAX_PATH_SIZE - 1] = '\0';
    link_path[MAX_PATH_SIZE - 1] = '\0';

    struct dirent* entry = NULL;
    DIR* dir = NULL;
    DIR* dst_dir = NULL;
    int df = 0;
    int res = 0;
    int dst_fd = 0;
    int src_fd = 0;
    int exists = 0;
    int src_size = 0;
    int n_read = 0;
    int n_write = 0;
    // time_t rawtime;
    // struct tm* timeinfo = NULL;
    struct stat src_info;
    struct stat dst_info;
    struct stat link_info;

    strcpy(dst_path, dst); /* copying dst full name to buffer */

    LOG("Checking entry: %s\n", src);
    /* Handle the case where src is not a directory, but a file */
    res = stat(src, &src_info);
    if (res < 0) {
        LOG("Error statting src file: %s\n", strerror(errno));
    }
    if ((src_info.st_mode & S_IFMT) == S_IFREG) {
        memcpy(buf, src, MAX_PATH_SIZE);
        name = basename(buf);
        LOG("Reg file in traverse: %s\n", name);
        copy_reg(src, dst, name);
        return 0;
    }

    // if (src_info.st_mode == S_IFREG) {
    //     src_fd = open(src, O_RDONLY);
    //     if (src_fd < 0) {
    //         LOG("Error opening src file: %s\n", strerror(errno));
    //     }

    //     src_size = src_info.st_size;

    //     n_read = read(src_fd, &buf, src_size);
    //     if (n_read != src_size) {
    //         LOG("Error reading, read: %d, expected: %d\n", n_read, src_size);
    //     }

    //     dst_fd = open(dst, O_WRONLY | );
    //     if (dst_fd < 0) {
    //         LOG("Error opening dst file: %s\n", strerror(errno));
    //     }
    //     n_write = write()

    // }

    dir = opendir(src);
    if (dir == NULL) {
        LOG("Failed opening src directory, %s\n", strerror(errno));   
        exit(-1);
    }

    while ((entry = readdir(dir)) != NULL) {
        assert(entry);

        df = dirfd(dir);
        if (df < 0) {
            LOG("Failed opening fd of  src directory, %s\n", strerror(errno));   
            exit(-1);
        }

        if (entry->d_type == DT_REG) { /* check if the file type is a regular file */

            fstatat(df, entry->d_name, &src_info, 0);

            LOG("%*s File %s, Time since last modification: %ld sec\n", indent, \
                    "", entry->d_name, \
                    src_info.st_mtime);             

            /* if file d_name not found in dst directory, copy it */
            exists = lookup(entry->d_name, dst);
            if (!exists) {
                snprintf(src_path, sizeof(src_path), "%s/%s", src, entry->d_name);
                LOG("NOT BACKUPED File %s, copying to %s\n", src, dst);
                #ifndef COPY_RW
                copy(src_path, dst_path,  DT_REG);
                #else 
                /* Copy regular file, where dst is the name of a directory where 
                we are copying to */
                res = copy_reg(src_path, dst, entry->d_name);
                if (res < 0) {
                    exit(EXIT_FAILURE);
                }
                #endif
            }

            /* if it has been found, compare last modified times, if they differ copy again */

            dst_dir = opendir(dst);
            if (dst_dir < 0) {
                LOG("Failed opening dst directory, %s\n", strerror(errno));   
                exit(-1);
            }

            int dstf = dirfd(dst_dir);
            if (dstf < 0) {
                LOG("Failed opening fd of dst directory, %s\n", strerror(errno));   
                exit(-1);
            }
            
            fstatat(dstf, entry->d_name, &dst_info, 0);
            LOG("%*s File(in destination directory) %s, Time since last modification: %ld sec (compared with %ld in source)\n", indent, \
                    "", entry->d_name, \
                    dst_info.st_mtime, src_info.st_mtime);
            closedir(dst_dir);

            if (dst_info.st_mtime < src_info.st_mtime) {
                char source_name[MAX_PATH_SIZE];
                snprintf(source_name, sizeof(source_name), "%s/%s", src, entry->d_name);
                LOG("UPDATING file %s\n", source_name);
                #ifndef COPY_RW
                copy(source_name, dst,  DT_REG);
                #else
                /* Copying again */
                res = copy_reg(source_name, dst, entry->d_name);
                if (res < 0) {
                    exit(EXIT_FAILURE);
                }
                #endif
                change_time(dst);
            }
                                    
        } else if (entry->d_type == DT_DIR) {

            fstatat(df, entry->d_name, &src_info, 0);

            /* skip . and .. directories */
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            LOG("%*s Dir %s, Time since last modification: %ld\n", indent, \
                    "", entry->d_name, src_info.st_mtime);

            /* searching for directory d_name in dest_name, if does not exist - copy recursively */
            exists = lookup(entry->d_name, dst);

            if (!exists) {
                snprintf(src_path, sizeof(src_path), "%s/%s", src, entry->d_name);
                LOG("NOT BACKUPED Dir %s, copying to %s\n", src, dst);
                #ifndef COPY_RW
                copy(src_path, dst_path, DT_DIR);
                #else
                /* If we see a directory, update dst_path and create a new directory,
                then traverse it */

                snprintf(dst_path, sizeof(dst_path), "%s/%s", dst, entry->d_name);
                res = mkdir(dst_path, 0777); // 0777 for execute permission
                if (res < 0) {
                    LOG("Error creating directory: %s\n", strerror(errno));
                }
                #endif
                /* do not search in directory that has just been copied */
                #ifndef COPY_RW
                continue;
                #endif
            }

            /* update path for searching in subdirectory */
            snprintf(src_path, sizeof(src_path), "%s/%s", src, entry->d_name);
            /* I HAVE NO IDEA WHY THIS IS WORKING WITHOUT IFNDEF WHEN IT IS NOT CORRECT */
            //#ifndef COPY_RW
            snprintf(dst_path, sizeof(dst_path), "%s/%s", dst, entry->d_name);
            //#endif

            /* recursive copy */
            traverse(src_path, dst_path, indent + INDENT_SIZE);

        /* Handle links with read/write */
        } else if (entry->d_type == DT_LNK) {

            /* get symlink info */
            fstatat(df, entry->d_name, &link_info, 0);

            /* skip . and .. directories may be unnecesary */
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            LOG("%*s Symlink %s, Time since last modification: %ld\n", indent, \
                    "", entry->d_name, link_info.st_mtime);

            /* See if link has already been copied */
            exists = lookup(entry->d_name, dst);
            if (!exists) {
                /* searching for symlink d_name in dest_name, copy only link by default */

                snprintf(src_path, sizeof(src_path), "%s/%s", src, entry->d_name);

                res = readlink(src_path, link_path, MAX_PATH_SIZE);
                if (res < 0) {
                    LOG("Error reading link: %s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                /* Make null-terminated */
                link_path[res] = '\0';

                LOG("Link contents: %s\n", link_path);
                LOG("dst path: %s\n", dst);
                LOG("Entry name: %s\n", entry->d_name);
                    
                /* if links are not set for deref, only copy symlink */
                if (lnk_type == LINKS_NO_DEREF) {
                    LOG("COPYING Symlink %s, to %s\n", src_path, dst);
                    #ifndef COPY_RW
                    copy(src_path, dst_path, LINKS_NO_DEREF);
                    #else
                    /* creating symlink in dst */

                    //snprintf(dst_path, sizeof (dst_path), "%s/%s", dst, entry->d_name);

                    /* Temporary buffer for creating symlink path */

                    char temp[MAX_PATH_SIZE];
                    snprintf(temp, sizeof(temp), "%s/%s", dst, entry->d_name);

                    res = symlink(link_path, temp);
                    if (res < 0) {
                        LOG("Error creating symlink: %s\n", strerror(errno));
                        exit(EXIT_FAILURE);
                    }
                    #endif
                } else {
                    LOG("COPYING ALL CONTENTS OF SYMLINK %s, to %s\n", src_path, \
                    dst_path);   
                    #ifndef COPY_RW
                    copy(src_path, dst_path, LINKS_DEREF);
                    #else
                    //snprintf(dst_path, sizeof(dst_path), "%s/%s", dst, entry->d_name);

                    /* recursive copy */
                    traverse(link_path, dst, indent + INDENT_SIZE);
                    #endif
                }
                /* do not search in directory that has just been copied */
                continue;
            }
        } else {
            fprintf(stderr, "Not regular file or directory\n");
        }
    }

    closedir(dir);
}

//----------------------------------------------------------------------
void daemon_stop() {
    LOG("STOPPING DAEMON, logs are in %s\n", log_path);
    exit(EXIT_SUCCESS);
}


//----------------------------------------------------------------------
void daemon_print(char* new_log_path) {

    char data[BUFSIZ];
    data[BUFSIZ - 1] = '\0';
    DIR* log_dir = NULL;
    LOG("Copying all data from /var/log to created log%s\n", log_path);

    /* open daemon logs for reading */
    /* open directory where logs should be printed to */
    log_dir = opendir(new_log_path);
    if (log_dir == NULL) {
        LOG("Error opening log dir: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* get fd of log_path directory */
    int df = dirfd(log_dir);
    if (dirfd < 0) {
        LOG("Error opening log dir descriptor   : %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* create user log file at new_log_path directory, or open if one already exists */
    int output = openat(df, "lbp.log", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (output < 0) {
        LOG("Error creating log: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
        output = openat(df, "lbp.log", O_WRONLY);
        if (output < 0) {
            LOG("Error opening log: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    int log = open(log_path, O_RDONLY);
    if (log < 0) {
        LOG("Error opening /var/log file %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    int n_read = 0;
    /* copy all data from /var/log to created log */
    while ( (n_read = read(log, data, BUFSIZ / 2)) != 0) { /* divided by 2 simply for safety
                                                                precautions */
        if (n_read < 0) {
            exit(EXIT_FAILURE);
        }

        int n_write = write(output, data, n_read);
        if (n_write < 0) {
            exit(EXIT_FAILURE);
        }
    }

    LOG("PRINTING LOGS, logs are in %s\n", new_log_path);

    close(log);
    close(output);
}

//----------------------------------------------------------------------
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
            LOG("Entry %s exists\n", entry->d_name);
            closedir(directory);
            return 1;
        }
    }
    
    closedir(directory);
    return 0;
}

//----------------------------------------------------------------------
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

int copy_reg(char* src, char* dst, char* name) {

    struct stat info;

    char dst_path[MAX_PATH_SIZE];
    char src_path[MAX_PATH_SIZE];
    DIR* dir = NULL;
    char* file_name = NULL;

    int fd_src = 0;
    int fd_dst = 0;
    int drfd = 0;

    int res = 0;

    int n_read = 0;
    int n_write = 0;

    char buf[BUFSIZ];
    int src_size = 0;

    fd_src = open(src, O_RDONLY);
    if (fd_src < 0) {
        LOG("Error opening source file %s for reading: %s\n", src, strerror(errno));
        return -1;
    }

    /* get src file size */
    res = fstat(fd_src, &info);
    if (res < 0) {
        LOG("Error fstatting source file %s: %s\n", src, strerror(errno));
    }
    src_size = info.st_size;

    /* read data from file */
    n_read = read(fd_src, buf, src_size);

    if (n_read != src_size) {
        LOG("Error, read: %d, expected: %d, reading from source file %s: %s\n", n_read, src_size, src, strerror(errno));
        return -1;
    }

    /* copy file via creating new file in dst path, first open a directory dst */
    dir = opendir(dst);
    if (dir == NULL) {
        LOG("Error opening dir when copying reg %s: %s\n", dst, strerror(errno));
    }
    drfd = dirfd(dir);
    if (drfd < 0) {
        LOG("Error getting dirfd: %s\n", strerror(errno));
    }

    fd_dst = openat(drfd, name, O_WRONLY | O_CREAT, 0666);
    if (fd_dst < 0) {
        LOG("Error opening dst file %s for writing: %s\n", dst, strerror(errno));
        return -1;
    }

    n_write = write(fd_dst, buf, n_read);
    if (n_write != n_read) {
        LOG("Error writing to dst file %s: %s\n", dst, strerror(errno));
        return -1;
    }

    close(fd_dst);
    close(fd_src);
    return 0;
}

//----------------------------------------------------------------------
void copy_rw(char* src, char* dst, char* name, int type) {

    DIR* dir = NULL;
    struct dirent* entry;
    int res = 0;

    char dst_path[MAX_PATH_SIZE];
    char src_path[MAX_PATH_SIZE];

    snprintf(src_path, sizeof(src_path), "%s", src);
    snprintf(dst_path, sizeof(dst_path), "%s", dst);

    if (type == DT_DIR) {
        /* create a directory */
        //mkdir(dst_path, 0666);
        dir = opendir(src);
        if (dir == NULL) {
            LOG("Error opening dir %s: %s\n", src, strerror(errno));
        }

        entry = readdir(dir);
        if (entry == NULL) {
            LOG("Error reading dir: %s\n", strerror(errno));
        }

        /* Update src_buf and dst_buf so they contain new path */
        snprintf(src_path, sizeof(src_path), "%s/%s", src, entry->d_name);

        LOG("Src and dst paths: %s, %s\n", src_path, dst_path);
        if (entry->d_type == DT_REG) {
            copy_rw(src_path, dst_path, name, DT_REG);
        } else if (entry->d_type == DT_DIR) {
            snprintf(dst_path, sizeof(dst_path), "%s/%s", dst, entry->d_name);
            res = mkdir(dst_path, 0666);
            if (res < 0) {
                LOG("Error creating directory in dest: %s\n", strerror(errno));
            }
            copy_rw(src_path, dst_path, name, DT_DIR);
        }

        //f (entry->d_type == DT_DIR) {
      //  } else if (entry->d_type == DT_REG) {
       // }
        //closedir(dir);
        /* If the entry is another directory, do a recursive step */

    } else if (type == DT_REG) {
        res = copy_reg(src, dst, name);
        if (res < 0) {
            exit(EXIT_FAILURE);
        }
    }

}

//----------------------------------------------------------------------
void change_time(char* dst) {

    char* argv[3];
    argv[0] = "-m";
    argv[1] = dst;
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



//----------------------------------------------------------------------
void init_dest_dir(const char* dst) {
    mkdir(dst, 0777);
    if (errno != EEXIST) {
       LOG("Creating new backup directory: %s\n", strerror(errno));
    }
}

