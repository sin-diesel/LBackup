#include "copy.h"

int main(int argc, char** argv) {
    
    if (argc != 3) {
        printf("usage: ./rescp <src/> <dest/>\n");
        exit(-1);
    }

    char* src_name = argv[1];
    char* dst_name = argv[2];

    // DIR* src = opendir(src_name);
    // if (src == NULL) {
    //     perror("Error opening src directory: ");
    // }

    // DIR* dst = opendir(src_name);
    // if (dst == NULL) {
    //     perror("Error opening dst directory: ");
    // }

    traverse(src_name, 1, dst_name);
    //traverse(dst_name, 1);


    // closedir(src);
    // closedir(dst);

    return 0;

}