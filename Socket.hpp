/**
 * Socket class for Web-Unix systems.
 * @author Rico Tiongson
 */


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

// concurrency
#include "semaphore.hpp"

namespace networking {

	// uses the std namespace
	using namespace std;

	// get the IP Adress string from socket id (sockfd)
	// @see http://man7.org/linux/man-pages/man2/getpeername.2.html
	string ip_address(int sockfd) {
		sockaddr_in addr;
		socklen_t addr_size = sizeof(sockaddr_in);
		if (getpeername(sockfd, (struct sockaddr *) &addr, &addr_size) == -1)
			// return the error
			return string("ip_address(): ") + strerror(errno);
		return inet_ntoa(addr.sin_addr);
	}

	// a wrapper class for creating a network socket
	// object oriented version with id(), port(), ip_address(), close(), and send()
	class Socket {
	private:
		int _port, _id;
		addrinfo hints, *service, *info;
		semaphore *writing, *reading;

	public:

		// getters
		inline int id() const {return _id;}
		inline int port() const {return _port;}
		inline string ip_address() const {return networking::ip_address(id());}

		// free socket through destructor, even if on force shutdown
		~Socket() {
			::freeaddrinfo(service);
			::close(id());
			if (writing) delete writing;
			if (reading) delete reading;
		}

		// write information
		template<typename T> void write(T* info, size_t bytes, int flags = 0) const;
		template<typename T> inline void write(T) const;

		// read information
		template<typename T> void read(T*, size_t bytes, int flags = 0) const;
		template<typename T> inline void read(T&, int flags = 0) const;
		template<typename T> inline T read() const;
		inline void read(char*) const;

		// atomic reply getter
		// template<typename T1, typename T2> T1 reply(T2) const;


		// stream-like send (<<) and receive (>>)
		template<typename T> inline Socket& operator << (const T& info) {write(info); return *this;}
		template<typename T> inline Socket& operator >> (T& variable) {read(variable); return *this;}

		// construct a socket via port
		Socket(int port): _port(port) {

			// set all members of hints to 0
			memset(&hints, 0, sizeof(hints));

			// specify flags of hints
			hints.ai_family = AF_UNSPEC; // can also be AF_INET or AF_INET6
			hints.ai_socktype = SOCK_STREAM; // use stream sockets
			hints.ai_flags = AI_PASSIVE; // socket will be used to listen to connections passively

			char *chrport = new char[10];
			sprintf(chrport, "%d", port);

			// get all possible addresses we can use to create a socket
			// NULL for first argument since we are going to listen for connections
			int status = getaddrinfo(NULL, chrport, &hints, &service);
			delete[] chrport;
			
			if (status != 0) {
				cerr << "getaddrinfo(): " << gai_strerror(status) << endl;
				throw this;
			}

			// go through results of getaddrinfo()
			// use the first possible address we can use
			_id = -1;
			for (info = service; info; info = info->ai_next) {
				
				// attempt to create a socket
				_id = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
				if (_id == -1) {
					perror("socket()");
					continue;
				}

				// attempt to bind to a port
				if (bind(_id, info->ai_addr, info->ai_addrlen) == -1) {
					perror("bind()");
					continue;
				}

				// we have binded a socket
				break;

			}

			// last check: if socket is really ok (in case loop was never touched)
			if (_id == -1) {
				cerr << "Unable to create a socket!" << endl;
				throw this;
			}

			// setup reading and writing semaphores for mutex
			this->reading = new semaphore(_id << 1 | 0);
			this->writing = new semaphore(_id << 1 | 1);

		}
	};

	// send information with number of bytes
	template<typename T>
	void Socket::write(T* info, size_t bytes, int flags) const {
		char* buffer = (char*) info;
		writing->wait();
		for (ssize_t sent = 0; sent < bytes; buffer += sent) {
			ssize_t next = send(id(), (void*) buffer, bytes - sent, flags);
			if (next == -1) {
				// an error occured in writing, raise an error message
				writing->signal();
				cerr << "Socket::write(): " << strerror(errno) << endl;
				throw this;
			}
			sent += next;
		}
		writing->signal();
	}

	// send information without specifying number of bytes
	template<typename T>
	inline void Socket::write(T info) const {
		write(&info, sizeof(T));
	}

	// special send: char array
	template<>
	inline void Socket::write(char info[]) const {
		write(info, strlen(info) + 1);
	}

	// special send: const char array
	template<>
	inline void Socket::write(const char info[]) const {
		write(info, strlen(info) + 1);
	}

	// special send: std::string
	template<>
	inline void Socket::write(string info) const {
		write(info.c_str(), info.length() + 1);
	}

	// receive information with number of bytes
	template<typename T>
	void Socket::read(T* buf, size_t bytes, int flags) const {
		char* buffer = (char*) buf;
		reading->wait();
		for (ssize_t received = 0; received < bytes; buffer += received) {
			ssize_t next = recv(id(), buffer, bytes - received, flags);
			if (next == -1) {
				// an error occured in reading, raise an error message
				reading->signal();
				cerr << "Socket::read(): " << strerror(errno) << endl;
				throw this;
			}
		}
		reading->signal();
	}

	// receive information and put it into a variable
	template<typename T>
	inline void Socket::read(T& variable, int flags) const {
		this->read(&variable, sizeof(T), flags);
	}

	// special receive: read into a character array without specifying number of bytes
	// read until '\0' sentinel is encountered
	inline void Socket::read(char* buffer) const {
		reading->wait();
		while (true) {
			// read 1 character until '\0'
			ssize_t next = ::recv(id(), buffer, 1, 0);
			if (next == -1) {
				// an error occured in reading, raise an error message
				reading->signal();
				cerr << "Socket::read(): " << strerror(errno);
				throw this;
			}
			if (next && *(buffer++) == '\0')
				break;
		}
		reading->signal();
	}

	// special receive: read into a string without specifying number of bytes
	// read until '\0' sentinel is encountered
	template<>
	inline void Socket::read(string& buffer, int flags) const {
		buffer.clear();
		reading->wait();
		char *temp = new char();
		while (true) {
			// read 1 character until '\0'
			ssize_t next = ::recv(id(), (void*) temp, 1, 0);
			if (next == -1) {
				// an error occured in reading, raise an error message
				reading->signal();
				cerr << "Socket::read(string&): " << strerror(errno) << endl;
				throw this;
			}
			if (next && *temp == '\0')
				break;
			buffer.push_back(*temp);
		}
		reading->signal();
	}


	// receive anonymous information by data type
	// @example int x = socket.read<int>();
	template<typename T>
	inline T Socket::read() const {
		void* info = malloc(sizeof(T));
		this->read(info);
		return *(T*) info;
	}

	// receive anonymous string
	template<>
	inline string Socket::read<string>() const {
		string buffer;
		this->read(buffer);
		return buffer;
	}

	// receive anonymous char*
	template<>
	inline char* Socket::read<char*>() const {
		string buffer;
		this->read(buffer);
		char *cbuffer = new char[buffer.length() + 1];
		strcpy(cbuffer, buffer.c_str());
		return cbuffer;
	}

	// receive anonymous const char*
	template<>
	inline const char* Socket::read<const char*>() const {
		string buffer;
		this->read(buffer);
		return buffer.c_str();
	}

}
