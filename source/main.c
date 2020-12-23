
/*  This program launches a daemon which monitors specified directory for any modifications
    and, should any occur or in case no modifications at all have ever been introduced to
    the directory, makes a copy of a directory to another backup directory */


/* These have been defined to use several systems calls not available otherwise */

#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include "lbp.h"

#define MAX_PATH_SIZE 1024


int main(int argc, char** argv) {

    int args_check = 0;

    args_check = check_args(argc, argv);
    if (args_check < 0) {
        exit(EXIT_FAILURE);
    }
    printf("Starting program...\n");
    
    // char src_path[MAX_PATH_SIZE];
    // char dst_path[MAX_PATH_SIZE];

    // realpath(argv[1], src_path);
    // realpath(argv[2], dst_path);

    // int links_behaviour = 0;
    // if (strcmp(argv[3], "-L") == 0) {
    //     links_behaviour = 1;
    // } else if (strcmp(argv[3], "-H") == 0) {
    //     links_behaviour = 0;
    // }


    // printf("Src dir: %s\n", src_path);
    // printf("Dst dir: %s\n", dst_path);
    // printf("Links behaviour: %d\n", links_behaviour);

    // int check = check_dest_dir(src_path, dst_path);
    // int exists = check_source_dir(src_path);
    
    // if (check == 1 || exists == 0) {
    //     exit(EXIT_FAILURE);
    // }
    // init_daemon(src_path, dst_path, links_behaviour);

    return 0;

}