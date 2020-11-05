#include <iostream>
#include <cstdlib>
#include <fstream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include <string>
#include <pthread.h>

using namespace std;
class client{
    public:
        string IP="";
        unsigned short port=0;
        int fd=0;
};
vector<client> ct;
void *TCP_connection(void *parameter){
    int index = *((int *)parameter);
    char buffer[1024];
    memset(&buffer, '\0', sizeof(buffer));
    int newConnection=ct[index].fd;
    string command1="list-user";
    string command2="get-ip";
    string command3="exit";

    while(1){
        string tmp="";
        recv(newConnection, buffer, 1024, 0);
        if(strcmp(command1.c_str(),buffer)==0){
            for(int i=0;i<ct.size();i++){
                if(ct[i].fd==-1){
                    continue;
                }
                tmp = tmp  + to_string(i+1) + " ";
            }
        }
        else if(strcmp(command2.c_str(),buffer)==0){
            tmp= tmp + "IP: " + ct[index].IP + ":" + to_string(ct[index].port);
        }
        else if(strcmp(command3.c_str(),buffer)==0){
            tmp= tmp + "Bye, " + to_string(index+1) + ".";
            ct[index].fd=-1;
            // Convert the sting into char*
            int back_len = tmp.size();
            char send_buffer[back_len+1];
            memset(&send_buffer, '\0', sizeof(send_buffer));
            strcpy(send_buffer, tmp.c_str());
            send(newConnection, send_buffer, strlen(send_buffer), 0);
            break;
        }
        else{
            tmp="Invalid Input";
        }
        // Flush the buffer
		memset(&buffer, '\0', sizeof(buffer));

        // Convert the sting into char*
        int back_len = tmp.size();
        char send_buffer[back_len+1];
        memset(&send_buffer, '\0', sizeof(send_buffer));
        strcpy(send_buffer, tmp.c_str());
        send(newConnection, send_buffer, strlen(send_buffer), 0);
    }

}
int main(int argc, char* argv[]){
    int server_port=atoi(argv[1]);
    pthread_t pid[15];
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    
    // Create both TCP and UDP socket
    int TCP_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (TCP_socket < 0) { 
        cout<<"TCP socket creation error!"<<endl;
        return 0; 
    }

    // Build up server information structure
    struct sockaddr_in server_info;
    bzero(&server_info, sizeof(server_info));
    server_info.sin_family = AF_INET;
    server_info.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_info.sin_port = htons(server_port);

    while(bind(TCP_socket, (struct sockaddr *) &server_info, sizeof(server_info)) < 0){
        cout<<"TCP binding error!"<<endl;
    }
    // TCP waiting for connection
    if(listen(TCP_socket, 10) != 0){
		cout<<"TCP binding error!"<<endl;
	}
    struct sockaddr_in client_info;               // Declare the client information structure
    socklen_t client_size;                                  // Address structure size
    char buffer[1024];                                         // Buffer for receiving the message
    memset(&buffer, '\0', sizeof(buffer));  // Initializing the buffer
    int clients=0;
    while(1){
        int newConnection = accept(TCP_socket, (struct sockaddr*)&client_info, &client_size);
        client tmp;
        unsigned short port=(unsigned short)ntohs(client_info.sin_port);
        string IP=inet_ntoa(client_info.sin_addr);
        tmp.IP=IP;
        tmp.port=port;
        tmp.fd=newConnection;
        ct.push_back(tmp);
        int num=ct.size()-1;
        cout<<"New connection from "<< IP<<":"<<port<<" "<<num+1<<endl;
        pthread_create(&pid[clients++],&attr,TCP_connection,(void *)&num);
    }
    
}