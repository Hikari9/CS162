// multiple chat linux client, for CS 162 Lab 10 requirement
// compile: g++ chat.cpp -o chat 
#include <iostream>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include "../net_client.hpp"

using namespace std;

int main(int argc, char* argv[]) {
	if (argc < 4) {
		printf("Some missing arguments\n");
		printf("Format: %s <server_ip> <port> <name>\n", argv[0]);
		return 0;
	}
	char* host = argv[1];
	char* port = argv[2];
	char* name = argv[3];
	// try connecting to the server indefinitely with a 5 second timeout
	net::client client;
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
	int sockfd;
	client.read(sockfd);
	#define label "(" << sockfd << ")[" << name << "]: "
	// MAIN CHAT: create a listening process
	if (fork()) {
		// child process, receive messages from server
		string message;
		while (client.read(message))
			cout << '\r' << message << '\n' << label << flush;
	} else {
		// parent process, send messages through console input
		string message;
		while (getline(cin, message)) {
			client.send(message);
			cout << label << flush;
		}
	}
}
