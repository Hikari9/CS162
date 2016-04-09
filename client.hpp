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

namespace net {

	// uses the std namespace
	using namespace std;

	// a wrapper class for creating a network socket
	// object oriented version with id(), port(), ip_address(), close(), and send()
	class client {
	protected:
		int _port, _id;
		semaphore writing, reading;
		addrinfo* info;

	public:

		// getters
		inline int id() const {return _id;}
		inline int port() const {return _port;}
		inline bool is_open() const {return id() != 0;}
		inline int ipver() const {return is_open() ? (info->ai_family == AF_INET ? 4 : 6) : 0;}
		inline string host() const;
		inline string ip_address() const;

		// boolean checker, synonymous to is_open()
		operator bool() const {return is_open();}

		// constructors
		// if host is NULL, then ip will bind host
		client(): _id(0) {}
		client(const char* host, int port): _id(0) {open(host, port);}
		client(const string& host, int port): _id(0) {open(host, port);}

		// note that the socket file descriptor will only close
		// if the current socket is the last socket
		// so this is relatively safe, especially for object cloning
		virtual ~client() {close();}

		// openers and closers
		void open(const char* host, int port);
		inline void open(const string& host, int port) {return open(host.empty() ? NULL : host.c_str(), port);}
		inline void open(int port) {open(NULL, port);}
		virtual void close();

		// write information
		template<typename T> void write(T* info, size_t bytes, int flags = 0) const;
		template<typename T> inline void write(T) const;

		// read information
		template<typename T> void read(T*, size_t bytes, int flags = 0) const;
		template<typename T> inline void read(T&, int flags = 0) const;
		template<typename T> inline T read() const;
		inline void read(char*) const;

		// stream-like send (<<) and receive (>>)
		template<typename T> inline client& operator << (const T& info) {write(info); return *this;}
		template<typename T> inline client& operator >> (T& variable) {read(variable); return *this;}

	};

	// close socket file descriptor and free the service pointer
	void client::close() {
		if (is_open()) {
			if (::close(id()) == -1)
				fprintf(stderr, "[%d] client::close(): %s\n", id(), strerror(errno));
			_id = 0;
		}
	}

	// open/reopen the port specified
	void client::open(const char* host, int port) {
		close();

		addrinfo hints, *service;
		_port = port;

		// set all members of hints to 0
		memset(&hints, 0, sizeof(hints));

		// specify flags of hints
		hints.ai_family = AF_UNSPEC; // can also be AF_INET or AF_INET6
		hints.ai_socktype = SOCK_STREAM; // use stream sockets
		hints.ai_flags = host ? AI_CANONNAME : AI_PASSIVE ; // socket will be used to listen to connections passively

		char *chrport = new char[10];
		sprintf(chrport, "%d", port);

		// get all possible addresses we can use to create a socket
		// NULL for first argument since we are going to listen for connections
		int status = getaddrinfo(host, chrport, &hints, &service);
		delete[] chrport;
		
		if (status != 0) {
			fprintf(stderr, "[%d] client::client()::getaddrinfo(): %s\n", id(), gai_strerror(status));
			throw this;
		}

		// go through results of getaddrinfo()
		// use the first possible address we can use
		_id = -1;
		for (info = service; info; info = info->ai_next) {
			
			// attempt to create a socket
			_id = ::socket(info->ai_family, info->ai_socktype, info->ai_protocol);
			if (_id == -1) {
				fprintf(stderr, "[%d] client::client()::socket(): %s\nAttempting another socket...\n", id(), strerror(errno));
				perror("socket()");
				continue;
			}

			// attempt to bind or connect to a port
			// if no host, then bind, else connect
			if ((host ? connect : bind)(_id, info->ai_addr, info->ai_addrlen) == 0)
				// binded with a socket
				break;

			fprintf(stderr, "[%d] client::client()::bind(): %s\nAttempting another socket...\n", id(), strerror(errno));
			// close socket if failed to load
			_id = -1;

		}
		
		// no longer needed
		// ::freeaddrinfo(service);

		// last check: if socket is really ok (in case loop was never touched)
		if (_id == -1) {
			fprintf(stderr, "[] client::client(): unable to create a socket\n");
			throw this;
		}

		// update semaphore keys for reading and writing mutex
		reading.key((id() << 1) | 0);
		writing.key((id() << 1) | 1);
	}

