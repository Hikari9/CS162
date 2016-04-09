#include <iostream>
#include <cstdio>
#include "net.hpp"

using namespace std;

int main(int argc, char* argv[]) {

	if (argc == 1) {
		printf("Some missing arguments\n");
		printf("Format: %s <port>\n", argv[0]);
		return 0;
	}

	// create server using port
	net::server server(atoi(argv[1]));
	printf("Created server at port %s [%d]\n", argv[1], server.sock);
	printf("Accepting client...\n");

	// accept clients through net::server::accept()
	net::client client = server.accept();
	printf("Client found at socket[%d]\n", client.sock);

	while (true) {
		// receive a message from the client
		printf("Waiting for client's message...\n");
		cout << "Client: " << client.read<string>() << endl;
		printf("Enter your reply: ");
		string reply;
		getline(cin, reply);
		client.send(reply);
	}

}