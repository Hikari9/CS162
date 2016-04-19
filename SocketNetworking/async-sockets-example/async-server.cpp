/**
 * A sample class extension for the server that
 * can asynchronously accepts clients, C++ 11 style.
 * 
 * Compile with: g++ async-server.cpp -pthread -std=c++11 -o async-server
 */

#include <iostream>			// std::cin, std::cout, std::string, std::getline()
#include <cstdio>			// std::printf
#include <memory>			// std::shared_ptr
#include <thread>			// std::thread
#include <vector>			// std::vector
#include <functional>		// std::function
#include "../net_server.hpp"	// net::server, net::socket, net::socket_exception
#include "../net_socketstream.hpp" // net::isocketstream

using namespace std;

struct async_server : public net::server {

	async_server(unsigned short port): net::server(port) {}

	/**
	 * @brief      asynchronously accepts a new socket using C++ 11 threading.
	 * @param[in]  done  a function that is called when a socket was accepted into the server
	 * @return     std::thread referring to the lightweight process thread that has executed the accept()
	 * @since      C++ 11
	 */
	
	thread accept(function<void(net::socket)> done) {
		return thread([this, &done] {done(net::server::accept());});
	}

};

int main() {
	// setup server at port 4000
	async_server server(4000);
	printf("Server is listening at %s:%d\n", server.ip(), server.port());

	
	// asynchronously listen to four clients
	thread threads[4];
	for (auto& thread_ptr : threads) {

		printf("Accepting a client...\n"); // will be printed 4 times
		            
		// accept the thread
		thread_ptr = server.accept([] (net::socket socket) {
			
			printf("Thread has accepted client [sockfd=%d]\n", (int) socket);
			
			// wrap this socket with a stream
			net::isocketstream oss(socket);
			
			// receive some messages from the client socket
			string message;
			while (getline(oss, message))
				printf("[%d]: %s\n", (int) socket, message.c_str());

			printf("Client [%d] has disconnected\n", (int) socket);

		});

	}

	// wait for all threads to finish, ala daemon
	for (auto& thread_ptr : threads)
		thread_ptr.join();

	return 0;
}