#include <iostream>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

using namespace std;
typedef struct{
    int random_number, port;
}chat_info;
bool isOwner=false, closed_server=false;
string usr="", server_close="---ServerisClosed---", interrupt_msg="";
pthread_t pid[2];   // one for create_chatroom, one for receiving UDP messages
pthread_attr_t attr, attrc;

void Leave_Chatroom(int TCP_socket, int random_number);
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

// BBS basic command
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
vector<int> Login(int TCP_socket, vector<string> &client_command){
    string tmp = "1 " + client_command[1] + " " + client_command[2];
    int len = tmp.size();

    vector<int> re;
    re.resize(2,-1);

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
            re[0] = receive_buffer[2] - '0';
            int len = strlen(receive_buffer) -4;
            char tmp[len+1];
            memcpy(tmp, &receive_buffer[4], len);
            re[1] = atoi(tmp);
            cout<<"Welcome, "<<client_command[1]<<endl;
            usr=client_command[1];
            return re;
        }
        else{
            cout<<"Login failed."<<endl;
            return re;
        }
	}
    return re;
}
bool Logout(int TCP_socket, int random_number){
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
        if(receive_buffer[0] == 'R'){
            cout<<"Please do “attach” and “leave-chatroom” first."<<endl;
            return false;
        }
        else{
            cout<<receive_buffer<<endl;
            return true;
        }
    }
}
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

