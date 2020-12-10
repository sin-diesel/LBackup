#include "copy.h"

#define MAX_PATH_SIZE 1024


int main(int argc, char** argv) {

    if (argc != 4) {
        printf("usage: ./rescp <src/> <dest/> <links_behaviour> [-L -deref, -H - do not deref ]\n"); 
        exit(-1);
    }

    char src_path[MAX_PATH_SIZE];
    char dst_path[MAX_PATH_SIZE];

    realpath(argv[1], src_path);
    realpath(argv[2], dst_path);

    int links_behaviour = 0;
    if (strcmp(argv[3], "-L") == 0) {
        links_behaviour = 1;
    } else if (strcmp(argv[3], "-H") == 0) {
        links_behaviour = 0;
    }


    printf("Src dir: %s\n", src_path);
    printf("Dst dir: %s\n", dst_path);
    printf("Links behaviour: %d\n", links_behaviour);

    int check = check_dest_dir(src_path, dst_path);
    if (check == 1) {
        exit(EXIT_FAILURE);
    }
    init_daemon(src_path, dst_path, links_behaviour);

    return 0;

}