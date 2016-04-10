#include <iostream>
#include <cstdio>
#include "net.hpp"

using namespace std;

int main(int argc, char* argv[]) {

	if (argc < 3) {
		printf("Some missing arguments\n");
		printf("Format: %s <host> <port>\n", argv[0]);
		return 0;
	}

	char* host = argv[1];
	char* port = argv[2];
	printf("Connecting to %s (Port %s)...\n", host, port);
	// net::client client(host, atoi(port));
	net::client client;
	try {
		client = net::client(host, atoi(port));
	} catch (net::client* ex) {
		fprintf(stderr, "Server at %s:%s was not found.\n", host, port);
		return 1;
	}
	printf("Connected [%d]\n", client.sock);
	while (true) {
		printf("Send a message: ");
		string message;
		getline(cin, message);
		client.send(message);
		printf("Waiting for server's reply...\n");
		client.read(message);
		cout << "Server: " << message << endl;
	}

}