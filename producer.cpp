#include <iostream>  // getline, string
#include <fstream>   // ifstream
#include <cstdio>    // printf, scanf, perror
#include <cstdlib>   // srand, atoi 
#include <ctime>     // time
#include <queue>     // queue
#include "semaphore.hpp"
#include "shared_memory.hpp"

using namespace std;

// assume one producer/consumer
int main(int argc, char* args[]) {
	srand(time(NULL)); // for random

	// check valid parameters
	if (argc < 3) {
		printf("Missing some arguments!\nUsage: %s <textfile> <shared memory size in bytes>", args[0]);
		return 1;
	}

	// get arguments
	char* file = args[1];
	int bytes = atoi(args[2]);
	int sleepTime = argc > 3 ? atoi(args[3]) : 1000;

	// read text file
	string buffer, line;
	ifstream fin(file);
	if (fin.fail()) {
		puts("File does not exist :(");
		return 2;
	}
	bool first = true;
	while (getline(fin, line)) {
		if (first) first = false;
		else buffer += '\n';
		buffer += line;
	}
	fin.close();

	if (buffer.empty()) {
		puts("Buffer is empty :(");
		return 3;
	}

	// enqueue chunks
	queue<string> chunks;
	for (int i = 0; i < buffer.size(); i += bytes)
		chunks.push(buffer.substr(i, bytes));

	// concurrency members
	semaphore access(0xACCE55);
	memory<short> feeding(0xFEEEEED);
	memory<char> food(0xF00000D, bytes);

	puts("File has been read. Preparing for production...");

	enum {IDLE, FEEDING, EXIT};

	// initially idle
	access.wait();
	feeding.write(IDLE);
	access.signal();

	// have a char array buffer for chunk
	char chunk[bytes + 1];

	// main loop
	while (true) {

		access.wait();

		if (feeding.read() == FEEDING) {
			// consumer has yet to eat the food
			access.signal();
			puts("Waiting for a consumer to eat...");
		}

		else if (!chunks.empty()) {
			// feed a new chunk, make sure buffer can contain all bytes
			strcpy(chunk, chunks.front().c_str());
			printf("Feeding (%s)...\n", chunk);

			feeding.write(FEEDING);
			food.write(chunk);

			chunks.pop();
			access.signal();
		}

		else {
			// no more chunks left
			feeding.write(EXIT);
			access.signal();
			puts("No more food to give. Sending an exit signal.");
			break;
		}
		usleep(sleepTime * 1000);
	}

	return 0;
}