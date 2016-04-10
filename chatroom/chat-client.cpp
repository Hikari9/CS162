#include <iostream>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include "../net.hpp"

using namespace std;

// client information
string name;
net::client client;

// listener to server's messages
void* listener(void*);
string hrule(40, '-');

int main(int argc, char* argv[]) {
	if (argc < 3) {
		printf("Some missing arguments\n");
		printf("Format: %s <host> <port>\n", argv[0]);
		return 0;
	}
	char* host = argv[1];
	char* port = argv[2];
	// ask for client's name for chat
	cout << "Enter your name: ";
	getline(cin, name);
	// try connecting to the server indefinitely with a 5 second timeout
	while (true) {
		printf("Connecting to server at %s:%s...\n", host, port);
		try {
			client = net::client(host, atoi(port));
			break;
		} catch (net::client*) {
			sleep(5);
		}
	}
	// send name to server
	cout << "Sending client information..." << endl;
	client << name;
	// receive ping back
	bool ping = client.read<bool>();
	if (ping) {
		cout << hrule << endl;
		// create a listening thread
		pthread_t listening_thread;
		int error;
		if (error = pthread_create(&listening_thread, NULL, &listener, NULL)) {
			errno = error;
			perror("pthread_create()");
		}
		// chat indefinitely
		string message;
		while (true) {
			cout << name << ": ";
			getline(cin, message);
			if (message == "exit")
				break;
			client << message;
		}
		// cancel listening thread on exit
		if (error = pthread_cancel(listening_thread)) {
			errno = error;
			perror("pthread_cancel()");
		}
	} else {
		cerr << "Server error" << endl;
		return EXIT_FAILURE;
	}
}

void* listener(void* args) {
	string message;
	while (client >> message)
		cout << "\r" << message << '\n' << name << ": " << flush;
}