
 #define _XOPEN_SOURCE 600


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <sys/wait.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <sys/ipc.h>
#include <dirent.h>

#define KILL 0
#define PRINT 1
#define SET 2

#define MAX_PATH_SIZE 1024



int main (int argc, char** argv) {
    if (argc < 2 || (argc > 2 && argc != 4) || (argc == 2 && (strcmp(argv[1], "-set") == 0))) {
        printf("usage: ./rc -s - stop daemon, -p - print logs, -set <src> <dst> \
                        - set new backup source and destination\n"); \
        exit(EXIT_FAILURE);
    }

    char* option = argv[1];

    int resop = 0;
    
    char * myfifo = "/tmp/reserv_fifo"; 
    resop = mkfifo(myfifo, O_CREAT | 0666);
    if (resop < 0) {
        printf("FIFO init error: %s\n", strerror(errno));
    }

    if (strcmp(option, "-stop") == 0) {
        int fd_fifo = open(myfifo, O_WRONLY);
        if (fd_fifo < 0) {
            printf("Error opening FIFO: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        int data = KILL;
        int n_write = write(fd_fifo, &data, sizeof(int));
        if (n_write != sizeof(int)) {
            printf("Error writing to FIFO: %s\n", strerror(errno));
        }
        close(fd_fifo);

    } else if (strcmp(option, "-print") == 0) {

        int fd_fifo = open(myfifo, O_WRONLY);
        if (fd_fifo < 0) {
            printf("Error opening FIFO: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        int data = PRINT;
        int n_write = write(fd_fifo, &data, sizeof(int));
        if (n_write != sizeof(int)) {
            printf("Error writing command to FIFO: %s\n", strerror(errno));
        }
    

        char buf[BUFSIZ];
        getcwd(buf, BUFSIZ);
        
        n_write = write(fd_fifo, buf, MAX_PATH_SIZE);
        if (n_write < 1) {
             printf("Error writing to FIFO: %s\n", strerror(errno));
        }

        close(fd_fifo);
    } else if (strcmp(option, "-set") == 0) {

        int fd_fifo = open(myfifo, O_WRONLY);
        if (fd_fifo < 0) {
            printf("Error opening FIFO: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        // write command number to fifo
        int data = SET;
        int n_write = write(fd_fifo, &data, sizeof(int));
        if (n_write != sizeof(int)) {
            printf("Error writing command to FIFO: %s\n", strerror(errno));
        }

        // copy src and dst paths to larger buffers
        char src_buf[MAX_PATH_SIZE];
        char dst_buf[MAX_PATH_SIZE];


        char* src = argv[2];
        char* dst = argv[3];

        realpath(src, src_buf);
        realpath(dst, dst_buf);

        
        n_write = write(fd_fifo, src_buf, MAX_PATH_SIZE);
        if (n_write < 1) {
             printf("Error writing to FIFO: %s\n", strerror(errno));
        }


         n_write = write(fd_fifo, dst_buf, MAX_PATH_SIZE);
        if (n_write < 1) {
             printf("Error writing to FIFO: %s\n", strerror(errno));
        }

        close(fd_fifo);
    } else {
        printf("usage: ./rc -s - stop daemon, -p - print logs, -set <src> <dst> \
                        - set new backup source and destination\n"); \
        exit(EXIT_FAILURE);
    }

    return 0;
}