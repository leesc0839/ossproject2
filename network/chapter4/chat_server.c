#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 30

void error_handling(char *message);

void read_childproc(int sig);

void read_routine(int serv_sock, int clnt_sock, char *buf);

void write_routine(int sock, char *buf);

int flag = 0;

int main(int argc, char *argv[]) {
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;

    pid_t pid;
    struct sigaction act;
    socklen_t adr_sz;
    int str_len, state;
    char buf[BUF_SIZE];
    if (argc != 2) {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    act.sa_handler = read_childproc;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    state = sigaction(SIGCHLD, &act, 0);
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *) &serv_adr, sizeof(serv_adr)) == -1)
        error_handling("bind() error");
    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    for (int i = 0; i < 5; i++) {
        adr_sz = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr *) &clnt_adr, &adr_sz);
        if (clnt_sock == -1)
            continue;
        else
            puts("new client connected...");
        pid = fork();
        if (pid == -1) {
            close(clnt_sock);
            continue;
        }
        if (pid == 0)
            read_routine(serv_sock, clnt_sock, buf);
        else
            write_routine(clnt_sock, buf);

        close(clnt_sock);
    }
    close(serv_sock);
    return 0;
}

void read_routine(int serv_sock, int clnt_sock, char *buf) {
    close(serv_sock);

    int str_len;
    while ((str_len = read(clnt_sock, buf, BUF_SIZE)) != 0){
        buf[str_len] = 0;
        printf("client sent : %s\n", buf);
    }

    close(clnt_sock);
    puts("client disconnected...");
    exit(0);
}

void write_routine(int clnt_sock, char *buf) {
    while (1) {
        if (flag) {
            flag = 0;
            return;
        }
        fgets(buf, BUF_SIZE, stdin);
        write(clnt_sock, buf, strlen(buf));
    }
}

void read_childproc(int sig) {
    pid_t pid;
    int status;
    pid = waitpid(-1, &status, WNOHANG);
    flag = 1;
    printf("removed proc id: %d \n", pid);

}

void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}