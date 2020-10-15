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

#define MAX_CLIENT 10

using namespace std;
sqlite3 * log;
string quotesql( const string& s ) {
    return string("'") + s + string("'");
}
sqlite3 * SQL_Initialization(){
    sqlite3 *db;
    char *zErrMsg = 0;
    int database = sqlite3_open("user.db", &db);
    sqlite3_busy_timeout(db,5000);

    database = sqlite3_open("online.db", &log);
    sqlite3_busy_timeout(log,5000);

    string sql = "CREATE TABLE USERS("
                "UID INTEGER PRIMARY KEY AUTOINCREMENT,"
                "Username TEXT NOT NULL UNIQUE,"
                "Email TEXT NOT NULL,"
                "Password TEXT NOT NULL );";
    
    database = sqlite3_exec(db, sql.c_str(), 0, 0, &zErrMsg);

    if( database != SQLITE_OK ){
        //fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

    char *zErrMsg2 = 0;
    string sql2 = "CREATE TABLE LOGIN("
                        "UID INTEGER PRIMARY KEY AUTOINCREMENT,"
                        "Num TEXT NOT NULL UNIQUE,"
                        "Username TEXT NOT NULL);";
    database = sqlite3_exec(log, sql2.c_str(), 0, 0, &zErrMsg2);
    
    if( database != SQLITE_OK ){
        //fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    return db;
}
void Register(int UDP_socket, char* buffer, struct sockaddr_in &client_info, sqlite3 * database){

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
        //fprintf(stderr, "SQL error: %s\n", zErrMsg);
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
    //cout<<"here"<<endl;
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
            //while(1){
                srand( time(NULL) );
                random_number = rand() % MAX_CLIENT;
                //cout<<random_number<<endl;
                string t=to_string(random_number);
                string sql2 = "INSERT INTO LOGIN (Num,Username) VALUES ("
                                    + quotesql(t) + ", "
                                    + quotesql(usr) + ");";
                char *zErrMsg2=0;
                int fd2 = sqlite3_exec(log, sql2.c_str(), 0, 0, &zErrMsg2);
               if( fd2 != SQLITE_OK ){
                    /*cout<<"LOGIN"<<endl;
                    fprintf(stderr, "SQL error: %s\n", zErrMsg2);*/
                    sqlite3_free(zErrMsg2);
                }
                /*else{
                    break;
                }*/
            //}
            //cout<<"here"<<endl;
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
    string tmp="Bye, ";
    int len = tmp.size();
    char back[len+1];
    memset(&back, '\0', sizeof(back));
    strcpy(back, tmp.c_str());
    int random_number = buffer[2]-'0';

    char *zErrMsg;
    sqlite3_stmt *Stmt;
    string sql = "SELECT Username from LOGIN WHERE Num = ?";
    int fd = sqlite3_prepare_v2(log, sql.c_str(), -1, &Stmt, 0);
    if( fd != SQLITE_OK ){
        //fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } 
    else{
        sqlite3_bind_text(Stmt,1,to_string(random_number).c_str(),-1,SQLITE_TRANSIENT);
    }
    int step = sqlite3_step(Stmt);
    
    if (step == SQLITE_ROW) {
        char* ret = (char*)sqlite3_column_text(Stmt,0);
        strcat(back,ret);
        strcat(back,".");
    }

    char *zErrMsg2;
    sqlite3_stmt *Stmt2;
    string sql2 = "DELETE from LOGIN WHERE Num = ?";
    int fd2 = sqlite3_prepare_v2(log, sql.c_str(), -1, &Stmt, 0);
    if( fd2 != SQLITE_OK ){
        //fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } 
    else{
        sqlite3_bind_text(Stmt,1,to_string(random_number).c_str(),-1,SQLITE_TRANSIENT);
    }
    // Convert the sting into char*
    int back_len = sizeof(back);
    char send_buffer[back_len+1];
    memset(&send_buffer, '\0', sizeof(send_buffer));
    strcpy(send_buffer, back);
    send(newConnection, send_buffer, strlen(send_buffer), 0);
}
void Whoami(int UDP_socket, char* buffer, struct sockaddr_in &client_info){

    int random_number = buffer[2]-'0';
    char *zErrMsg;
    sqlite3_stmt *Stmt;
    string sql = "SELECT Username from LOGIN WHERE Num = ?";
    int fd = sqlite3_prepare_v2(log, sql.c_str(), -1, &Stmt, 0);
    if( fd != SQLITE_OK ){
        //fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } 
    else{
        sqlite3_bind_text(Stmt,1,to_string(random_number).c_str(),-1,SQLITE_TRANSIENT);
    }
    int step = sqlite3_step(Stmt);
    
    if (step == SQLITE_ROW) {
        char* ret = (char*)sqlite3_column_text(Stmt,0);
        // Convert the sting into char*
        int back_len=sizeof(ret);
        char send_buffer[back_len+1];
        memset(&send_buffer, '\0', sizeof(send_buffer));
        strcpy(send_buffer, ret);
        // Send the result back to the client
        sendto(UDP_socket, send_buffer, sizeof(send_buffer), 0, (struct sockaddr*)&client_info, sizeof(client_info));
    }
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
int main(int argc, char* argv[]){
    if(argc!=2){
        cout<<"Usage: ./server <server port>"<<endl;
        return 0;
    }
    int server_port=atoi(argv[1]);
    
    // Initializing the database
    sqlite3 * database = SQL_Initialization();

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
    if(listen(TCP_socket, 10) == 0){
		//cout<<"Listening"<<endl;
	}
    else{
		cout<<"TCP binding error!"<<endl;
	}

    int newConnection = 0;                              // New connection's descriptor
    struct sockaddr_in client_info;               // Declare the client information structure
    socklen_t client_size;                                  // Address structure size
    pid_t childpid;                                                // Used to determine whether the process is child or not
    char buffer[1024];                                         // Buffer for receiving the message
    memset(&buffer, '\0', sizeof(buffer));  // Initializing the buffer

    while(1){
        // Accept the new connection
        newConnection = accept(TCP_socket, (struct sockaddr*)&client_info, &client_size);
        cout<<"New connection."<<endl;

       // Create new process for new client
       if((childpid = fork()) == 0){
			// Close the duplicate socket
            close(TCP_socket);
            // Initializing the select function
            fd_set rset;
            FD_ZERO(&rset);
            int maxfdp = max(newConnection, UDP_socket) + 1;
            while(1){
                FD_SET(newConnection, &rset);
                FD_SET(UDP_socket, &rset);
                //cout<<"receiving2"<<endl;
                int rec = select(maxfdp, &rset, NULL, NULL, NULL);

                // If the coming message is passed by TCP
                if (FD_ISSET(newConnection, &rset)){
                    // Receive the TCP packet
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
                            return 0;
                        default:
                            break;
                    }
					//send(newConnection, buffer, strlen(buffer), 0);
                    // Flush the buffer
					memset(&buffer, '\0', sizeof(buffer));
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
                else{
                    //cout<<"WOW"<<endl;
                }
			}
		}
    }
    close(newConnection);
    close(UDP_socket);
    sqlite3_close(database);

    return 0;
}