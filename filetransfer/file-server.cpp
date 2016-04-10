/**
 * Receive a file from a client through the network.
 */
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "../net.hpp"
#include "../netinfo.hpp"

using namespace std;
const int PORT = 20050;

int main(int argc, char* argv[]) {
	// get arguments
	if (argc == 1) {
		printf("Some missing arguments\n");
		printf("Format: %s <filename>\n", argv[0]);
		return 0;
	}
	char* filename = argv[1];
	// open server for connections
	printf("Setting up server...\n");
	net::server server(PORT);
	printf("Server is at %s\n", net::ip_address().c_str());
	printf("Waiting for client...\n");
	net::client client = server.accept();
	// get the number of bytes to receive first
	int bytes = client.read<int>();
	char* buffer = (char*) malloc(bytes);
	// write into the buffer
	try {client.read(buffer, bytes);}
	catch (net::client*) {
		printf("Connection failed\n");
		free(buffer);
		return EXIT_FAILURE;
	}
	printf("Writing to file...\n");
	// write to file
	try {
		ofstream file(filename);
		file.write(buffer, bytes);
		client << true;
		printf("Downloaded file to %s\n", filename);
	} catch (...) {
		printf("Could not write to file\n");
		client << false;
	}
	free(buffer);
}