// BBS command to server
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
void Create_Post(int TCP_socket, string input, int random_number){
    string tmp = "61 ";
    tmp = tmp +to_string(random_number) + " ";
    for(int i=12 ; i<input.size();i++){
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
void List_Board(int TCP_socket){
    string tmp = "62";
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
void List_Post(int TCP_socket, string input){
    string tmp = "63 ";
    for(int i=10 ; i<input.size();i++){
        tmp += input[i];
    }
    int len = tmp.size();

    char send_buffer[len+1];
    char receive_buffer[4096];
    memset(&send_buffer, '\0', sizeof(send_buffer));
    memset(&receive_buffer, '\0', sizeof(receive_buffer));

    strcpy(send_buffer, tmp.c_str());
    send(TCP_socket, send_buffer, strlen(send_buffer), 0);
    if(recv(TCP_socket, receive_buffer, 4096, 0) < 0){
	    cout<<"Error receiving data!"<<endl;
	}
    else{
        cout<<receive_buffer<<endl;
    }
}
void Read(int TCP_socket, string input){
    string tmp = "64 ";
    for(int i=5 ; i<input.size();i++){
        tmp += input[i];
    }
    int len = tmp.size();

    char send_buffer[len+1];
    char receive_buffer[4096];
    memset(&send_buffer, '\0', sizeof(send_buffer));
    memset(&receive_buffer, '\0', sizeof(receive_buffer));

    strcpy(send_buffer, tmp.c_str());
    send(TCP_socket, send_buffer, strlen(send_buffer), 0);
    if(recv(TCP_socket, receive_buffer, 4096, 0) < 0){
	    cout<<"Error receiving data!"<<endl;
	}
    else{
        cout<<receive_buffer<<endl;
    }
}
void Delete_Post(int TCP_socket, string input, int random_number){
    string tmp = "65 ";
    tmp = tmp +to_string(random_number) + " ";
    for(int i=12 ; i<input.size();i++){
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
void Update_Post(int TCP_socket, string input, int random_number){
    string tmp = "66 ";
    tmp = tmp +to_string(random_number) + " ";
    for(int i=12 ; i<input.size();i++){
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
void Comment(int TCP_socket, string input, int random_number){
    string tmp = "67 ";
    tmp = tmp +to_string(random_number) + " ";
    for(int i=8 ; i<input.size();i++){
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

vector<string> getTime(){
    // get current date
    time_t now = time(0);
    tm *ltm = localtime(&now);
    int hour = ltm->tm_hour, minute = ltm->tm_min;
    vector<string> timing;
    timing.resize(2,"");
    if(hour<10){
        timing[0]="0"+to_string(hour);
    }
    else{
        timing[0]=to_string(hour);
    }
    if(minute<10){
        timing[1]="0"+to_string(minute);
    }
    else{
        timing[1]=to_string(minute);
    }
    return timing;
}
// Chatroom command to chat server
void *Receiving_Chat(void *random){
    int random_number = *((int *)random);

    struct sockaddr_in broadcastAddr; /* Broadcast Address */
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    broadcastAddr.sin_family = AF_INET;                 /* Internet address family */
    broadcastAddr.sin_addr.s_addr = htonl(INADDR_ANY);  /* Any incoming interface */
    broadcastAddr.sin_port = htons(5550+random_number);      /* Broadcast port */

    /* Bind to the broadcast port */
    bind(sock, (struct sockaddr *) &broadcastAddr, sizeof(broadcastAddr));

    char buffer[1024];
    memset(&buffer, '\0', sizeof(buffer));
    
    while(1){
        recvfrom(sock, buffer, 1024, 0, NULL, 0);
        if(strcmp(buffer, server_close.c_str()) == 0){
            closed_server=true;
            vector<string> timing = getTime();
            string msg="sys[" + timing[0] + ":" + timing[1] + "] : the chatroom is close.\nWelcome back to BBS.\n";
            cout<<msg;
            break;
        }
        for(int i=1;i<strlen(buffer);i++){
            if(buffer[i]=='\0'){
                break;
            }
            cout<<buffer[i];
        }
        cout<<endl;
        memset(&buffer, '\0', sizeof(buffer));
    }
    close(sock);
}

void Connect_Chat(int port, int random_number, int TCP_socket){
    int chat_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (chat_socket < 0) { 
        cout<<"TCP socket creation error!"<<endl;
        return;
    }
    if(port >=5550 && port <=5559){
        port +=10;
    }
    // Socket Connection
    struct sockaddr_in server_info;
    bzero(&server_info, sizeof(server_info));
    server_info.sin_family = AF_INET;
    server_info.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_info.sin_port = htons(port);

    int chat_connection = connect(chat_socket, (struct sockaddr *) &server_info, sizeof(server_info));
    if(chat_connection<0){
        cout<<"Connection error!"<<endl;
        if(isOwner){
            Leave_Chatroom(TCP_socket, random_number);
	        pthread_cancel(pid[0]);
        }
        close(chat_socket);
        return;
    }
    pthread_create(&pid[1],&attr,Receiving_Chat,(void *)&random_number);
    cout<<"*****************************\n"
          "** Welcome to the chatroom **\n"
          "*****************************\n";
    // receive chat record
    char buffer[1024];
    memset(&buffer, '\0', sizeof(buffer));
    recv(chat_socket, buffer, 1024, 0);
    for(int i=1;i<strlen(buffer);i++){
        if(buffer[i]=='\0'){
            break;
        }
        cout<<buffer[i];
    }

    // sending connection message
    if(!isOwner || isOwner){
        vector<string> timing = getTime();
        string msg= "" + to_string(random_number) + "sys[" + timing[0] + ":" + timing[1] + "] : " + usr + " join us.";
        
        int len = msg.size();
        char send_buffer[len+1];
        memset(&send_buffer, '\0', sizeof(send_buffer));

        strcpy(send_buffer, msg.c_str()); 
	    send(chat_socket, send_buffer, strlen(send_buffer), 0);
    }
    string input="";
    while(getline(cin, input)){
        if(closed_server){
            interrupt_msg=input;
            break;
        }
        if(input == "leave-chatroom"){
            if(isOwner){
                string msg = "---Close Server---";
                send(chat_socket, msg.c_str(), msg.size(), 0);
                close(chat_socket);
                Leave_Chatroom(TCP_socket, random_number);
                break;
            }
            else{
                vector<string> timing = getTime();
                string msg= "!" + to_string(random_number) + "sys[" + timing[0] + ":" + timing[1] + "] : " + usr + " leave us.";
                send(chat_socket, msg.c_str(), msg.size(), 0);
                close(chat_socket);
                break;
            }
        }
        else if(input == "detach" && isOwner){
            vector<string> timing = getTime();
            string msg= "!" + to_string(random_number) + "sys[" + timing[0] + ":" + timing[1] + "] : " + usr + " leave us.";
            send(chat_socket, msg.c_str(), msg.size(), 0);
            close(chat_socket);
            break;
        }
        else{
            vector<string> timing = getTime();
            string msg= "" + to_string(random_number) + usr + "[" + timing[0] + ":" + timing[1] + "] : " + input;
            int len = msg.size();
            char send_buffer[len+1];
            memset(&send_buffer, '\0', sizeof(send_buffer));
            strcpy(send_buffer, msg.c_str()); 
	        send(chat_socket, send_buffer, strlen(send_buffer), 0);
        }
    }
    close(chat_socket);
}

// Chatroom command to Server
void *Chat(void *parameter){
    chat_info *para=(chat_info*)parameter;
    int port = para->port;
    int random_number = para->random_number;
    string command = "./chat " + to_string(random_number) + " " + to_string(port) + " " + usr;
    system(command.c_str());
}
int Create_Chatroom(int TCP_socket, int random_number, vector<string> &client_command){
    string tmp = "70 ";
    tmp = tmp +to_string(random_number) + " " + client_command[1];
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
        if(receive_buffer[0]=='S'){
            cout<<"start to create chatroom…"<<endl;
            chat_info para;
            para.port=atoi(client_command[1].c_str());
            para.random_number=random_number;
            pthread_attr_init(&attrc);
            pthread_create(&pid[0],&attrc,Chat,(void *)&para);
            sleep(1);
            return atoi(client_command[1].c_str());
        }
        else if(receive_buffer[0]=='N'){
            cout<<"Please create-chatroom first."<<endl;
        }
        else{
            cout<<"Your chatroom is still running."<<endl;
        }
    }
    return -1;
}
void List_Chatroom(int UDP_socket, struct sockaddr_in &server_info){
    string tmp = "71";
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
        cout<<receive_buffer<<endl;
    }
}
void Join_Chatroom(int TCP_socket, int random_number, vector<string> &client_command){
    string tmp = "72 ";
    tmp = tmp + client_command[1];
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
        if(receive_buffer[0]=='F'){
            cout<<"The chatroom does not exist or the chatroom is close."<<endl;
        }
        else{
            string tmp_port="";
            for(int i=0;i<strlen(receive_buffer);i++){
                if(receive_buffer[i]=='\0'){
                    break;
                }
                tmp_port += receive_buffer[i];
            }
            int port=atoi(tmp_port.c_str());
            Connect_Chat(port, random_number, TCP_socket);
            closed_server=false;
            if(interrupt_msg==""){
                cout<<"Welcome back to BBS.\n% ";
            }
        }
    }
}
bool Restart_Chatroom(int TCP_socket, int random_number, int port){
    string tmp = "73 ";
    tmp = tmp +to_string(random_number);
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
        if(receive_buffer[0]=='S'){
            cout<<"start to create chatroom…"<<endl;
            chat_info para;
            para.port=port;
            para.random_number=random_number;
            pthread_attr_init(&attrc);
            pthread_create(&pid[0],&attrc,Chat,(void *)&para);
            sleep(1);
            return true;
        }
        else if(receive_buffer[0]=='N'){
            cout<<"Please create-chatroom first."<<endl;
        }
        else{
            cout<<"Your chatroom is still running."<<endl;
        }
    }
    return false;
}
void Leave_Chatroom(int TCP_socket, int random_number){
    string tmp = "74 ";
    tmp = tmp +to_string(random_number);
    int len = tmp.size();

    char send_buffer[len+1];
    char receive_buffer[1024];
    memset(&send_buffer, '\0', sizeof(send_buffer));
    memset(&receive_buffer, '\0', sizeof(receive_buffer));

    strcpy(send_buffer, tmp.c_str()); 
	send(TCP_socket, send_buffer, strlen(send_buffer), 0);
}

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
    int chat_port = -1;
    string client_input="";
    cout<<"% ";
    
    while(1){
        if(interrupt_msg==""){
            getline(cin, client_input);
        }
        else{
            client_input = interrupt_msg;
            interrupt_msg="";
        }
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
            else{
                Create_Post(TCP_socket, client_input, random_number);
            }
            cout<<"% ";
            continue;
        }
        else if(command == "list-board"){
            List_Board(TCP_socket);
            cout<<"% ";
            continue;
        }
        else if(command == "list-post"){
            List_Post(TCP_socket, client_input);
            cout<<"% ";
            continue;
        }
        else if(command == "read"){
            Read(TCP_socket, client_input);
            cout<<"% ";
            continue;
        }
        else if(command == "delete-post"){
            if(random_number<0){
                cout<<"Please login first."<<endl;
            }
            else{
                Delete_Post(TCP_socket, client_input, random_number);
            }
            cout<<"% ";
            continue;
        }
        else if(command == "update-post"){
            if(random_number<0){
                cout<<"Please login first."<<endl;
            }
            else{
                Update_Post(TCP_socket, client_input, random_number);
            }
            cout<<"% ";
            continue;
        }
        else if(command == "comment"){
            if(random_number<0){
                cout<<"Please login first."<<endl;
            }
            else{
                Comment(TCP_socket, client_input, random_number);
            }
            cout<<"% ";
            continue;
        }

        // Process the client input (non Board and Post related)
       vector<string> client_command = input_processing(client_input);

        // BBS basic command
        if(client_command[0] == "register"){
            if(client_command[1] == "" || client_command[2] == "" || client_command[3] == ""){
                cout<<"Usage: register <username> <email> <password>"<<endl;
            }
            else{
                Register(UDP_socket, client_command, server_info);
            }
        }
        else if(client_command[0] == "login"){
            if(random_number>=0){
                cout<<"Please logout first."<<endl;
            }
            else if(client_command[1]== "" || client_command[2]== "" || client_command[3] != ""){
                cout<<"Usage: login <username> <password>"<<endl;
            }
            else{
                vector<int> re = Login(TCP_socket, client_command);
                random_number = re[0];
                chat_port = re[1];
                //cout<<random_number<<endl;
            }
        }
        else if(client_command[0] == "logout"){
            if(random_number<0){
                cout<<"Please login first."<<endl;
            }
            else{
                bool success = Logout(TCP_socket, random_number);
                if(success){
                    usr="";
                    random_number=-1;
                }
            }
        }
        else if(client_command[0] == "whoami"){
            if(random_number<0){
                cout<<"Please login first."<<endl;
            }
            else{
                Whoami(UDP_socket, random_number, server_info);
            }
        }
        else if(client_command[0] == "list-user"){
            List_User(TCP_socket);
        }
        else if(client_command[0] == "exit"){
           Exit(TCP_socket, UDP_socket);
            break;
        }

        // chatroom related command
        else if(client_command[0] == "create-chatroom"){
            if(random_number<0){
                cout<<"Please login first."<<endl;
            }
            else if(client_command[1]==""){
                cout<<"Usage: create-chatroom <port>"<<endl;
            }
            else{
                chat_port = Create_Chatroom(TCP_socket, random_number, client_command);
                if(chat_port){
                    isOwner=true;
                    Connect_Chat(chat_port, random_number, TCP_socket);
                    isOwner=false;
                    closed_server=false;
                    cout<<"Welcome back to BBS."<<endl;
                }
            }
        }
        else if(client_command[0] == "list-chatroom"){
            if(random_number<0){
                cout<<"Please login first."<<endl;
            }
            else{
                List_Chatroom(UDP_socket, server_info);
            }
        }
        else if(client_command[0] == "join-chatroom"){
            if(random_number<0){
                cout<<"Please login first."<<endl;
            }
            else if(client_command[1]==""){
                cout<<"Usage: join-chatroom <chatroom_name>"<<endl;
            }
            else{
                Join_Chatroom(TCP_socket, random_number, client_command);
                continue;
            }
        }
        else if(client_command[0] == "restart-chatroom"){
            if(random_number<0){
                cout<<"Please login first."<<endl;
            }
            else{
                bool success = Restart_Chatroom(TCP_socket, random_number, chat_port);
                if(success){
                    isOwner=true;
                    Connect_Chat(chat_port, random_number, TCP_socket);
                    isOwner=false;
                    closed_server=false;
                    cout<<"Welcome back to BBS."<<endl;
                }
            }
        }
        else if(client_command[0] == "attach"){
            if(random_number<0){
                cout<<"Please login first."<<endl;
            }
            else if(chat_port<0){
                cout<<"Please create-chatroom first."<<endl;
            }
            else{
                isOwner=true;
                Connect_Chat(chat_port, random_number, TCP_socket);
                isOwner=false;
                closed_server=false;
                cout<<"Welcome back to BBS."<<endl;
            }
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
