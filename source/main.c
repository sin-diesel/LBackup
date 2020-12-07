#include "copy.h"

#define MAX_PATH_SIZE 1024


int main(int argc, char** argv) {

    if (argc != 3) {
        printf("usage: ./rescp <src/> <dest/>\n");
        exit(-1);
    }

    char src_path[MAX_PATH_SIZE];
    char dst_path[MAX_PATH_SIZE];

    realpath(argv[1], src_path);
    realpath(argv[2], dst_path);


    printf("Src dir: %s\n", src_path);
    printf("Dst dir: %s\n", dst_path);

    init_daemon(src_path, dst_path);

    return 0;

}