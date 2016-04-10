#include <iostream>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include "net.hpp"

using namespace std;

// client's name

// listen to server's messages
void* listener(void* args) {
	net::client *server = (net::client*) args;
	string message;
	while ((*server) >> message) {
		printf("\n%s\nYou: ", message.c_str());
		fflush(stdout);
	}
}

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
	string name; getline(cin, name);
	net::client client;
	// try connecting to the server indefinitely with a 5 second timeout
	do {
		printf("Connecting to server at %s:%s...\n", host, port);
		try {client = net::client(host, atoi(port));}
		catch (net::client* ex) {
			// fprintf(stderr, "Server at %s:%s was not found.\n", host, port);
			sleep(5);
			continue;
		}
	} while (false);
	printf("Sending client information...\n");
	// send name to server
	client << name;
	// receive ping back
	bool ping = client.read<bool>();
	if (ping) {
		cout << string(40, '-') << endl;
		printf("%s has entered the room.\n", name.c_str());
		// create a listening thread
		pthread_t listening_thread;
		int error;
		if (error = pthread_create(&listening_thread, NULL, &listener, &client)) {
			errno = error;
			perror("pthread_create()");
		}
		// chat indefinitely
		while (true) {
			printf("You: ");
			string message;
			getline(cin, message);
			if (message == "exit")
				break;
			client << message;
		}
		// cancle listening thread on exit
		if (error = pthread_cancel(listening_thread)) {
			errno = error;
			perror("pthread_cancel()");
		}
	} else {
		fprintf(stderr, "Server error\n");
		return EXIT_FAILURE;
	}

}
