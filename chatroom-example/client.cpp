// compile: g++ client.cpp -o client 
#include <iostream>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include "../net_client.hpp"

using namespace std;

int main(int argc, char* argv[]) {
	if (argc < 3) {
		printf("Some missing arguments\n");
		printf("Format: %s <host> <port>\n", argv[0]);
		return 0;
	}
	char* host = argv[1];
	char* port = argv[2];
	// ask for client's name for chat
	printf("Enter your name: ");
	string name;
	getline(cin, name);
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
	// receive ping back if ok
	if (client.read<bool>() == false) {
		// get server's error message
		cout << "Server: " << client.read<string>() << endl;
		return EXIT_FAILURE;
	}
	// create a listening process
	pid_t f = fork();
	if (f == -1)
		perror("fork()");
	else if (f) {
		// child process, receive messages
		string message;
		while (client.read(message))
			cout << "\r" << message << '\n' << name << ": " << flush;
	} else {
		// parent process, send messages through console input
		string message;
		while (getline(cin, message)) {
			client.send(message);
			cout << name << ": ";
		}
	}
}
