#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<pthread.h>

#define BUF_SIZE 100
#define MAX_CLNT 256

int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
const char* win_msg = "당신이 이겼습니다\n";
const char* lose_msg = "당신은 졌습니다\n";
const char* turn_msg = "당신의 차례가 아닙니다\n";
int cur_turn = 0;
pthread_mutex_t mutex;

void error_handling(char* message);
void *handle_clnt(void* arg);
void send_msg(char* msg, int len);
void send_msg_to_one(char* msg, int len, int sock_num);
int main(int argc, char* argv[]) {
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    int clnt_adr_sz;
    pthread_t t_id;
    if(argc != 2) {
        printf("Usage %s <PORT>", argv[0]);
    }

    pthread_mutex_init(&mutex, NULL);
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;  //tcpip 통신
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);   //내가 사용할 수 있는 ip 사용
    serv_adr.sin_port = htons(atoi(argv[1]));   //port 저장

    if(bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1){
        error_handling("bind() error");
    }
    if(listen(serv_sock, 5) == -1) {
        error_handling("listen() error");
    }
    while(1) {
        clnt_adr_sz = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
        pthread_mutex_lock(&mutex);
        clnt_socks[clnt_cnt++] = clnt_sock;
        pthread_mutex_unlock(&mutex);
        // 클라이언트에게 클라이언트 번호 보내기
        char clnt_num_msg[BUF_SIZE];
        sprintf(clnt_num_msg, "You are client #%d\n", clnt_cnt);
        write(clnt_sock, clnt_num_msg, strlen(clnt_num_msg));

        pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock);
        pthread_detach(t_id);
        printf("Connected client IP: %s\n", inet_ntoa(clnt_adr.sin_addr));
    }
    close(serv_sock);
    return 0;
}
void *handle_clnt(void* arg) {
    int clnt_sock = *((int*)arg);
    int str_len = 0, i;
    char msg[BUF_SIZE];
    int winner_sock = -1;   //승자
    
    while(winner_sock == -1 && (str_len = read(clnt_sock, msg, sizeof(msg))) != 0) {
        //만약 자신의 차례가 아니면 client에 알려줌
        if(clnt_sock != clnt_socks[cur_turn]) {
            send_msg_to_one(turn_msg, BUF_SIZE, clnt_sock);
            continue;
        }
        if(msg[str_len-2] == 'E') { //승자가 끝났음을 알림
            msg[str_len-3] = '\n';  //E표식을 없앰
            msg[str_len-2] = 0;
            str_len -= 2;
            winner_sock = clnt_sock;
        }
        send_msg(msg, str_len);//i행 j열
        //순서를 넘김
        if(cur_turn == 0) cur_turn = 1;
        else cur_turn = 0;
    }
    //승자가 정해짐
    sleep(1);
    if(winner_sock != -1) {
        send_msg_to_one(win_msg, BUF_SIZE, winner_sock);
        if(clnt_socks[0] == clnt_sock) {
            send_msg_to_one(lose_msg, BUF_SIZE, clnt_socks[1]);
        } else {
            send_msg_to_one(lose_msg, BUF_SIZE, clnt_socks[0]);
        }
    }

    pthread_mutex_lock(&mutex); //client 중간에 이탈할 수 있음!
    for(i = 0; i < clnt_cnt; i++) {
        if(clnt_sock == clnt_socks[i]) {
            while(i++ < clnt_cnt-1) {
                clnt_socks[i] = clnt_socks[i+1];
            }
            break;
        }
    }
    clnt_cnt--;
    if (clnt_cnt == 0) cur_turn=0;
    pthread_mutex_unlock(&mutex);
    close(clnt_sock);
    return NULL;
}
//client 1, 2에게 메시지를 다 보냄
void send_msg(char* msg, int len) {
    int i;
    pthread_mutex_lock(&mutex);
    for(i=0;i<clnt_cnt;i++){    //접속 client가 같은 메시지 열람
        write(clnt_socks[i], msg, len); 
    }
    pthread_mutex_unlock(&mutex);
} 
void error_handling(char* message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
//특정 client에게 메시지를 보냄
void send_msg_to_one(char* msg, int len, int sock_num) {
    pthread_mutex_lock(&mutex);
    write(sock_num, msg, len);
    pthread_mutex_unlock(&mutex);
}
