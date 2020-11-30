#include "copy.h"

int main(int argc, char** argv) {
    
    if (argc != 3) {
        printf("usage: ./rescp <src/> <dest/>\n");
        exit(-1);
    }

    char* src_name = argv[1];
    char* dst_name = argv[2];

    int initial_indent = 1;

    traverse(src_name, dst_name, initial_indent);

    return 0;

}