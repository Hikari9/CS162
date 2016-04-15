/**
 * Receive a file from a client through the network, until when server closes
 */
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "../net_server.hpp"
#include "../net_client.hpp"

using namespace std;

int main(int argc, char* argv[]) {
	// get arguments
	if (argc < 3) {
		printf("Some missing arguments\n");
		printf("Format: %s <port> <filename>\n", argv[0]);
		return 0;
	}
	char* port = argv[1];
	char* filename = argv[2];
	// open server for one connection
	net::server server(atoi(port), 1);
	printf("Server is at %s:%s\n", server.ip(), port);
	printf("Waiting for client...\n");
	net::client client = server.accept();
	printf("Writing to file...\n");
	// number of bytes is unknown
	ofstream file(filename);
	char ch;
	string buffer;
	// write into the buffer
	try {
		while (client.read(ch)) {
			file.put(ch);
			cout << (int) ch << endl;
		}
		file.close();
	}
	catch (exception ex) {
		printf("Receive failed %s\n", ex.what());
		return EXIT_FAILURE;
	}
	printf("Downloaded file to %s\n", filename);
}