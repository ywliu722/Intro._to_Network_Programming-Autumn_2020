#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;
string end_flag="END OF FILE";
int main(int argc, char* argv[]){

    char* server_IP = argv[1];
    int server_port=atoi(argv[2]);

    // Build up server information structure
    struct sockaddr_in server_info;
    bzero(&server_info, sizeof(server_info));
    server_info.sin_family = AF_INET;
    server_info.sin_addr.s_addr = inet_addr(server_IP);
    server_info.sin_port = htons(server_port);
    
    int UDP_socket=socket(AF_INET, SOCK_DGRAM, 0);
    string input="";
    char receive_buffer[1024];
    memset(&receive_buffer, '\0', sizeof(receive_buffer));
    cout<<"% ";
    while(getline(cin,input)){
        // exit
        if(input == "exit"){
            break;
        }
        // get-file-list
        if(input=="get-file-list"){
            int len=0;
            socklen_t server_size=sizeof(server_info);
            sendto(UDP_socket, input.c_str(),input.size(), 0, (const struct sockaddr*)&server_info, sizeof(server_info));
            len=recvfrom(UDP_socket, receive_buffer, 1024, 0, (struct sockaddr*)&server_info, &server_size);
            for(int i=0;i<len;i++){
                cout<<receive_buffer[i];
            }
            cout<<endl;
            memset(&receive_buffer, '\0', sizeof(receive_buffer));
        }
        // get-file
        else{
            int filenum=0;
            for(int i=0;i<input.size();i++){
                if(input[i]==' '){
                    filenum++;
                }
            }
            socklen_t server_size=sizeof(server_info);
            sendto(UDP_socket, input.c_str(),input.size(), 0, (const struct sockaddr*)&server_info, sizeof(server_info));
            for(int i=0;i<filenum;i++){
                recvfrom(UDP_socket, receive_buffer, 1024, 0, (struct sockaddr*)&server_info, &server_size);
                fstream fd;
                cout<<receive_buffer<<endl;
                fd.open(receive_buffer, ios::out);
                if(!fd){
                cout<<"Cannot open the file!"<<endl;
                }
                memset(&receive_buffer, '\0', sizeof(receive_buffer)); 
                // receive content
                while(1){
                    recvfrom(UDP_socket, receive_buffer, 1024, 0, (struct sockaddr*)&server_info, &server_size);
                    // compare with EOF flag
                    if(strcmp(receive_buffer, end_flag.c_str())==0){
                        cout<<"End of the file transform!"<<endl;
                        break;
                    }
                    // determine the length of file chunk
                    int len=0;
                    while(receive_buffer[len]!='\0'){
                        len++;
                    }
                    fd.write(receive_buffer, len);  // write the chunk into file
                    memset(&receive_buffer, '\0', sizeof(receive_buffer));  // Initializing the buffer
                }
                fd.close();
            }
        }
         cout<<"% ";
    }
    return 0;
}