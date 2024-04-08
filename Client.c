#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include <WinSock2.h>
//#include<arpa/inet.h>
//#include<sys/socket.h>
#include<pthread.h>

#pragma comment (lib, "ws2_32.lib")   //-lws2_32

#define SIZE 5
#define BUF_SIZE 50
char board[SIZE][SIZE];

void error_handling(char *message);
void display_board();
int is_valid_input(int row, int col);
int check_win();


int main(int argc, char * argv[]) {
    char msg[BUF_SIZE];
    char input[BUF_SIZE];
    char * win_msg = "Win1";
    int x, y;
    int sd;
    struct sockaddr_in serv_adr;

    WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		error_handling("WSAStartup() error");
    }

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
    while (1) {
        printf("Game start!\n");
        display_board();

        if(recv(sd, msg, BUF_SIZE, 0) != 0) {
            if (msg == "your turn") {
                printf("Enter row and col: ");
                scanf("%d %d", &x, &y);
                if (!is_valid_input(x, y)){
                    board[x][y] = 'O';
                    if (check_win()) {
                        send(sd, win_msg, strlen(win_msg), 0);
                        break;
                    } else {
                        sprintf(input, "%d %d", x, y);
                        send(sd, input, strlen(input), 0);
                    }
                }
            }

            else {
                sscanf(msg, "%d %d", &x, &y);
                if (!is_valid_input(x, y)) board[x][y] = 'X';
            }
        }
    }

    WSACleanup();
    close(sd);
    return 0;
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