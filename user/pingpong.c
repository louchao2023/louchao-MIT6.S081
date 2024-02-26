#include "kernel/types.h"
#include "user/user.h"

#define RD 0 //pipe的read端
#define WR 1 //pipe的write端

int main(int argc, char const *argv[]) {
    char buf = 'P'; //用于传送的字节

    int cp[2]; //子进程->父进程
    int pc[2]; //父进程->子进程
    pipe(cp);
    pipe(pc);

    int pid = fork();
    int exit_status = 0;

    if (pid < 0) {
        fprintf(2, "fork() error!\n");
        close(cp[RD]);
        close(cp[WR]);
        close(pc[RD]);
        close(pc[WR]);
        exit(1);
    } else if (pid == 0) { //子进程
        close(pc[WR]);
        close(cp[RD]);

        if (read(pc[RD], &buf, sizeof(char)) != sizeof(char)) {
            fprintf(2, "child read() error!\n");
            exit_status = 1; //标记出错
        } else {
            fprintf(1, "%d: received ping\n", getpid());
        }

        if (write(cp[WR], &buf, sizeof(char)) != sizeof(char)) {
            fprintf(2, "child write() error!\n");
            exit_status = 1;
        }

        close(pc[RD]);
        close(cp[WR]);

        exit(exit_status);
    } else { //父进程
        close(pc[RD]);
        close(cp[WR]);

        if (write(pc[WR], &buf, sizeof(char)) != sizeof(char)) {
            fprintf(2, "parent write() error!\n");
            exit_status = 1;
        }

        if (read(cp[RD], &buf, sizeof(char)) != sizeof(char)) {
            fprintf(2, "parent read() error!\n");
            exit_status = 1; //标记出错
        } else {
            fprintf(1, "%d: received pong\n", getpid());
        }

        close(pc[WR]);
        close(cp[RD]);

        exit(exit_status);
    }
}

