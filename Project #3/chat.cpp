#include <iostream>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fstream>

using namespace std;

pthread_t pid[15], udp_pid;
vector<bool> online;
vector<string> chat_record;
vector<string> sys_record;
bool closed_server=false;
string leave_room = "---Close Server---";

void *UDP_Broadcast(void *parameter){
    int owner = *((int *)parameter);
    int sys_num=0, chat_num=0;
    int UDP_socket = socket(AF_INET, SOCK_DGRAM, 0);
    while(1){
        if(closed_server){
            string msg = "---ServerisClosed---";
            for(int i=0;i<online.size();i++){
                if(online[i] && i!=owner){
                    struct sockaddr_in receiverAddr;
                    receiverAddr.sin_family = AF_INET;
                    receiverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
                    receiverAddr.sin_port = htons(5550+i);
                    sendto(UDP_socket, msg.c_str(), msg.size(), 0, (struct sockaddr *) &receiverAddr, sizeof(receiverAddr));
                }
            }
            break;
        }
        // sending system messages to chat owner
        while(sys_num < sys_record.size() && online[owner]){
            if(sys_record[sys_num][0]-'0'==owner){
                sys_num++;
                continue;
            }
            for(int i=0;i<online.size();i++){
                if(online[i] && (sys_record[sys_num][0]-'0' != i) && (sys_record[sys_num][0]-'0' != owner)){
                    struct sockaddr_in receiverAddr;
                    receiverAddr.sin_family = AF_INET;
                    receiverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
                    receiverAddr.sin_port = htons(5550+i);
                    sendto(UDP_socket, sys_record[sys_num].c_str(), sys_record[sys_num].size(), 0, (struct sockaddr *) &receiverAddr, sizeof(receiverAddr));
                }
            }
            sys_num++;
        }

        // broadcast chat messages
        while(chat_num < chat_record.size()){
            for(int i=0;i<online.size();i++){
                if(online[i] && (chat_record[chat_num][0]-'0' != i)){
                    struct sockaddr_in receiverAddr;
                    receiverAddr.sin_family = AF_INET;
                    receiverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
                    receiverAddr.sin_port = htons(5550+i);
                    sendto(UDP_socket, chat_record[chat_num].c_str(), chat_record[chat_num].size(), 0, (struct sockaddr *) &receiverAddr, sizeof(receiverAddr));
                }
            }
            chat_num++;
        }
    }
    close(UDP_socket);
    exit(0);
}

void *TCP_connection(void *parameter){
    int newConnection = *((int *)parameter);
    char buffer[1024];
    memset(&buffer, '\0', sizeof(buffer));

    // send the last 3 chat record
    string record="1";
    if(chat_record.size()<3){
        for(int i=0;i<chat_record.size();i++){
            int len = chat_record[i].size();
            record = record + chat_record[i].substr(1,len-1) + "\n";
        }
    }
    else{
        for(int i=chat_record.size()-3;i<chat_record.size();i++){
            int len = chat_record[i].size();
            record = record + chat_record[i].substr(1,len-1) + "\n";
        }
    }
    
    int len = record.size();
    char send_buffer[len+1];
    memset(&send_buffer, '\0', sizeof(send_buffer));

    strcpy(send_buffer, record.c_str()); 
	send(newConnection, send_buffer, strlen(send_buffer), 0);

    // receive name
    recv(newConnection, buffer, 1024, 0);
    int random_number = buffer[0]-'0';
    online[random_number]=true;
    string tmp="";
    for(int i=0;i<strlen(buffer);i++){
        if(buffer[i]=='\0'){
            break;
        }
        tmp+=buffer[i];
    }
    sys_record.push_back(tmp);
    memset(&buffer, '\0', sizeof(buffer));

    while(1){
        recv(newConnection, buffer, 1024, 0);
        // leaving chatroom
        if(buffer[0] == '!'){
            online[random_number]=false;
            string tmp="";
            for(int i=1;i<strlen(buffer);i++){
                if(buffer[i]=='\0'){
                    break;
                }
                tmp+=buffer[i];
            }
            sys_record.push_back(tmp);
            break;
        }
        else if(strcmp(buffer, leave_room.c_str()) == 0){
            closed_server=true;
            break;
        }
        else{
            string tmp="";
            for(int i=0;i<strlen(buffer);i++){
                if(buffer[i]=='\0'){
                    break;
                }
                tmp+=buffer[i];
            }
            chat_record.push_back(tmp);
        }
        memset(&buffer, '\0', sizeof(buffer));
    }
    close(newConnection);
}
int main(int argc, char *argv[]){
    int owner = atoi(argv[1]);
    int port = atoi(argv[2]);
    online.resize(10, false);

    pthread_attr_t attr, attru;
    pthread_attr_init(&attr);
    pthread_attr_init(&attru);

    int TCP_socket = socket(AF_INET, SOCK_STREAM, 0);
    // Build up server information structure
    struct sockaddr_in server_info;
    bzero(&server_info, sizeof(server_info));
    server_info.sin_family = AF_INET;
    server_info.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_info.sin_port = htons(port);

    bind(TCP_socket, (struct sockaddr *) &server_info, sizeof(server_info));
    listen(TCP_socket, 10);
    int newConnection=0, clients=0;
    struct sockaddr_in client_info;               // Declare the client information structure
    socklen_t client_size;

    pthread_create(&udp_pid, &attru, UDP_Broadcast, (void *)&owner);

    while(1){
        newConnection = accept(TCP_socket, (struct sockaddr*)&client_info, &client_size);
        pthread_create(&pid[clients++],&attr,TCP_connection,(void *)&newConnection);
    }
    return 0;
}