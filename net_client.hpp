/**
 * A lightweight socket wrapper for a network client (IPv4), using low-level
 * POSIX sockets. Extends the net::socket class. The net::client object
 * supports communication between the server and the client using bytes
 * using the client::send() and client::read() methods.
 * 
 * A client calls the connect() method if a server's hostname and a port
 * are passed in the constructor. But though this class is named as a
 * "client" class, this does not in any way mean that the server cannot
 * make use of its functions. In a server-client model, a server can accept
 * multiple clients through the net::server::accept() method, which can
 * be wrapped into the net::client object.
 * 
 * Unlike net::isocketstream and net::osocketstream which stream data as
 * characters, net::client sends and reads data in raw bytes, which is 
 * buffer-free and more efficient. It does so through implicit conversion
 * using C++ templates, given that the data sent or received are either
 * strings, integral data types, or structs without pointers.
 * 
 * When net::client sends strings, it also sends a terminating '\0' char
 * to the receiver, such that strings sent or received can be identified
 * implicitly without knowing their lengths.
 * 
 * @namespace  	net
 * @author 		Rico Tiongson
 * @package  	SocketNetworking
 */


#ifndef __INCLUDE_NET_CLIENT__
#define __INCLUDE_NET_CLIENT__

#include <cstring>			// std::strlen()
#include <string>			// std::string
#include <sys/types.h>		// sockaddr, sockaddr_in
#include <sys/socket.h>		// connect(), send(), recv()
#include <netdb.h>			// gethostbyname()
#include <arpa/inet.h>		// htons()
#include "net_socket.hpp"	// net::socket, net::socket_exception


namespace net {

	using namespace std;

	/**
	 * @brief      a lightweight wrapper class for a client socket
	 */

	class client : public socket {
	public:

		/**
		 * @brief      constructs and wraps a file descriptor as a client socket
		 * @details    does not call connect()
		 * @param[in]  sockfd	a socket file descriptor
		 */

		client(int sockfd = -1): socket(sockfd) {}

		/**
		 * @brief      constructs and copies another socket as a client socket
		 * @details    does not call connect()
		 * @param[in]  sock		another socket
		 */

		client(const socket& sock): socket(sock) {}

		/**
		 * @brief      constructs and connects a client socket to a server with a specific host and port
		 * @param[in]  host  the host server to connect to
		 * @param[in]  port  the host port to connect to
		 * @throw      a socket_exception if client cannot connect to host
		 */

		client(const string& host, unsigned short port): socket() {
			// get host by name
			struct hostent *server = gethostbyname(host.c_str());
			if (!server)
				throw socket_exception("client::gethostbyname()");
			// create socket address
			struct sockaddr_in sad;
			memset(&sad, 0, sizeof sad);
			sad.sin_family = AF_INET;
			sad.sin_port = htons(port);
			// connect
			if (connect(sockfd, (sockaddr*) &sad, sizeof sad) < 0)
				throw socket_exception("client::connect()");
		}

		/**
		 * @brief      sends data with specified number of bytes to the connected socket
		 * @details    closes this client socket if the connection was lost
		 * @param      data   pointer to the address of data to send
		 * @param[in]  bytes  number of bytes to send
		 * @tparam     T      the type of data to send
		 * @throw      a socket_exception if there was an error in sending
		 * @return     a reference to this client object, which can be used to detect if the connection was unexpectedly closed or not
		 */

		template <typename T>
		client& send(T* data, size_t bytes) throw(socket_exception) {
			for (char* buffer = (char*) data; bytes;) {
				ssize_t sent = ::send(sockfd, buffer, bytes, 0);
				if (sent < 0)
					throw socket_exception("client::send()");
				else if (!sent) {
					close();
					break;
				}
				buffer += sent;
				bytes -= sent;
			}
			return *this;
		}

		/**
		 * @brief      implicitly sends data as bytes to the connected socket
		 * @details    uses the sizeof() function to determine how many bytes of data to send; closes this client socket if the connection was lost
		 * @param[in]  data  the data to send; must not have any pointers
		 * @tparam     T     the non-pointer data type to send
		 * @throw      a socket_exception if there was an error in sending
		 * @return     a reference to this client object, which can be used to detect if the connection was unexpectedly closed or not
		 */

		template <typename T>
		inline client& send(T data) throw(socket_exception) {
			return send(&data, sizeof(data));
		}

		/**
		 * @brief      sends a char array to the connected socket
		 * @details    closes this client socket if the connection was lost; sends until the terminating character '\0', inclusive
		 * @param      data  the char array to send; should be a valid string with a terminating '\0' character
		 * @throw      a socket_exception if there was an error in sending
		 * @return     a reference to this client object, which can be used to detect if the connection was unexpectedly closed or not
		 */

		inline client& send(char data[]) throw(socket_exception) {
			return send(data, strlen(data) + 1);
		}
		
