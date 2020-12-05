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

    // char* src_full_name = getcwd(src_path, MAX_PATH_SIZE);
    // char* dst_full_name = getcwd(dst_path, MAX_PATH_SIZE);

    // if (src_full_name == NULL || dst_full_name == NULL) {
    //     printf("Error setting full path\n");    
    // }

    // printf("Src working dir: %s\n", src_path);
    // printf("Dst working dir: %s\n", dst_path);

    // strcat(src_path, argv[1]);
    // strcat(dst_path, argv[2]);

    printf("Src dir: %s\n", src_path);
    printf("Dst dir: %s\n", dst_path);

    init_daemon(src_path, dst_path);

    int initial_indent = 1;

   // init_dest_dir(dst_name);
   // traverse(src_name, dst_name, initial_indent);

    return 0;

}