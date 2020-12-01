all: client.cpp server.cpp chat.cpp
	g++ client.cpp -o client -lpthread
	g++ server.cpp -o server -l sqlite3 -lpthread
	g++ chat.cpp -o chat -lpthread