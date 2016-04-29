// multiple chat linux client, for CS 162 Lab 10 requirement
// compile: g++ chat.cpp -o chat 
#include <iostream>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <pthread.h>
#include <termios.h>  // getch()
#include <unistd.h>
#include "../net_client.hpp"

using namespace std;

// get character not until '\n'
char getch(){
    char buf=0;
    struct termios old={0};
    fflush(stdout);
    if(tcgetattr(0, &old)<0)
        perror("tcsetattr()");
    old.c_lflag&=~ICANON;
    old.c_lflag&=~ECHO;
    old.c_cc[VMIN]=1;
    old.c_cc[VTIME]=0;
    if(tcsetattr(0, TCSANOW, &old)<0)
        perror("tcsetattr ICANON");
    if(read(0,&buf,1)<0)
        perror("read()");
    old.c_lflag|=ICANON;
    old.c_lflag|=ECHO;
    if(tcsetattr(0, TCSADRAIN, &old)<0)
        perror ("tcsetattr ~ICANON");
    if (buf == 127) {
        printf("\b ");
        buf = '\b';
    }
    printf("%c", buf);
    return buf;
}

net::client client;
char *name;
string inbuffer;
int sockfd;
#define label "(" << sockfd << ")[" << name << "]: "

void* listener(void* args) {
	string message;
	while (client.read(message)) {
		// pad some spaces
		int len = inbuffer.length();
		cout << string(len, '\b') << string(len, ' ') << '\r' << message;
		cout << '\n' << label << inbuffer << flush;
	}
}

int main(int argc, char* argv[]) {
	if (argc < 4) {
		printf("Some missing arguments\n");
		printf("Format: %s <server_ip> <port> <name>\n", argv[0]);
		return 0;
	}
	char* host = argv[1];
	char* port = argv[2];
	name = argv[3];
	// try connecting to the server indefinitely with a 5 second timeout
	while (!client) {
		printf("Connecting to server at %s:%s...\n", host, port);
		try {client = net::client(host, atoi(port));}
		catch (net::socket_exception) {sleep(3);}
	}
	// send name to server
	printf("Sending client information...\n");
	client.send(name);
	// wait for an OK from the server
	bool ok = client.read<bool>();
	if (!ok) {
		// print the error message from the server
		cerr << client.read<string>() << endl;
		return EXIT_FAILURE;
	}
	// get your socket number from the server
	client.read(sockfd);
	// MAIN CHAT: create a listening process
	int error;
	pthread_t thread;
	if (error = pthread_create(&thread, NULL, &listener, NULL)) {
		errno = error;
		perror("pthread_create()");
	}
	while (client && !feof(stdin)) {
		char c = getch();
		if (c == '\b' && !inbuffer.empty())
			inbuffer.erase(inbuffer.length() - 1);
		else if (c == '\n') {
			if (!inbuffer.empty()) {
				client.send(inbuffer);
				if (inbuffer == "@exit")
					break;
				inbuffer.clear();
				cout << label << flush;
			}
		}
		else 
			inbuffer.push_back(c);
	}
}
