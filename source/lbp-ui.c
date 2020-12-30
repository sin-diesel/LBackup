
#define _XOPEN_SOURCE 600


#include "lbp.h"

#define MAX_PATH_SIZE 1024


static const char myfifo[] = "/tmp/lbp_fifo";

int check_arguments(int argc, char** argv) {
    if (argc < 2) {
        printf("usage: ./lbp-ui --stop - stop daemon, --print - print logs, --set <src> <dst> \
                        - set new backup source and destination\n");
        return -1;
    } else if (argc == 2) {
        if (strcmp(argv[1], "--set") == 0) {
            printf("usage: ./lbp-ui --stop - stop daemon, --print - print logs, --set <src> <dst> \
                        - set new backup source and destination\n"); 
            return -1;
        }
    } else if (argc == 4) {
        if (strcmp(argv[1], "--set") != 0) {
            printf("usage: ./lbp-ui --stop - stop daemon, --print - print logs, --set <src> <dst> \
                        - set new backup source and destination\n"); 
            return -1;
        } else {
            return 0;
        }
    } else {
        printf("usage: ./lbp-ui --stop - stop daemon, --print - print logs, --set <src> <dst> \
                        - set new backup source and destination\n"); 
        return -1;
    }
    return 0;
}


int main (int argc, char** argv) {

    int fd_fifo = -1;
    char* option = argv[1];
    int resop = 0;
    int n_write = 0;
    int valid_args = 0;
    int data = -1; /* This value is passed to fifo so that daemon knows what to do */

    valid_args = check_arguments(argc, argv);
    if (valid_args < 0) {
        exit(EXIT_FAILURE);
    }
    /* open fifo for communication with daemon */
    resop = mkfifo(myfifo, O_CREAT | 0666);
    if (resop < 0) {
        printf("FIFO init error: %s\n", strerror(errno));
    }

    fd_fifo = open(myfifo, O_WRONLY);
    if (fd_fifo < 0) {
        printf("Error opening FIFO: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* determine which option was passed to program */
    if (strcmp(option, "--stop") == 0) {
        data = STOP;
        n_write = write(fd_fifo, &data, sizeof(int));
        if (n_write != sizeof(int)) {
            printf("Error writing to FIFO: %s\n", strerror(errno));
        }

    } else if (strcmp(option, "--print") == 0) {
        data = PRINT;
        n_write = write(fd_fifo, &data, sizeof(int));
        if (n_write != sizeof(int)) {
            printf("Error writing command to FIFO: %s\n", strerror(errno));
        }

        char buf[BUFSIZ];
        getcwd(buf, BUFSIZ);
        
        n_write = write(fd_fifo, buf, MAX_PATH_SIZE);
        if (n_write < 1) {
             printf("Error writing to FIFO: %s\n", strerror(errno));
        }

    } else if (strcmp(option, "--set") == 0) {


        /* write command number to fifo */
        data = SET;
        n_write = write(fd_fifo, &data, sizeof(int));
        if (n_write != sizeof(int)) {
            printf("Error writing command to FIFO: %s\n", strerror(errno));
        }

        /* copy src and dst paths to larger buffers */
        char src_buf[MAX_PATH_SIZE];
        char dst_buf[MAX_PATH_SIZE];

        /* add error checking */
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

    } else {
        printf("usage: ./lbp-ui --stop - stop daemon, --print - print logs, --set <src> <dst> \
                        - set new backup source and destination\n"); 
        close(fd_fifo);
        exit(EXIT_FAILURE);
    }
    close(fd_fifo);

    return 0;
}