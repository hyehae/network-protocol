#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
//#include <WinSock2.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<pthread.h>

//#pragma comment (lib, "ws2_32.lib")   //-lws2_32

#define SIZE 5
#define BUF_SIZE 100
pthread_mutex_t mutex;
char board[SIZE][SIZE];

const char* win_msg = "당신이 이겼습니다.";
const char* lose_msg = "당신이 졌습니다.";
const char* turn_msg = "당신의 차례가 아닙니다.";

void error_handling(char *message);
void display_board();
int is_valid_input(int row, int col);
int check_win();
void * send_msg(void* arg);
void * recv_msg(void* arg);


int main(int argc, char * argv[]) {
    char msg[BUF_SIZE];
    char input[BUF_SIZE];
    char * win_msg = "Win1";
    int x, y;
    int sd;
    struct sockaddr_in serv_adr;
    pthread_t snd_thread, rcv_thread;  //보내는 거, 받는 거

    void * thread_return;

    /**
    WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		error_handling("WSAStartup() error");
    }
    **/

    if (argc!=3){
        printf("Usage %s <IP> <PORT>\n", argv[0]);
        exit(1);
    }

    if ((sd=socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("fail_socket\n");
        exit(EXIT_FAILURE);
    }

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family=AF_INET;
    serv_adr.sin_addr.s_addr=inet_addr(argv[1]);
    serv_adr.sin_port=htons(atoi(argv[2]));

    if (connect(sd, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) {
        error_handling("connect() error");
    }

    memset(board, ' ', sizeof(board));
    // Game loop
    pthread_create(&snd_thread, NULL, send_msg, (void*)&sd);
    pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sd);
    pthread_join(snd_thread, &thread_return);
    pthread_join(rcv_thread, &thread_return);

    //WSACleanup();
    close(sd);
    return 0;
}

void * send_msg(void* arg) {  //내 차례 (-> 입력받은 좌표 서버에게 보냄)
    int sd =*((int*)arg);
    char input[BUF_SIZE];
    int x, y;

    while(1) {
        printf("Enter row and col: ");
        scanf("%d %d", &x, &y);

        pthread_mutex_lock(&mutex);
        if (!is_valid_input(x, y)) {
            board[x][y] = 'O';
            if (check_win()) {
                write(sd, win_msg, strlen(win_msg));  //이긴 경우, win_msg 보내기
                break;
            } else {
                sprintf(input, "%d %d", x, y); //입력을 서버에 보내기
                write(sd, input, strlen(input));
            }
        }
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

void * recv_msg(void* arg) {
    int sd =*((int*)arg);
    char msg[BUF_SIZE];
    int x, y;

    while(1) {  //상대방 차례 입력 (-> 서버에서 보냄), 
        if (read(sd, msg, BUF_SIZE) != 0) {
            pthread_mutex_lock(&mutex);
            sscanf(msg, "%d %d", &x, &y);
            if(!is_valid_input(x, y)) {
                board[x][y] = 'X';
            }
            //입력할 때, check_win하므로 받는 부분에서는 생략
        }
    }
}
    

void display_board() {
    printf("\n");
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            printf(" %c ", board[i][j]);
            if (j != SIZE - 1) printf("|");
        }
        
        printf("\n");
        if (i != SIZE - 1) {
            for (int k = 0; k < SIZE; k++) {
                printf("--");
                if (k != SIZE - 1) printf("|");
            }
            printf("\n");
        }
    }
    printf("\n");
}

int is_valid_input(int row, int col) {
    if (row < 0 && row >= 5 && col < 0 && col >= 5) {
        printf("out of range\n");
        return 1;
    }

    else if (board[row][col] == 'O' || board[row][col] == 'X') {
        printf("already checked!\n");
        return 1;
    }

    else return 0;
}

int check_win() {
    int row, col;
    int diag1 = 0, diag2 = 0;

    for (row = 0; row < SIZE; row++) {
        int row_count = 0, col_count = 0;
        for (col = 0; col < SIZE; col++) {
            if (board[row][col] == 'O') row_count++;
            if (board[col][row] == 'O') col_count++;
        }
        if (row_count == SIZE || col_count == SIZE) return 1;
        if (board[row][row] == 'O') diag1++;
        if (board[row][SIZE - 1 - row] == 'O') diag2++;
    }
    if (diag1 == SIZE || diag2 == SIZE) return 1;

    return 0;

}


void error_handling(char *message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}