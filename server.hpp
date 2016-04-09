/**
 * server class for unix server systems.
 * @author Rico Tiongson
 */


#ifndef _INCLUDE_NETWORKING_SERVER
#define _INCLUDE_NETWORKING_SERVER 1

// C++ stl imports
#include <iostream>
#include <string>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <algorithm>

// linux socket
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

 // networking socket
#include "client.hpp"

namespace net {

	using namespace std;

	// wrapper class for creating a server with a socket
	// essentially contains a list of client sockets and abstracts "send all" functions
	class server : public vector<client> {
	private:

		// have a socket that wraps the server
		client server_socket;
		int _backlog; // the maximum number of allowed queued connections

		// a wrapper class for a server's client
		// deletes itself in the server's list when close() is called
		class server_client : public client {
		private:
			server* service;
		public:
			// overload constructor to call accept()
			server_client(server* service): service(service), client() {
				// connect to client by initiating an accept protocol
				sockaddr_storage clientaddr;
				socklen_t sin_size;
				_port = service->port();
				_id = ::accept(service->id(), (sockaddr*) &clientaddr, &sin_size);
				if (_id == -1) {
					perror("server::accept()");
					throw service;
				}
				// update semaphore keys for reading and writing mutex
				reading.key(id() << 1 | 0);
				writing.key(id() << 1 | 1);
			}


		};

	public:

		// getters
		inline int id() const {return server_socket.id();}
		inline int port() const {return server_socket.port();}
		inline int backlog() const {return _backlog;}
		inline bool is_open() const {return _backlog >= 0;}
		inline int ipver() const {return server_socket.ipver();}
		inline string host() const {return server_socket.host();}
		inline string ip_address() const {return server_socket.ip_address();}

		// constructors
		server(): server_socket(), _backlog(-1) {}
		server(int port, int backlog = SOMAXCONN): server_socket() {open(port, backlog);}

		// open a new port on the server with maximum of backlog number of queued connections
		// does not close the old server
		void open(int port, int backlog = SOMAXCONN) {
			server_socket.open(port);
			_backlog = backlog;
			if (listen(id(), _backlog = max(0, backlog)) == -1) {
				cerr << "server::open(): " << strerror(errno) << endl;
				throw this;
			}
		}

		void close() {
			if (is_open()) {
				for (int i = 0; i < size(); ++i)
					(*this)[i].close();
				server_socket.close();
			}
		}

		// accept a client then appends it to this server
		client& accept() {
			if (!is_open()) {
				cerr << "server::accept(): open a port first" << endl;
				throw this;
			}
			client *cl = new server_client(this);
			this->push_back(*cl);
			return *cl;
		}

		// sends a message to all clients in the server
		template<typename T>
		void send_all(T message) const {
			// if (fork() == 0) { // TODO: use threads instead of fork()
				for (int i = 0; i < size(); ++i)
					if ((*this)[i].is_open()) {
						cout << "Sending to " << (*this)[i].id() << endl;
						(*this)[i].write(message);
					}
				// exit(0);
			// }
		}

		// sends a message to all clients with specified number of bytes 
		template<typename T>
		void send_all(T* message, size_t bytes, int flags = 0) const {
			// if (fork() == 0) { // TODO: use threads instead of fork()
				for (int i = 0; i < size(); ++i)
					if ((*this)[i].is_open())
						(*this)[i].write(message, bytes, flags);
				// exit(0);
			// }
		}

	};

	// output stream overload
	ostream& operator << (ostream& out, const server& s) {
		return out << s.host() << ':' << s.port();
	}

}

#endif /* _INCLUDE_NETWORKING_SERVER */