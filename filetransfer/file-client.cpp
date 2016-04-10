/**
 * Send a file to a server through network.
 */
#include <iostream>
#include <fstream>
#include <iterator>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "../net.hpp"

using namespace std;
const int PORT = 20050;

int main(int argc, char* argv[]) {
	// get arguments
	if (argc < 3) {
		printf("Some missing arguments\n");
		printf("Format: %s <host> <filename>\n", argv[0]);
		return 0;
	}
	char* host = argv[1];
	char* filename = argv[2];
	// retrieve file
	ifstream file(filename);
	if (!file) {
		printf("File not found\n");
		return EXIT_FAILURE;
	}
	// connect to the server
	net::client client;
	try {client = net::client(host, PORT);}
	catch (net::client*) {
		printf("Could not connect to server\n");
		return EXIT_FAILURE;
	}
	// get size of file
	file.seekg(0, file.end);
	int bytes = file.tellg();
	file.seekg(0);
	// read file to memory
	char* buffer = (char*) malloc(bytes);
	file.read(buffer, bytes);
	file.close();
	// send file and buffer
	client.send(bytes);
	client.send(buffer, bytes);
	free(buffer);
	// receive a ping back that all is ok
	printf("Waiting for upload to finish...\n");
	if (client.read<bool>()) printf("Uploaded file to server\n");
	else printf("An error occured in uploading %s\n", filename);
}