	// get the IP Adress string from socket file descriptor (sockfd)
	// @see http://man7.org/linux/man-pages/man2/getpeername.2.html
	string client::ip_address() const {
		sockaddr_in addr; // dummy address handler
		socklen_t addr_size = sizeof(addr);
		if (getsockname(id(), (sockaddr *) &addr, &addr_size) == -1) {
			perror("ip_address(): ");
			throw this;
		}
		return inet_ntoa(addr.sin_addr);
	}

	// synonym: get host name through ip address
	string client::host() const {
		return ip_address();
	}

	// send information with number of bytes
	template<typename T>
	void client::write(T* info, size_t bytes, int flags) const {
		char* buffer = (char*) info;
		writing.wait();
		for (ssize_t sent = 0, next = 0; sent < bytes; sent += next) {
			next = send(id(), buffer += next, bytes - sent, flags);
			if (next == -1) {
				// an error occured in writing, raise an error message
				writing.signal();
				fprintf(stderr, "[%d] client::write(): %s\n", id(), strerror(errno));
				throw this;
			}
		}
		writing.signal();
	}

	// send information without specifying number of bytes
	template<typename T>
	inline void client::write(T info) const {
		write(&info, sizeof(T));
	}

	// special send: char array
	template<>
	inline void client::write(char info[]) const {
		write(info, strlen(info) + 1);
	}

	// special send: const char array
	template<>
	inline void client::write(const char info[]) const {
		write(info, strlen(info) + 1);
	}

	// special send: std::string
	template<>
	inline void client::write(string info) const {
		write(info.c_str(), info.length() + 1);
	}

	// receive information with number of bytes
	template<typename T>
	void client::read(T* buf, size_t bytes, int flags) const {
		char* buffer = (char*) buf;
		reading.wait();
		for (ssize_t received = 0, next = 0; received < bytes; received += next, buffer += next) {
			next = recv(id(), buf, bytes, flags);
			if (next == -1) {
				// an error occured in reading, raise an error message
				reading.signal();
				fprintf(stderr, "[%d] client::read(): %s\n", id(), strerror(errno));
				throw this;
			}
		}
		reading.signal();
	}

	// receive information and put it into a variable
	template<typename T>
	inline void client::read(T& variable, int flags) const {
		this->read(&variable, sizeof(T), flags);
	}

	// special receive: read into a character array without specifying number of bytes
	// read until '\0' sentinel is encountered
	inline void client::read(char* buffer) const {
		reading.wait();
		while (true) {
			// read 1 character until '\0'
			ssize_t next = ::recv(id(), buffer, 1, 0);
			if (next == -1) {
				// an error occured in reading, raise an error message
				reading.signal();
				fprintf(stderr, "[%d] client::read(): %s\n", id(), strerror(errno));
				throw this;
			}
			if (next && *(buffer++) == '\0')
				break;
		}
		reading.signal();
	}

	// special receive: read into a string without specifying number of bytes
	// read until '\0' sentinel is encountered
	template<>
	inline void client::read(string& buffer, int flags) const {
		buffer.clear();
		reading.wait();
		char *temp = new char();
		while (true) {
			// read 1 character until '\0'
			ssize_t next = ::recv(id(), (void*) temp, 1, 0);
			if (next == -1) {
				// an error occured in reading, raise an error message
				reading.signal();
				fprintf(stderr, "[%d] client::read(string&): %s\n", id(), strerror(errno));
				throw this;
			}
			if (next && *temp == '\0')
				break;
			buffer.push_back(*temp);
		}
		reading.signal();
	}


	// receive anonymous information by data type
	// @example int x = socket.read<int>();
	template<typename T>
	inline T client::read() const {
		T t; read(t); return t;
	}

	// receive anonymous string
	template<>
	inline string client::read<string>() const {
		string buffer;
		this->read(buffer);
		return buffer;
	}

	// receive anonymous char*
	template<>
	inline char* client::read<char*>() const {
		string buffer;
		this->read(buffer);
		char *cbuffer = new char[buffer.length() + 1];
		strcpy(cbuffer, buffer.c_str());
		return cbuffer;
	}

	// receive anonymous const char*
	template<>
	inline const char* client::read<const char*>() const {
		string buffer;
		this->read(buffer);
		return buffer.c_str();
	}

	// output stream overload operator
	ostream& operator << (ostream& out, const client& c) {
		return out << c.host() << ':' << c.port();
	}

}

#endif /*_INCLUDE_NETWORKING_SOCKET*/