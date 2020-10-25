#include <iostream>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

// To process the client input string
vector<string> input_processing(string client_input){
    int index=0;
    vector<string> result;
    result.resize(4);
    for(int i=0;i<4;i++){
        result[i].clear();
    }
    for(int i=0; i<client_input.size();i++){
        if(index>3){
            result[0].clear();
            return result;
        }
        if(client_input[i]==' '){
            index++;
        }
        else{
            result[index] += client_input[i];
        }
    }
    return result;
}
// Register
void Register(int UDP_socket, vector<string> &client_command, struct sockaddr_in &server_info){
    string tmp = "0 " + client_command[1] + " " + client_command[2] + " " + client_command[3];
    int len = tmp.size();

    char send_buffer[len+1];
    char receive_buffer[1024];
    memset(&send_buffer, '\0', sizeof(send_buffer));
    memset(&receive_buffer, '\0', sizeof(receive_buffer));

    socklen_t server_size=sizeof(server_info);
    strcpy(send_buffer, tmp.c_str());
    sendto(UDP_socket, send_buffer, strlen(send_buffer), 0, (const struct sockaddr*)&server_info, sizeof(server_info));
    if(recvfrom(UDP_socket, receive_buffer, 1024, 0, (struct sockaddr*)&server_info, &server_size) <0){
        cout<<"Error receiving data!"<<endl;
    }
    else{
        if(receive_buffer[0]=='S'){
            cout<<"Register successfully."<<endl;
        }
        else{
            cout<<"Username is already used."<<endl;
        }
    }
}
// Login
int Login(int TCP_socket, vector<string> &client_command){
    string tmp = "1 " + client_command[1] + " " + client_command[2];
    int len = tmp.size();

    char send_buffer[len+1];
    char receive_buffer[1024];
    memset(&send_buffer, '\0', sizeof(send_buffer));
    memset(&receive_buffer, '\0', sizeof(receive_buffer));

    strcpy(send_buffer, tmp.c_str()); 
	send(TCP_socket, send_buffer, strlen(send_buffer), 0);
    if(recv(TCP_socket, receive_buffer, 1024, 0) < 0){
	    cout<<"Error receiving data!"<<endl;
	}
    else{
        if(receive_buffer[0]=='0'){
            int len = strlen(receive_buffer) -2;
            char tmp[len+1];
            memcpy(tmp, &receive_buffer[2], len);
            int random_number = atoi(tmp);
            cout<<"Welcome, "<<client_command[1]<<endl;
            return random_number;
        }
        else{
            cout<<"Login failed."<<endl;
            return -1;
        }
	}
    return -1;
}
// Logout
void Logout(int TCP_socket, int random_number){
    string tmp = "2 " + to_string(random_number);
    int len = tmp.size();

    char send_buffer[len+1];
    char receive_buffer[1024];
    memset(&send_buffer, '\0', sizeof(send_buffer));
    memset(&receive_buffer, '\0', sizeof(receive_buffer));

    strcpy(send_buffer, tmp.c_str());
    send(TCP_socket, send_buffer, strlen(send_buffer), 0);
    if(recv(TCP_socket, receive_buffer, 1024, 0) < 0){
	    cout<<"Error receiving data!"<<endl;
	}
    else{
        cout<<receive_buffer<<endl;
    }
}
// Whoami
void Whoami(int UDP_socket, int random_number, struct sockaddr_in &server_info){
    string tmp = "3 " + to_string(random_number);
    int len = tmp.size();
    char send_buffer[len+1];
    char receive_buffer[1024];
    memset(&send_buffer, '\0', sizeof(send_buffer));
    memset(&receive_buffer, '\0', sizeof(receive_buffer));

    strcpy(send_buffer, tmp.c_str());
    socklen_t server_size=sizeof(server_info);
    sendto(UDP_socket, send_buffer, strlen(send_buffer), 0, (const struct sockaddr*)&server_info, sizeof(server_info));
    if(recvfrom(UDP_socket, receive_buffer, 1024, 0, (struct sockaddr*)&server_info, &server_size) <0){
        cout<<"Error receiving data!"<<endl;
    }
    else{
        cout<<receive_buffer<<endl;
    }
}
// List-User
void List_User(int TCP_socket){
    string tmp = "4";
    int len = tmp.size();

    char send_buffer[len+1];
    char receive_buffer[2048];
    memset(&send_buffer, '\0', sizeof(send_buffer));
    memset(&receive_buffer, '\0', sizeof(receive_buffer));

    strcpy(send_buffer, tmp.c_str());
    send(TCP_socket, send_buffer, strlen(send_buffer), 0);
    if(recv(TCP_socket, receive_buffer, 2048, 0) < 0){
	    cout<<"Error receiving data!"<<endl;
	}
    else{
        cout<<"Name\tEmail"<<endl;
        int index=0;
        for(int i=0;i<sizeof(receive_buffer);i++){
            if(receive_buffer[i]=='\0'){
                break;
            }
            if(receive_buffer[i]==' '){
                if(index%2){
                    cout<<endl;
                    index++;
                }
                else{
                    cout<<"\t";
                    index++;
                }
            }
            else{
                cout<<receive_buffer[i];
            }
        }
    }
}
// Exit
void Exit(int TCP_socket, int UDP_socket){
    string tmp="5";
    int len = tmp.size();

    char send_buffer[len+1];
    memset(&send_buffer, '\0', sizeof(send_buffer));
    
    strcpy(send_buffer, tmp.c_str());
    send(TCP_socket, send_buffer, strlen(send_buffer), 0);

    // Close socket
    close(TCP_socket);
    close(UDP_socket);
}
void Create_Board(int TCP_socket, string input, int random_number){
    string tmp = "60 ";
    tmp = tmp + to_string(random_number) + " ";
    for(int i=13 ; i<input.size();i++){
        tmp += input[i];
    }
    int len = tmp.size();

    char send_buffer[len+1];
    char receive_buffer[2048];
    memset(&send_buffer, '\0', sizeof(send_buffer));
    memset(&receive_buffer, '\0', sizeof(receive_buffer));

    strcpy(send_buffer, tmp.c_str());
    send(TCP_socket, send_buffer, strlen(send_buffer), 0);
    if(recv(TCP_socket, receive_buffer, 2048, 0) < 0){
	    cout<<"Error receiving data!"<<endl;
	}
    else{
        cout<<receive_buffer<<endl;
    }
}
void Create_Post(int TCP_socket, string input){
    string tmp = "61 ";
}
//list-board
void List_Board(int TCP_socket){
    string tmp = "62 ";
}
//list-post
void List_Post(int TCP_socket, string input){
    string tmp = "63 ";
}
//read
void Read(int TCP_socket, string input){
    string tmp = "64 ";
}
//delete-post
void Delete_Post(int TCP_socket, string input){
    string tmp = "65 ";
}
//update-post
void Update_Post(int TCP_socket, string input){
    string tmp = "66 ";
}
//comment
void Comment(int TCP_socket, string input){
    string tmp = "67 ";
}
// Main function
int main(int argc, char *argv[]){

    // Get and process the information from command line
    if(argc!=3){
        cout<<"Usage: ./client <server IP> <server port>"<<endl;
        return 0;
    }
    char* server_IP=argv[1];
    int server_port=atoi(argv[2]);

    // Create Socket for both TCP and UDP
    int TCP_socket = socket(AF_INET, SOCK_STREAM, 0);
    int UDP_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (TCP_socket < 0) { 
        cout<<"TCP socket creation error!"<<endl;
        return 0; 
    }
    if (UDP_socket < 0) { 
        cout<<"UDP socket creation error!"<<endl;
        return 0; 
    }

    // Socket Connection
    struct sockaddr_in server_info;
    bzero(&server_info, sizeof(server_info));
    server_info.sin_family = AF_INET;
    server_info.sin_addr.s_addr = inet_addr(server_IP);
    server_info.sin_port = htons(server_port);

    int TCP_connection = connect(TCP_socket, (struct sockaddr *) &server_info, sizeof(server_info));
    if(TCP_connection<0){
        cout<<"Connection error!"<<endl;
        return 0;
    }

    cout<<"********************************"<<endl
             <<"** Welcome to the BBS server. **"<<endl
             <<"********************************"<<endl;
    
    int  random_number=-1;
    string client_input="";
    cout<<"% ";
    
    while(getline(cin, client_input)){
        string command="";
        for(int i=0;i<client_input.size();i++){
            if(client_input[i]==' '){
                break;
            }
            command += client_input[i];
        }
        // Board and Post related command
        if(command == "create-board"){
            if(random_number<0){
                cout<<"Please login first."<<endl;
            }
            else{
                Create_Board(TCP_socket, client_input, random_number);
            }
            cout<<"% ";
            continue;
        }
        else if(command == "create-post"){
            if(random_number<0){
                cout<<"Please login first."<<endl;
            }
            cout<<"% ";
            continue;
        }
        else if(command == "list-board"){
            cout<<"% ";
            continue;
        }
        else if(command == "list-post"){
            cout<<"% ";
            continue;
        }
        else if(command == "read"){
            cout<<"% ";
            continue;
        }
        else if(command == "delete-post"){
            if(random_number<0){
                cout<<"Please login first."<<endl;
            }
            cout<<"% ";
            continue;
        }
        else if(command == "update-post"){
            if(random_number<0){
                cout<<"Please login first."<<endl;
            }
            cout<<"% ";
            continue;
        }
        else if(command == "comment"){
            if(random_number<0){
                cout<<"Please login first."<<endl;
            }
            cout<<"% ";
            continue;
        }

        // Process the client input (non Board and Post related)
       vector<string> client_command = input_processing(client_input);

        // register
        if(client_command[0] == "register"){
            if(client_command[1] == "" || client_command[2] == "" || client_command[3] == ""){
                cout<<"Usage: register <username> <email> <password>"<<endl;
            }
            else{
                Register(UDP_socket, client_command, server_info);
            }
        }
        // login
        else if(client_command[0] == "login"){
            if(random_number>=0){
                cout<<"Please logout first."<<endl;
            }
            else if(client_command[1]== "" || client_command[2]== "" || client_command[3] != ""){
                cout<<"Usage: login <username> <password>"<<endl;
            }
            else{
                random_number = Login(TCP_socket, client_command);
            }
        }
        // logout
        else if(client_command[0] == "logout"){
            if(random_number<0){
                cout<<"Please login first."<<endl;
            }
            else{
                Logout(TCP_socket, random_number);
                random_number=-1;
            }
        }
        // whoami
        else if(client_command[0] == "whoami"){
            if(random_number<0){
                cout<<"Please login first."<<endl;
            }
            else{
                Whoami(UDP_socket, random_number, server_info);
            }
        }
        // list-user
        else if(client_command[0] == "list-user"){
            List_User(TCP_socket);
        }
        // exit
        else if(client_command[0] == "exit"){
           Exit(TCP_socket, UDP_socket);
            break;
        }
        // invalid command
        else{
            cout<<"Please enter the right command"<<endl;
        }
        
        client_command.clear();
        cout<<"% ";
    }

    return 0;
}