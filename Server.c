#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUF_SIZE 100
#define MAX_CLNT 2 // Maximum number of clients

int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
pthread_mutex_t mutex;
int current_player = 0; // Player ID: 0 or 1

void *handle_clnt(void *arg);
void send_msg(char *msg, int len);
void error_handling(char *message);

int main(int argc, char *argv[]) {
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    int clnt_adr_sz;
    pthread_t t_id;

    if (argc != 2) {
        printf("Usage: %s <PORT>\n", argv[0]);
        exit(1);
    }

    // Initialize mutex and server socket
    pthread_mutex_init(&mutex, NULL);
    serv_sock = socket(PF_INET, SOCK_STREAM, 0); 

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    // Bind server socket
    if (bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("bind error()");
    if (listen(serv_sock, 5) == -1) // Queue size = 5
        error_handling("listen error()");

    while(1) {
        clnt_adr_sz = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz); 

        pthread_mutex_lock(&mutex);
        clnt_socks[clnt_cnt++] = clnt_sock;
        pthread_mutex_unlock(&mutex);

        pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock);
        pthread_detach(t_id);
    }

    close(serv_sock);
    return 0;
}

void *handle_clnt(void *arg) {
    int clnt_sock = *((int*)arg);
    char msg[BUF_SIZE];

    // Notify client about their player ID
    sprintf(msg, "You are player %d\n", clnt_cnt - 1);
    write(clnt_sock, msg, strlen(msg));

    // Game loop
    while(1) {
        // Only the current player can make a move
        if ((clnt_cnt >= MAX_CLNT) && (clnt_cnt > 0) && (current_player == (clnt_cnt - 1))) {
            // TODO: Implement game flow and broadcasting
            // Receive moves from clients
            // Broadcast game status updates to clients
        } else {
            // Notify other players to wait for their turn
            sprintf(msg, "Waiting for player %d to make a move\n", current_player);
            write(clnt_sock, msg, strlen(msg));
        }
    }

    close(clnt_sock);
    return NULL;
}

void send_msg(char *msg, int len) {
    int i;
    pthread_mutex_lock(&mutex);
    for (i = 0 ; i < clnt_cnt; i++) 
        write(clnt_socks[i], msg, len); // Broadcasting
    pthread_mutex_unlock(&mutex);
}

void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
