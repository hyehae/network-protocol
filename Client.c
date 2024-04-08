#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#include<pthread.h>

#define BUF_SIZE 100
#define NAME_SIZE 20

char name[NAME_SIZE] = "[DEFAULT]";
char msg[BUF_SIZE];
char map[5][5];
char mymark = ' ';
char yourmark = ' ';
void error_handling(char* message);
void* send_msg(void* arg);
void* rcv_msg(void* arg);
void print_map();
int made_line(int r, int c);

int main(int argc, char* argv[]) {
    int sock;
    struct sockaddr_in serv_addr;
    pthread_t snd_thread, rcv_thread;

    void* thread_return;
    if(argc != 4) {
        printf("Usage: %s <IP> <PORT> <NAME>\n", argv[0]);
        exit(1);
    }
    sprintf(name, "[%s]", argv[3]);

    sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        error_handling("connect() error!");
    }
    //내 순서를 서버에서 부여받음
    int turn = 0;
    // 서버로부터 클라이언트 번호 받기
    char clnt_num_msg[BUF_SIZE];
    read(sock, clnt_num_msg, BUF_SIZE);
    printf("%s", clnt_num_msg); // 클라이언트 번호 출력
    sscanf(clnt_num_msg, "You are client #%d\n", &turn); // 클라이언트 번호 저장
    //mark 부여
    if(turn == 1) {
        mymark = 'o';
        yourmark = 'x';
    } else {
        mymark = 'x';
        yourmark = 'o';
    }
    
    //빙고판 초기화
    for(int i=0;i<5;i++) {
        for(int j=0;j<5;j++) {
            map[i][j] = '.';
        }
    }
    pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);
    pthread_create(&rcv_thread, NULL, rcv_msg, (void*)&sock);
    pthread_join(snd_thread, &thread_return);
    pthread_join(rcv_thread, &thread_return);
    close(sock);
}
void* send_msg(void* arg) {
    int sock = *((int*)arg);
    char name_msg[NAME_SIZE + BUF_SIZE];

    while(1) {
        printf("표시할 행, 열을 입력하시오(0~4) ex)0 4\n");
        fgets(msg, BUF_SIZE, stdin);    //msg : 0 1
        //입력형식 잘못됨
        int r = 0, c = 0;
        if(sscanf(msg, "%d %d\n", &r, &c) != 2) {
            printf("입력 형식이 잘못되었습니다.\n");
            continue;
        }
        sprintf(name_msg, "%s %s", name, msg);
        //TODO: 내 차례에 올바르게 했을 때만 map을 채움..  
        // map[r][c] = mymark;

        if(made_line(r, c)) {   //내가 이겼음 (선이 먼저 만들어졌음)
            //만약 끝났다면 0 1 E 아니면 0 1 형태로 server에 보고
            int len = strlen(name_msg);
            name_msg[len + 1] = 'E';
        }
        write(sock, name_msg, strlen(name_msg));    //server에 write
    }
    return NULL;
}
void* rcv_msg(void* arg) {
    int sock = *((int*)arg);
    char name_msg[NAME_SIZE + BUF_SIZE];

    int str_len;
    while(1) {
        str_len = read(sock, name_msg, NAME_SIZE + BUF_SIZE - 1);
        if(str_len == -1) {
            return (void*) -1;  //read 끝남
        }
        //서버에서 에러메시지를 받은 것임
        if(!strcmp(name_msg, "당신의 차례가 아닙니다.")) {
            fputs(name_msg, stdout);
            continue;
        }

        //서버에서 결과를 부여받음
        if(!strcmp(name_msg, "당신이 졌습니다.") || !strcmp(name_msg, "당신이 이겼습니다.")) {
            fputs(name_msg, stdout);
            close(sock);
            exit(0);
        }
        name_msg[str_len] = 0;
        fputs(name_msg, stdout);
        
        //빙고를 채움
        int r = name_msg[str_len - 4] - '0';
        int c = name_msg[str_len - 2] - '0';
        if(!strncmp(name, name_msg, strlen(name))) {  
            map[r][c] = mymark;
        } else {
            map[r][c] = yourmark;
        }
        //변한 틱택토판 출력
        print_map();
    }
    return NULL;
}
void error_handling(char* message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
int made_line(int r, int c) {
    //가로선, 세로선
    if(map[0][c] == mymark && map[1][c] == mymark && map[2][c] == mymark && map[3][c] == mymark && map[4][c] == mymark) return 1;
    if(map[r][0] == mymark && map[r][1] == mymark && map[r][2] == mymark && map[r][3] == mymark && map[r][4] == mymark) return 1;
    //대각선
    if(r == c){
        if(map[0][0] == mymark && map[1][1] == mymark && map[2][2] == mymark && map[3][3] == mymark && map[4][4] == mymark) return 1;
    }
    if(r + c == 4) {
        if(map[0][4] == mymark && map[1][3] == mymark && map[2][2] == mymark && map[3][1] == mymark && map[4][0] == mymark) return 1;
    }
    return 0;
}
void print_map() {
    for(int i=0;i<5;i++) {
        for(int j=0;j<5;j++) {
            printf("%c ", map[i][j]);
        }
        printf("\n");
    }
}