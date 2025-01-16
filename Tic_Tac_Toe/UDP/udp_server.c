#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 6969
#define BUFFER_SIZE 1024

char board[3][3];
int client1, client2;
int state;
int server_fd; 
char buffer[BUFFER_SIZE];
struct sockaddr_in address, client1_addr, client2_addr;
int addr_len;
char move[10], wait[10];
char empty[10];

int startgame();

void initboard() {
    for(int i = 0;i < 3;i++) {
        for(int j = 0;j < 3;j++) {
            board[i][j] = '.';
        }
    }
    memset(move, 0, 10);
    sprintf(move, "Move");
    memset(wait, 0, 10);
    sprintf(wait, "Wait");
}

int checker() {
    for(int i = 0;i < 3;i++) {
        if (board[i][0] == board[i][1] && board[i][1] == board[i][2]) {
            if (board[i][0] == 'X') {
                return 1;
            } else if (board[i][0] == 'O') {
                return 2;
            }
        }
    }
    for(int i = 0;i < 3;i++) {
        if (board[0][i] == board[1][i] && board[1][i] == board[2][i]) {
            if (board[0][i] == 'X') {
                return 1;
            } else if (board[0][i] == 'O') {
                return 2;
            }
        }
    }
    if (board[0][0] == board[1][1] && board[1][1] == board[2][2]) {
        if (board[0][0] == 'X') {
            return 1;
        } else if (board[0][0] == 'O') {
            return 2;
        }
    }
    if (board[0][2] == board[1][1] && board[1][1] == board[2][0]) {
        if (board[0][2] == 'X') {
            return 1;
        } else if (board[0][2] == 'O') {
            return 2;
        }
    }
    int flag = 0;
    for(int i = 0;i < 3;i++) {
        for(int j = 0;j < 3;j++) {
            if (board[i][j] == '.') {
                flag = 1;
                break;
            }
        }
    }
    if (flag == 0) {
        return 0;
    }
    return -1;
}

