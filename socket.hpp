/**
 * socket class for unix server systems.
 * @author Rico Tiongson
 */


#ifndef _INCLUDE_NETWORKING_SOCKET
#define _INCLUDE_NETWORKING_SOCKET 1

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

	// a wrapper class for creating a network socket
	// object oriented version with id(), port(), ip_address(), close(), and send()
	class socket {
	private:
		int _port, _id;
		addrinfo hints, *service, *info;
		semaphore *writing, *reading;

	public:

		// getters
		inline int id() const {return _id;}
		inline int port() const {return _port;}
		inline bool is_open() const {return id() != 0;}
		inline int ipver() const {return is_open() ? (info->ai_family == AF_INET ? 4 : 6) : 0;}
		inline string host() const;
		inline string ip_address() const;

		// constructors and destructors
		// if host is NULL, then ip will bind host
		socket(): _id(0) {}
		socket(const char* host, int port): _id(0) {open(host, port);}
		socket(const string& host, int port): _id(0) {open(host, port);}
		~socket() {this->close();}

		// openers and closers
		void open(const char* host, int port);
		inline void open(const string& host, int port) {return open(host.empty() ? NULL : host.c_str(), port);}
		inline void open(int port) {open(NULL, port);}
		void close();

		// write information
		template<typename T> void write(T* info, size_t bytes, int flags = 0) const;
		template<typename T> inline void write(T) const;

		// read information
		template<typename T> void read(T*, size_t bytes, int flags = 0) const;
		template<typename T> inline void read(T&, int flags = 0) const;
		template<typename T> inline T read() const;
		inline void read(char*) const;

		// stream-like send (<<) and receive (>>)
		template<typename T> inline socket& operator << (const T& info) {write(info); return *this;}
		template<typename T> inline socket& operator >> (T& variable) {read(variable); return *this;}

	};

	// close socket file descriptor and free the service pointer
	void socket::close() {
		if (is_open()) {
			::freeaddrinfo(service);
			::close(id());
			_id = 0;
			if (reading) delete reading;
			if (writing) delete writing;
		}
	}

	// open/reopen the port specified
	void socket::open(const char* host, int port) {
		close();
		_port = port;

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
		int status = getaddrinfo(host, chrport, &hints, &service);
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
				// perror("socket()");
				continue;
			}

			// attempt to bind or connect to a port
			// if no host, then bind, else connect
			if ((host ? connect : bind)(_id, info->ai_addr, info->ai_addrlen) == -1) {
				// perror(host ? "connect()" : "bind()");
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

	// open synonyms

	// get the IP Adress string from socket file descriptor (sockfd)
	// @see http://man7.org/linux/man-pages/man2/getpeername.2.html
	string socket::ip_address() const {
		sockaddr_in addr; // dummy address handler
		socklen_t addr_size = sizeof(sockaddr_in);
		if (getpeername(id(), (sockaddr *) &addr, &addr_size) == -1) {
			perror("ip_address(): ");
			throw this;
		}
		return inet_ntoa(addr.sin_addr);
	}

	// synonym: get host name through ip address
	string socket::host() const {
		return ip_address();
	}

	// send information with number of bytes
	template<typename T>
	void socket::write(T* info, size_t bytes, int flags) const {
		char* buffer = (char*) info;
		writing->wait();
		for (ssize_t sent = 0; sent < bytes; buffer += sent) {
			ssize_t next = send(id(), (void*) buffer, bytes - sent, flags);
			if (next == -1) {
				// an error occured in writing, raise an error message
				writing->signal();
				cerr << "socket::write(): " << strerror(errno) << endl;
				throw this;
			}
			sent += next;
		}
		writing->signal();
	}

	// send information without specifying number of bytes
	template<typename T>
	inline void socket::write(T info) const {
		write(&info, sizeof(T));
	}

	// special send: char array
	template<>
	inline void socket::write(char info[]) const {
		write(info, strlen(info) + 1);
	}

	// special send: const char array
	template<>
	inline void socket::write(const char info[]) const {
		write(info, strlen(info) + 1);
	}

	// special send: std::string
	template<>
	inline void socket::write(string info) const {
		write(info.c_str(), info.length() + 1);
	}

	// receive information with number of bytes
	template<typename T>
	void socket::read(T* buf, size_t bytes, int flags) const {
		char* buffer = (char*) buf;
		reading->wait();
		for (ssize_t received = 0; received < bytes; buffer += received) {
			ssize_t next = recv(id(), buffer, bytes - received, flags);
			if (next == -1) {
				// an error occured in reading, raise an error message
				reading->signal();
				cerr << "socket::read(): " << strerror(errno) << endl;
				throw this;
			}
		}
		reading->signal();
	}

	// receive information and put it into a variable
	template<typename T>
	inline void socket::read(T& variable, int flags) const {
		this->read(&variable, sizeof(T), flags);
	}

	// special receive: read into a character array without specifying number of bytes
	// read until '\0' sentinel is encountered
	inline void socket::read(char* buffer) const {
		reading->wait();
		while (true) {
			// read 1 character until '\0'
			ssize_t next = ::recv(id(), buffer, 1, 0);
			if (next == -1) {
				// an error occured in reading, raise an error message
				reading->signal();
				cerr << "socket::read(): " << strerror(errno);
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
	inline void socket::read(string& buffer, int flags) const {
		buffer.clear();
		reading->wait();
		char *temp = new char();
		while (true) {
			// read 1 character until '\0'
			ssize_t next = ::recv(id(), (void*) temp, 1, 0);
			if (next == -1) {
				// an error occured in reading, raise an error message
				reading->signal();
				cerr << "socket::read(string&): " << strerror(errno) << endl;
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
	inline T socket::read() const {
		void* info = malloc(sizeof(T));
		this->read(info);
		return *(T*) info;
	}

	// receive anonymous string
	template<>
	inline string socket::read<string>() const {
		string buffer;
		this->read(buffer);
		return buffer;
	}

	// receive anonymous char*
	template<>
	inline char* socket::read<char*>() const {
		string buffer;
		this->read(buffer);
		char *cbuffer = new char[buffer.length() + 1];
		strcpy(cbuffer, buffer.c_str());
		return cbuffer;
	}

	// receive anonymous const char*
	template<>
	inline const char* socket::read<const char*>() const {
		string buffer;
		this->read(buffer);
		return buffer.c_str();
	}

}

#endif /*_INCLUDE_NETWORKING_SOCKET*/