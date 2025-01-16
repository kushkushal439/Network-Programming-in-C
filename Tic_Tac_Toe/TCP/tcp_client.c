#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 6969
#define SERVER_ADDRESS "127.0.0.1" // Loopback address for local testing
#define BUFFER_SIZE 1024

char board[3][3];


void initboard() {
    for(int i = 0;i < 3;i++) {
        for(int j = 0;j < 3;j++) {
            board[i][j] = ' ';
        }
    }
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER_ADDRESS, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }
    int f = 0;
    while(1) {
        int stat = 0;
        memset(buffer, 0, BUFFER_SIZE);
        stat = recv(sock, buffer, BUFFER_SIZE,0);
        // else f = 0;
        if (stat < 0) {
            perror("Read failed");
            exit(EXIT_FAILURE);
        }
        int check = 0;
        if (strstr(buffer,"p1win") != NULL) {
            printf("Player 1 wins\n");
            check = 1;
        }
        if (strstr(buffer,"p2win") != NULL) {
            printf("Player 2 wins\n");
            check = 1;
        }
        if (strstr(buffer,"draw") != NULL) {
            printf("Draw\n");
            check = 1;
        }
        if (check == 1) {
            printf("Would you like to play again? (yes/no): ");
            char data[BUFFER_SIZE];
            scanf("%s", data);
            send(sock, data, strlen(data), 0);
            memset(buffer, 0, BUFFER_SIZE);
            int valread = recv(sock, buffer, BUFFER_SIZE,0);
            if (valread < 0) {
                perror("Read failed");
                exit(EXIT_FAILURE);
            }
            if (strstr(buffer, "yes") != NULL) {
                initboard();
                memset(buffer, 0, BUFFER_SIZE);
                // stat = recv(sock, buffer, BUFFER_SIZE,0);
                f = 1;
                printf("Game restarted !\n");
                sprintf(buffer, "ok");
                send(sock, buffer, strlen(buffer), 0);
                continue;
            }
            else {
                printf("Game over\n");
                break;
            }
        }   
        char boardin[3][3];
        int iter = 0;
        for(int i = 0;i < 3;i++) {
            for(int j = 0;j < 3;j++) {
                char c = '-';
                while(c == '-') {
                    c = buffer[iter++];
                    if (!(c == 'X' || c == 'O' || c == '.')) {
                        c = '-';
                    }
                }
                boardin[i][j] = c;
            }
        }
        for(int i = 0;i < 3;i++) {
            for(int j = 0;j < 3;j++) {
                printf("%c ", boardin[i][j]);
            }
            printf("\n");
        }
        if (strstr(buffer, "Move") == NULL) {
            printf("Waiting for opponents move.....\n");
        }
        else {
            printf("Enter your move: ");
            int valid = 0;
            int x,y;
            while(valid == 0) {
                scanf("%d %d", &x, &y);
                if (x >= 3 || y >= 3 || x < 0 || y < 0) {
                    printf("Invalid move, try again: ");
                }
                else if (boardin[x][y] != '.') {
                    printf("Invalid move, try again: ");
                }
                else {
                    valid = 1;
                }
            }
            char data[BUFFER_SIZE];
            sprintf(data, "%d %d", x, y);
            send(sock, data, strlen(data), 0);
        }
    }
    close(sock);

    return 0;
}
