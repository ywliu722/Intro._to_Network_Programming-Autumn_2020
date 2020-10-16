#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sqlite3.h>
#include <pthread.h>

#define MAX_CLIENT 10

using namespace std;
int clients=0;
vector<string> ClientTable;
string quotesql( const string& s ) {
    return string("'") + s + string("'");
}
void SQL_Initialization(sqlite3 * db){
    char *zErrMsg = 0;

    string sql = "CREATE TABLE USERS("
                "UID INTEGER PRIMARY KEY AUTOINCREMENT,"
                "Username TEXT NOT NULL UNIQUE,"
                "Email TEXT NOT NULL,"
                "Password TEXT NOT NULL );";
    
    int database = sqlite3_exec(db, sql.c_str(), 0, 0, &zErrMsg);

    if( database != SQLITE_OK ){
        //fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
}
void Register(int UDP_socket, char* buffer, struct sockaddr_in &client_info, sqlite3 *database){
    int index=0;
    string usr="", email="", pwd="";
    for(int i=2;i<strlen(buffer);i++){
        if(buffer[i] == '\0'){
            break;
        }
        if(buffer[i] == ' '){
            index++;
        }
        if(index==0){
            usr += buffer[i];
        }
        else if(index==1){
            email += buffer[i];
        }
        else{
            pwd += buffer[i];
        }
    }

    string sql = "INSERT INTO USERS (Username, Email, Password) VALUES ("
           + quotesql(usr) + ", "
           + quotesql(email) + ","
           + quotesql(pwd) + ");";
    
    char *zErrMsg=0;
    string tmp = "";
    int result = sqlite3_exec(database, sql.c_str(), 0, 0, &zErrMsg);
    if( result != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        tmp = "F";
    }
    else{
        tmp = "S";
    }
    // Convert the sting into char*
    int len = tmp.size();
    char send_buffer[len+1];
    memset(&send_buffer, '\0', sizeof(send_buffer));
    strcpy(send_buffer, tmp.c_str());

    // Send the result back to the client
    sendto(UDP_socket, send_buffer, sizeof(send_buffer), 0, (struct sockaddr*)&client_info, sizeof(client_info));
}
void Login(int newConnection, char* buffer, sqlite3 * database){
    int index=0;
    string usr="", pwd="";
    string tmp="";
    for(int i=2;i<strlen(buffer);i++){
        if(buffer[i] == '\0'){
            break;
        }
        if(buffer[i] == ' '){
            index++;
        }
        if(index==0){
            usr += buffer[i];
        }
        else{
            pwd += buffer[i];
        }
    }
    char *zErrMsg;
    sqlite3_stmt *Stmt;
    string sql = "SELECT Password from USERS WHERE Username = ?";
    int fd = sqlite3_prepare_v2(database, sql.c_str(), -1, &Stmt, 0);
    if( fd != SQLITE_OK ){
        //fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } 
    else{
        sqlite3_bind_text(Stmt,1,usr.c_str(),-1,SQLITE_TRANSIENT);
    }
    int step = sqlite3_step(Stmt);
    
    if (step == SQLITE_ROW) {
        char* ret = (char*)sqlite3_column_text(Stmt,0);
        // 0: Success, 1: Fail
        if(strcmp(ret,pwd.c_str())==0){
            tmp="0 ";
            int random_number = 0;
            srand( time(NULL) );
            while(1){
                random_number = rand() % MAX_CLIENT;
                if(ClientTable[random_number]=="-"){
                    ClientTable[random_number].clear();
                    ClientTable[random_number] += usr;
                    break;
                }
            }
            tmp+=to_string(random_number);
        }
        else{
            tmp="1 ";
        }
    }
    else{
        tmp="1 ";
    }
    
    // Convert the sting into char*
    int len = tmp.size();
    char send_buffer[len+1];
    memset(&send_buffer, '\0', sizeof(send_buffer));
    strcpy(send_buffer, tmp.c_str());
    send(newConnection, send_buffer, strlen(send_buffer), 0);
}
void Logout(int newConnection, char* buffer){
    string back="Bye, ";
    int random_number = buffer[2]-'0';
    back = back + ClientTable[random_number] + ".";
    ClientTable[random_number].clear();
    ClientTable[random_number]+="-";
    
    // Convert the sting into char*
    int back_len = sizeof(back);
    char send_buffer[back_len+1];
    memset(&send_buffer, '\0', sizeof(send_buffer));
    strcpy(send_buffer, back.c_str());
    send(newConnection, send_buffer, strlen(send_buffer), 0);
}
void Whoami(int UDP_socket, char* buffer, struct sockaddr_in &client_info){
    string back="";
    int random_number = buffer[2]-'0';
    back += ClientTable[random_number];
    // Convert the sting into char*
    int back_len=sizeof(back);
    char send_buffer[back_len+1];
    memset(&send_buffer, '\0', sizeof(send_buffer));
    strcpy(send_buffer, back.c_str());
    // Send the result back to the client
    sendto(UDP_socket, send_buffer, sizeof(send_buffer), 0, (struct sockaddr*)&client_info, sizeof(client_info));
}
void List_User(int newConnection, sqlite3 * database){
    sqlite3_stmt *Stmt;
    string sql = "SELECT Username, Email FROM USERS";
    int fd=sqlite3_prepare_v2(database, sql.c_str(), -1, &Stmt, 0);
    char send_buffer[1024];
    memset(&send_buffer, '\0', sizeof(send_buffer));
    int nCol=0;
    while(sqlite3_step(Stmt) == SQLITE_ROW){
        nCol = 0;
        strcat(send_buffer, (char*)sqlite3_column_text(Stmt,nCol++));
        strcat(send_buffer, (char*)sqlite3_column_text(Stmt,nCol++));
        strcat(send_buffer," ");
    }
    send(newConnection, send_buffer, strlen(send_buffer), 0);
}
void *TCP_connection(void *parameter){
    int newConnection = *((int *)parameter);
    char buffer[1024];
    memset(&buffer, '\0', sizeof(buffer));

    // Initializing the database
    sqlite3 * database;
    sqlite3_open_v2("user.db", &database, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX, NULL);

    while(1){
        recv(newConnection, buffer, 1024, 0);
        //printf("Client(TCP): %s\n", buffer);

        // Classify the message
        switch(buffer[0]){
            // Login command
            case '1':
                Login(newConnection, buffer, database);
                break;
            // Logout command
            case '2':
                Logout(newConnection, buffer);
                break;
            // List_User command
            case '4':
                List_User(newConnection, database);
                break;
            // Exit command
            case '5':
                close(newConnection);
                pthread_exit(0);
            default:
                break;
        }
        // Flush the buffer
		memset(&buffer, '\0', sizeof(buffer));
    }
}
int main(int argc, char* argv[]){
    if(argc!=2){
        cout<<"Usage: ./server <server port>"<<endl;
        return 0;
    }
    int server_port=atoi(argv[1]);

    // Initialize the client table
    ClientTable.resize(MAX_CLIENT, "-");

    // Initializing the database
    sqlite3 * database;
    sqlite3_open_v2("user.db", &database, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX, NULL);
    SQL_Initialization(database);

    pthread_t pid[15];
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    
    // Create both TCP and UDP socket
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

    // Build up server information structure
    struct sockaddr_in server_info;
    bzero(&server_info, sizeof(server_info));
    server_info.sin_family = AF_INET;
    server_info.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_info.sin_port = htons(server_port);

    // Binding both TCP and UDP socket to server IP and port
    while(bind(TCP_socket, (struct sockaddr *) &server_info, sizeof(server_info)) < 0){
        cout<<"TCP binding error!"<<endl;
    }
    while(bind(UDP_socket, (struct sockaddr *) &server_info, sizeof(server_info)) < 0){
        cout<<"UDP binding error!"<<endl;
    }

    // TCP waiting for connection
    if(listen(TCP_socket, 10) != 0){
		cout<<"TCP binding error!"<<endl;
	}

    int newConnection = 0;                              // New connection's descriptor
    struct sockaddr_in client_info;               // Declare the client information structure
    socklen_t client_size;                                  // Address structure size
    char buffer[1024];                                         // Buffer for receiving the message
    memset(&buffer, '\0', sizeof(buffer));  // Initializing the buffer

    // Initializing the select function
    fd_set rset;
     FD_ZERO(&rset);
    int maxfdp = max(TCP_socket, UDP_socket) + 1;
            
    while(1){
        FD_SET(TCP_socket, &rset);
        FD_SET(UDP_socket, &rset);
        //cout<<"receiving2"<<endl;
        int rec = select(maxfdp, &rset, NULL, NULL, NULL);

        // If the coming message is passed by TCP
        if (FD_ISSET(TCP_socket, &rset)){
            // Accept the new connection
            newConnection = accept(TCP_socket, (struct sockaddr*)&client_info, &client_size);
            cout<<"New connection."<<endl;
            pthread_create(&pid[clients++],&attr,TCP_connection,(void *)&newConnection);
        }
        
        // If the coming message is passed by UDP
        if (FD_ISSET(UDP_socket, &rset)){
            // Receive the UDP packet
            recvfrom(UDP_socket, buffer, sizeof(buffer), 0, (struct sockaddr*)&client_info, &client_size);
            //cout<<"Client(UDP): "<<buffer<<endl;
            // Classify the message
            switch(buffer[0]){
                // Register command
                case '0':
                    Register(UDP_socket, buffer, client_info, database);
                    break;
                // Whoami command
                case '3':
                    Whoami(UDP_socket, buffer, client_info);
                    break;
                default:
                    break;
            }
            // Flush the buffer
            memset(&buffer, '\0', sizeof(buffer));
        }
	}
    close(TCP_socket);
    close(UDP_socket);

    return 0;
}