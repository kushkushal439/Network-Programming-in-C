#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <time.h>
#include <netinet/in.h>

#define PORT 6969
#define BUFFER_SIZE 9192
#define BIG_BUFFER_SIZE 1000000
#define CHUNK_SIZE 5

int client;
int server_fd; 
char buffer[BUFFER_SIZE];
struct sockaddr_in address, client_addr;
int addr_len;

#define int long long

typedef struct packet {
    char data[CHUNK_SIZE+1];   // data
    int seq_no;      // seq_id
    int size;        // size of data in packet
    int last_packet; // time in ms
    int done;        // to denote end
} packet;



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


void send_data_to_client (char* data,char* type) {
    memset(data, 0, BUFFER_SIZE);
    printf("Sending to client: %s\n", data);
    sendto(server_fd, data, strlen(data), 0, (struct sockaddr *)&client_addr, addr_len);
    printf("sent to client\n");
    memset(data, 0, BUFFER_SIZE);
}

int get_time_in_ms() {
    struct timeval time_now;
    gettimeofday(&time_now, NULL);
    return time_now.tv_sec * 1000 + time_now.tv_usec / 1000;
}


int send_data() {
    char data1[BUFFER_SIZE];
    // fgets(data1, BUFFER_SIZE, stdin);
    scanf("%s", data1);
    int len = strlen(data1);
    printf("{status} data1 is %s\n", data1);
    int chunks = len / CHUNK_SIZE;
    if (chunks * CHUNK_SIZE < len) {
        chunks++;
    }
    int ack[chunks];
    for (int i = 0;i < chunks;i++) ack[i] = 0;
    int times[chunks];
    for(int i = 0;i < chunks;i++) times[i] = 0;
    packet *p[chunks];
    int seq_no = 0;
    for(int i = 0;i < chunks;i++) {
        p[i] = (packet *)malloc(sizeof(packet));
        p[i]->seq_no = seq_no;
        p[i]->size = CHUNK_SIZE;
        p[i]->last_packet = get_time_in_ms();
        times[i] = p[i]->last_packet;
        if (i == chunks - 1) {
            p[i]->size = len - i * CHUNK_SIZE;
        }
        strncpy(p[i]->data, data1 + i * CHUNK_SIZE, p[i]->size);
        p[i]->data[p[i]->size] = '\0';
        int tot = p[i]->size;
        p[i]->done = 0;
        p[i]->size = chunks;
        sendto(server_fd, p[i], sizeof(*p[i]),MSG_DONTWAIT , (struct sockaddr *)&client_addr, addr_len);
        printf("{status} sending %lld : %s at %lld and size is %lld and actual size = %lld \n", p[i]->seq_no,p[i]->data, p[i]->last_packet, p[i]->size,tot);
        seq_no++;
        // sleep(1);
    }
    while(1) {
        int stat = recvfrom(server_fd, buffer, BUFFER_SIZE, MSG_DONTWAIT, (struct sockaddr *) &client_addr, &addr_len); 
        for(int j = 0;j < chunks;j++) {
            stat = recvfrom(server_fd, buffer, BUFFER_SIZE, MSG_DONTWAIT, (struct sockaddr *) &client_addr, &addr_len); 
            if (stat < 0) continue;;
            printf("{status} Recieved ack: %s\n", buffer);
            int ind = atoi(buffer);
            ack[ind] = 1;
        }
        int cnt = 0;
        for(int i = 0;i < chunks;i++) {
            if (ack[i] == 1) {
                cnt++;
                continue;
            }
            int timenow = get_time_in_ms();
            if (timenow - times[i] > 200) {
                sendto(server_fd, p[i], sizeof(*p[i]), MSG_DONTWAIT, (struct sockaddr *)&client_addr, addr_len);
                printf("{status} retransmitting %lld : %s at %lld and size is %lld \n", p[i]->seq_no,p[i]->data, p[i]->last_packet, p[i]->size);
                times[i] = get_time_in_ms();
            }
        }
        if (cnt == chunks) break;
    }
    packet* done = (packet *)malloc(sizeof(packet));
    done->done = 1;
    sendto(server_fd, done, sizeof(*done), MSG_DONTWAIT, (struct sockaddr *)&client_addr, addr_len);
    return 1;
}

void update_data(packet* init,char* data[],int got[]) {
    got[init->seq_no] = 1;
    data[init->seq_no] = (char*)malloc(sizeof(init->data)+1);
    strcpy(data[init->seq_no],init->data);
}

int recieve_data() {
    int stat = -1;
    packet* init = (packet *)malloc(sizeof(packet));
    while(stat < 0) {
        stat = recvfrom(server_fd, init, sizeof(packet), MSG_DONTWAIT, (struct sockaddr *) &client_addr, &addr_len);
    }
    printf("{status} recieved %lld : %s at %lld and size is %lld \n", init->seq_no,init->data, init->last_packet, init->size);
    sprintf(buffer, "%lld", init->seq_no);
    sendto(server_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, addr_len);
    int cnt = 0;
    int chunks = init->size;
    int got[chunks];
    for(int i = 0;i < chunks;i++) {
        got[i] = 0;
    }
    got[init->seq_no] = 1;
    char data[chunks][CHUNK_SIZE+1];
    strcpy(data[init->seq_no],init->data);
    cnt++;
    int it = 0;
    sprintf(buffer, "%lld", init->seq_no);
    sendto(server_fd, buffer, sizeof(buffer), MSG_DONTWAIT, (struct sockaddr *)&client_addr, addr_len);
    while(1) {
        it++;
        stat = recvfrom(server_fd, init, sizeof(packet), MSG_DONTWAIT, (struct sockaddr *) &client_addr, &addr_len);
        if (stat < 0 ) continue;
        if (init->done == 1) break;
        // if (it % 5 == 0) continue;
        printf("{status} recieved %lld : %s at %lld and size is %lld \n", init->seq_no,init->data, init->last_packet, init->size);
        got[init->seq_no] = 1;
        strcpy(data[init->seq_no],init->data);
        sprintf(buffer, "%lld", init->seq_no);
        sendto(server_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, addr_len);
        printf("{status} sent ack %s to server\n",buffer);
    }
    char final[BIG_BUFFER_SIZE] = "";
    for(int i = 0;i < chunks;i++) {
        strcat(final,data[i]);
    }
    int finlen = strlen(final);
    final[finlen] = '\0';
    printf("\n\nreceived \n%s \nfrom server\n\n", final);
}


signed main() {
    addr_len = sizeof(address);
    int valread;
    init_server(&server_fd, &address);
    char ip[BUFFER_SIZE];
    recvfrom(server_fd, ip, BUFFER_SIZE, 0, (struct sockaddr *) &client_addr, &addr_len);
    printf("recieved ip = %s\n", ip);
    for(;;) {
        printf("\nEnter your message\n");
        send_data();
        printf("\nSent data, waiting for other person to send\n");
        recieve_data();
    }
    return 0;
}
