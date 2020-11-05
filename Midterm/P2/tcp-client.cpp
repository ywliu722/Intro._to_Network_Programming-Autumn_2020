#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;
int main(int argc, char* argv[]){
    char* server_IP=argv[1];
    int server_port=atoi(argv[2]);

    struct sockaddr_in server_info;
    bzero(&server_info,sizeof(server_info));
    server_info.sin_family=AF_INET;
    server_info.sin_addr.s_addr=inet_addr(server_IP);
    server_info.sin_port=htons(server_port);

    int TCP_socket=socket(AF_INET, SOCK_STREAM, 0);
    int TCP_connection = connect(TCP_socket, (struct sockaddr*)&server_info, sizeof(server_info));
    
    string input="";
    char buffer[1024];
    memset(&buffer, '\0', sizeof(buffer));
    cout<<"% ";
    while(getline(cin,input)){
        if(input=="exit"){
            send(TCP_socket, input.c_str(), input.size(), 0);
            break;
        }
        send(TCP_socket, input.c_str(), input.size(), 0);
	    recv(TCP_socket, buffer, 1024, 0);
	    cout<<buffer<<endl;
	    memset(&buffer, '\0', sizeof(buffer));
        cout<<"% ";
    }

    close(TCP_socket);
    
    return 0;
}
