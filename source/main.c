
/*  This is a LBackup program (LightBackup), it 
    launches a daemon which monitors specified directory
    for any modifications and, should any occur or in 
    case no modifications at all have ever been introduced to
    the directory, makes a copy of a directory to another backup directory

    IMPORTANT NOTE: the program creates log file in /var/log, therefore it ought
    to be launched under sudo. */


/* These have been defined to use several systems calls not available otherwise */

/* TODO
1) add each log printing time DONE
2) refactor program interface DONE
3) refactor log printing in general for easier understanding DONE 
4) add readme DONE
5) add read/write instead of cp command */

#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include "lbp.h"

#define MAX_PATH_SIZE 1024

#define ERROR_CLR(error)  do {                                          \
                            printf("\033[0;31m"); /* set color to red */ \
                            printf("error: ");                          \
                            printf("\033[0m"); /* reset color */            \
                            printf("%s\n", error);  }                       \
                        while (0);                                      \



int main(int argc, char** argv) {

    int args_check = 0;
    int dir_check = 0;

    char src_path[MAX_PATH_SIZE];
    char dst_path[MAX_PATH_SIZE];

    args_check = check_args(argc, argv);
    if (args_check < 0) {
        exit(EXIT_FAILURE);
    }

    realpath(argv[1], src_path); /* convert path to absolute */
    realpath(argv[2], dst_path);

    printf("Src dir: %s\n", src_path);
    printf("Dst dir: %s\n", dst_path);

    dir_check = check_dest_dir(src_path, dst_path);
    if (dir_check == 1) {
        ERROR_CLR("Backup directory specified as destination\n");
        exit(EXIT_FAILURE);
    }
    
    dir_check = check_source_dir(src_path);
    if (dir_check == -1) {
        ERROR_CLR("Source directory does not exist.\n");
        exit(EXIT_FAILURE);
    }

    init_dest_dir(dst_path);

    printf("Starting program...\n");

    init_daemon(src_path, dst_path, lnk_type);

    run_backup(src_path, dst_path);

    return 0;

}