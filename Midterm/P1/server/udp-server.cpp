#include <iostream>
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
#include <dirent.h>
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

using namespace std;
string end_flag="END OF FILE";
int main(int argc, char* argv[]){

    int server_port=atoi(argv[1]);

    // Build up server information structure
    struct sockaddr_in server_info;
    bzero(&server_info, sizeof(server_info));
    server_info.sin_family = AF_INET;
    server_info.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_info.sin_port = htons(server_port);

    // open udp socket
    int UDP_socket = socket(AF_INET, SOCK_DGRAM, 0);
    while( bind(UDP_socket, (struct sockaddr *) &server_info, sizeof(server_info)) < 0){
        cout<<"UDP binding error!"<<endl;
    }

    struct sockaddr_in client_info;               // Declare the client information structure
    socklen_t client_size;                                  // Address structure size
    char buffer[1024];                                         // Buffer for receiving the message
    memset(&buffer, '\0', sizeof(buffer));
    string getlist="get-file-list";
    while(recvfrom(UDP_socket, buffer, sizeof(buffer), 0, (struct sockaddr*)&client_info, &client_size)){
        if(strcmp(getlist.c_str(), buffer)==0){
            string tmp="Files: ";
            for(auto& p: fs::directory_iterator(fs::current_path())){
                string path=(string)p.path();
                string process="";
                for(int i=path.size()-1;i>=0;i--){
                    if(path[i]=='/'){
                        for(int j=i+1;j<=path.size();j++){
                            process+=path[j];
                        }
                        i=-1;
                    }
                }
                tmp = tmp + process + " ";
            }
            // Convert the sting into char*
            int back_len = tmp.size();
            cout<<tmp<<endl;
            char send_buffer[back_len+1];
            memset(&send_buffer, '\0', sizeof(send_buffer));
            strcpy(send_buffer, tmp.c_str());
            sendto(UDP_socket, tmp.c_str(), tmp.size(), 0, (const struct sockaddr*)&client_info, sizeof(client_info));
        }
        else{
            string tmp="";
            vector<string> filelist;
            filelist.clear();
            for(int i=0;i<strlen(buffer);i++){
                if(buffer[i]=='\0'){
                    filelist.push_back(tmp);
                    tmp.clear();
                    break;
                }
                if(buffer[i]==' '){
                    filelist.push_back(tmp);
                    tmp.clear();
                }
                else{
                    tmp+=buffer[i];
                }
            }
            filelist.push_back(tmp);

            for(int i=1;i<filelist.size();i++){
                fstream fd;
                fd.open(filelist[i], ios::in);
                sendto(UDP_socket, filelist[i].c_str(), filelist[i].size(), 0, (const struct sockaddr*)&client_info, sizeof(client_info));
                while(!fd.eof()){
                    fd.read(buffer, sizeof(buffer));
                    sendto(UDP_socket, buffer, sizeof(buffer), 0, (const struct sockaddr*)&client_info, sizeof(client_info));
                    memset(&buffer, '\0', sizeof(buffer));
                }
                sendto(UDP_socket, end_flag.c_str(), end_flag.size(), 0, (const struct sockaddr*)&client_info, sizeof(client_info));
                fd.close();
            }
        }
    }

    return 0;
}