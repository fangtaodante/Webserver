#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "server_epoll.h"


int main(int argc, const char*argv[]) {
    //启动epoll模型
    if(argc<3){
        printf("Eg: ./a.out port path\n");
        exit(1);
    }

    int port = atoi(argv[1]);
    int ret = chdir(argv[2]);
    if (ret == -1) {
        perror("chdir error");
        exit(1);
    }

    epoll_run(port);
    return 0;

}