		/**
		 * @brief      sends a const char array to the connected socket
		 * @details    closes this client socket if the connection was lost; sends until the terminating character '\0', inclusive
		 * @param      data  the const char array to send; should be a valid string with a terminating '\0' character
		 * @throw      a socket_exception if there was an error in sending
		 * @return     a reference to this client object, which can be used to detect if the connection was unexpectedly closed or not
		 */
		
		inline client& send(const char data[]) throw(socket_exception) {
			return send(data, strlen(data) + 1);
		}

		/**
		 * @brief      sends a string to the connected socket
		 * @details    closes this client socket if the connection was lost; sends until the terminating character '\0', inclusive
		 * @param      data  the string to send; should be a valid string with a terminating '\0' character
		 * @throw      a socket_exception if there was an error in sending
		 * @return     a reference to this client object, which can be used to detect if the connection was unexpectedly closed or not
		 */
		
		inline client& send(const string& data) throw(socket_exception) {
			return send(data.c_str(), data.length() + 1);
		}

		/**
		 * @brief      receives data with a certain number of bytes from the connected socket
		 * @details    closes this client socket if the connection was lost
		 * @param      data   pointer to the address of the receiving data buffer
		 * @param[in]  bytes  number of bytes to receive
		 * @tparam     T      the type of data to receive
		 * @throw      a socket_exception if there was an error in receiving
		 * @return     a reference to this client object, which can be used to detect if the connection was unexpectedly closed or not
		 */

		template <typename T>
		client& read(T* data, size_t bytes) throw(socket_exception) {
			for (char* buffer = (char*) data; bytes;) {
				ssize_t received = ::recv(sockfd, buffer, bytes, 0);
				if (received < 0)
					throw socket_exception("client::read()");
				else if (!received) {
					close();
					break;
				}
				buffer += received;
				bytes -= received;
			}
			return *this;
		}

		/**
		 * @brief      implicitly receives data of a certain data type from the connected socket
		 * @details    uses the sizeof() function to determine how many bytes of data to receive; closes this client socket if the connection was lost
		 * @param      data   a reference to where to put the received data
		 * @tparam     T      the type of data to receive
		 * @throw      a socket_exception if there was an error in receiving
		 * @return     a reference to this client object, which can be used to detect if the connection was unexpectedly closed or not
		 */
		
		template <typename T>
		inline client& read(T& data) throw(socket_exception) {
			return read(&data, sizeof(data));
		}
		
		/**
		 * @brief      implicitly receives an anonymous data type from the connected socket
		 * @details    uses the sizeof() function to determine how many bytes of data to receive; closes this client socket if the connection was lost
		 * @tparam     T      the type of data to receive
		 * @throw      a socket_exception if there was an error in receiving, or if the socket was unexpectedly closed while reading
		 * @return     the received object of data type T
		 */

		template <typename T>
		inline T read() throw(socket_exception) {
			char buffer[sizeof(T)];
			if (!read(buffer, sizeof(T)))
				throw socket_exception("client::read()");
			return *(T*) buffer; // return in this manner in case T has no default constructor
		}

		/**
		 * @brief      receives a string as a char array from the connected socket
		 * @details    closes this client socket if the connection was lost; receives all characters until a terminating character '\0' was found, exclusive
		 * @param      data  a pointer to the char array that describes where to put the received string
		 * @throw      a socket_exception if there was an error in receiving
		 * @return     a reference to this client object, which can be used to detect if the connection was unexpectedly closed or not
		 */

		inline client& read(char data[]) throw(socket_exception) {
			while (read(*data) && *data != '\0')
				++data;
			return *this;
		}
		
		/**
		 * @brief      receives a string from the connected socket
		 * @details    closes this client socket if the connection was lost; receives all characters until a terminating character '\0' was found, exclusive
		 * @param      data  a reference to where to put the received string
		 * @throw      a socket_exception if there was an error in receiving
		 * @return     a reference to this client object, which can be used to detect if the connection was unexpectedly closed or not
		 */

		inline client& read(string& data) throw(socket_exception) {
			data.clear();
			char buffer;
			while (read(buffer) && buffer != '\0')
				data.push_back(buffer);
			return *this;
		}

	};

	/**
	 * @brief      a template specialization for implicitly receiving an anonymous string from the connected socket
	 * @throw      a socket_exception if there was an error in receiving, or if the socket was unexpectedly closed while reading
	 * @return     the received string
	 */

	template<> inline string client::read<string>() throw(socket_exception) {
		string data;
		if (!read(data))
			throw socket_exception("client::read()");
	}

	/**
	 * @brief      a template specialization for implicitly receiving an anonymous c-string from the connected socket
	 * @return     the received c-string
	 */

	template<> inline char* client::read<char*>() throw(socket_exception) {
	 	string data;
	 	if (!read(data))
	 		throw socket_exception("client::read()");
		char *buffer = new char[data.length() + 1];
		strcpy(buffer, data.c_str());
		return buffer;
	}

}

#endif /* __INCLUDE_NET_CLIENT__ */