void init_server() {
if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    memset(&address, '0', sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    // bind the socket to the address, port
    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Server listening on port %d...\n", PORT);
}

int get_data_from_client(char* buffer, struct sockaddr_in client_addr) {
    int in = recvfrom(server_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &client_addr, &addr_len);
    return in;
}

void append_board_data(char* data) {
    char temp1[BUFFER_SIZE] = "";
    for(int i = 0;i < 3;i++) {
        for(int j = 0;j < 3;j++) {
            char temp[BUFFER_SIZE];
            sprintf(temp, "%c ", board[i][j]);
            strcat(temp1, temp);
        }
    }
    strcpy(data, temp1);
}

void make_string(char* data,char* type) {
    memset(data, 0, BUFFER_SIZE);
    append_board_data(data);
    sprintf(data, "%s | %s\n", data,type);
}

void send_data_to_client_1 (char* data,char* type) {
    make_string(data, type);
    printf("Sending to client 1: %s\n", data);
    sendto(server_fd, data, strlen(data), 0, (struct sockaddr *)&client1_addr, addr_len);
    printf("sent to client 1\n");
    memset(data, 0, BUFFER_SIZE);
}

void send_data_to_client_2 (char* data,char* type) {
    memset(data, 0, BUFFER_SIZE);
    make_string(data, type);
    printf("Sending to client 2: %s\n", data);
    sendto(server_fd, data, strlen(data), 0, (struct sockaddr *)&client2_addr, addr_len);
    printf("sent to client 2\n");
    memset(data, 0, BUFFER_SIZE);
}

void check_end(int ch,char* data) {
    if (ch == 1) {
        printf("Client 1 wins\n");
        sprintf(data, "p1win");
        sendto(server_fd, data, strlen(data), 0, (struct sockaddr *)&client1_addr, addr_len);
        sendto(server_fd, data, strlen(data), 0, (struct sockaddr *)&client2_addr, addr_len);
        memset(data, 0, BUFFER_SIZE);
    }
    else if (ch == 2) {
        printf("Client 2 wins\n");
        sprintf(data, "p2win\n");
        sendto(server_fd, data, strlen(data), 0, (struct sockaddr *)&client1_addr, addr_len);
        sendto(server_fd, data, strlen(data), 0, (struct sockaddr *)&client2_addr, addr_len);
        memset(data, 0, BUFFER_SIZE);
    }
    else {
        printf("Draw\n");
        sprintf(data, "draw");
        sendto(server_fd, data, strlen(data), 0, (struct sockaddr *)&client1_addr, addr_len);
        sendto(server_fd, data, strlen(data), 0, (struct sockaddr *)&client2_addr, addr_len);
        memset(data, 0, BUFFER_SIZE);
    }
}

int end_or_continue(int ch,char* data) {
    int valread;
    if (ch != -1) {
        check_end(ch,data); //
        char temp1[BUFFER_SIZE], temp2[BUFFER_SIZE];
        valread = get_data_from_client(temp1,client1_addr);
        if (valread < 0) {
            perror("Read failed");
            exit(EXIT_FAILURE);
        }
        valread = get_data_from_client(temp2,client2_addr);
        if (valread < 0) {
            perror("Read failed");
            exit(EXIT_FAILURE);
        }
        if (strstr(temp1, "yes") != NULL && strstr(temp2, "yes") != NULL) {
            initboard();
            memset(data, 0, BUFFER_SIZE);
            sprintf(data, "yes\n");
            printf("sending yes to client 1\n");
            sendto(server_fd, data, strlen(data), 0, (struct sockaddr *)&client1_addr, addr_len);
            printf("sending yes to client 2\n");
            sendto(server_fd, data, strlen(data), 0, (struct sockaddr *)&client2_addr, addr_len);
            printf("sent to both\n");
            startgame();
            return 1;
        }
        else {
            char temp[BUFFER_SIZE];
            sprintf(data, "no\n");
            sendto(server_fd, data, strlen(data), 0, (struct sockaddr *)&client1_addr, addr_len);
            sendto(server_fd, data, strlen(data), 0, (struct sockaddr *)&client2_addr, addr_len);
            close(server_fd);
            return 0;
        }
    }
    else {
        return 2;
    }
}

int startgame() {
    int valread;
    char data[BUFFER_SIZE];
    char temp[BUFFER_SIZE];
    // now print board into data
    send_data_to_client_1(data,move);
    send_data_to_client_2(data,wait);
    while(1) {
        valread = get_data_from_client(buffer,client1_addr);
        if (valread < 0) {
            perror("Read failed");
            exit(EXIT_FAILURE);
        }
        printf("Client 1: %s\n", buffer);
        int x,y;
        sscanf(buffer, "%d %d", &x, &y);
        board[x][y] = 'X';
        int ch;
        ch = checker(); // 
        int stat = end_or_continue(ch,data); //
        if (stat != 2) {
            break;
        }
        send_data_to_client_1(data,wait);
        send_data_to_client_2(data,move);
    
        valread = get_data_from_client(buffer,client2_addr);
        if (valread < 0) {
            perror("Read failed");
            exit(EXIT_FAILURE);
        }
        printf("Client 2: %s\n", buffer);
        // update board
        sscanf(buffer, "%d %d", &x, &y);
        board[x][y] = 'O';
        ch = checker();
        stat = end_or_continue(ch,data);
        if (stat != 2) {
            break;
        }
        send_data_to_client_1(data,move);
        send_data_to_client_2(data,wait);
    }
}

int main() {
    initboard();
    addr_len = sizeof(address);
    int valread;
    init_server(&server_fd, &address);

    recvfrom(server_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &client1_addr, &addr_len);
    printf("Client 1 connected.\n");
    recvfrom(server_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &client2_addr, &addr_len);
    printf("Client 2 connected.\n");
    int stat;
    startgame();
    return 0;
}
