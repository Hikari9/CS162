/**
 * This header introduces a lightweight, copy-safe socket class
 * that abstracts the low-end methods of POSIX web sockets. Also
 * includes a socket exception class for error handling.
 * 
 * This socket wrapper performs automatic garbage collection for
 * multiple copies of the same socket using a map of file descriptors.
 * Only the last instance of the socket will close the socket file
 * descriptor. Forking is safe as long as socket::close() was not run
 * prematurely.
 *
 * @namespace  	net
 * @author 		Rico Tiongson
 * @package  	SocketNetworking
 */

#ifndef __INCLUDE_NET_SOCKET__
#define __INCLUDE_NET_SOCKET__

#include <string.h>		// strerror()
#include <errno.h>		// std::errno
#include <string>		// std::string
#include <map>			// std::map
#include <exception>	// std::exception
#include <unistd.h>		// close()
#include <sys/types.h>	// sockaddr, sockaddr_in, socklen_t
#include <sys/socket.h>	// socket()
#include <arpa/inet.h> 	// inet_ntoa(), ntohs() 

namespace net {

	/**
	 * @brief      an exception handler class for socket errors.
	 */

	class socket_exception : public std::exception {
	public:
		const char* function;
		socket_exception(const char* function): function(function) {}
		virtual const char* what() const throw() {return ((std::string(function) + ": ") + strerror(errno)).c_str();}
	};

	/**
	 * @brief       a lightweight wrapper class for TCP IPv4 sockets
	 */
	
	class socket {

	private:

		/**
		 * a map of the number of socket instances for each socket file descriptor
		 */

		static std::map<int, int> instances;

	protected:

		/**
		 * the socket file descriptor
		 */
		
		int sockfd;

	public:

		/**
		 * @brief      constructs a new TCP/IPv4 socket as a stream
		 */

		socket(): sockfd(::socket(AF_INET, SOCK_STREAM, 0)) {
			if (sockfd >= 0) ++instances[sockfd];
		}

		/**
		 * @brief      constructs a socket wrapper for a specific file descriptor
		 * @param[in]  sockfd  the socket file descriptor to wrap
		 */

		socket(int sockfd): sockfd(sockfd) {
			if (sockfd >= 0) ++instances[sockfd];
		}

		/**
		 * @brief      constructs a copy of another socket object
		 * @param[in]  sock  the socket to copy
		 */

		socket(const socket& sock): sockfd(sock.sockfd) {
			if (sockfd >= 0) ++instances[sockfd];
		}

		/**
		 * @brief      copies another socket object
		 * @param[in]  sock  the socket to copy
		 * @return     a reference to the current socket object
		 */

		socket& operator = (const socket& sock) {
			sockfd = sock.sockfd;
			if (sockfd >= 0) ++instances[sockfd];
			return *this;
		}

		/**
		 * @brief      destroys this socket if it is the last instance of its file descriptor
		 */

		~socket() {
			if (good() && --instances[sockfd] == 0)
				close();
		}

		/**
		 * @brief      checks if this socket is open and available
		 * @return     a bool describing if the current socket is open
		 */

		virtual bool good() const {
			return sockfd >= 0 && instances.count(sockfd);
		}

		/**
		 * @brief      checks if this socket is open and available
		 */

		inline operator bool() const {
			return good();
		}

		/**
		 * @brief      gets this socket object's file descriptor
		 */

		inline operator int() const {
			return sockfd;
		}

		/**
		 * @brief      forcefully closes this socket
		 */

		virtual void close() {
			if (good()) {
				instances.erase(sockfd);
				::close(sockfd);
			}
			sockfd = -1;
		}

		/**
		 * @brief      checks if this socket's file descriptor is less than another's file descriptor
		 * @param[in]  sock  the socket to copare with
		 * @return     true if this socket is less than the other
		 */
	
		bool operator < (const socket& sock) const {
			return sockfd < sock.sockfd;
		}

		/**
		 * @brief      gets the local IP address of the host socket
		 * @return     a const char pointer to the local IPv4 address of the host socket
		 */

		const char* local_ip_address() const throw(socket_exception)  {
			struct sockaddr_in sad;
			socklen_t len = sizeof(sad);
			if (getsockname(sockfd, (sockaddr*) &sad, &len) < 0)
				throw socket_exception("socket::local_ip_address()");
			return inet_ntoa(sad.sin_addr);
		}

		/**
		 * @brief      gets the local port of the host socket
		 * @return     an unsigned short determining the local IPv4 port used by the host socket
		 */

		unsigned short local_port() const throw(socket_exception) {
			struct sockaddr_in sad;
			socklen_t len = sizeof(sad);
			if (getsockname(sockfd, (sockaddr*) &sad, &len) < 0)
				throw socket_exception("socket::local_ip_address()");
			return ntohs(sad.sin_port);
		}

		/**
		 * @brief      gets the foreign IP address of a communicating socket
		 * @details    typically used to determine the IP address of a socket returned by server::accept()
		 * @return     a const char pointer to the local IPv4 address of the communicating socket
		 */

		const char* foreign_ip_address() const throw(socket_exception) {
			struct sockaddr_in sad;
			socklen_t len = sizeof(sad);
			if (getpeername(sockfd, (sockaddr*) &sad, &len) < 0)
				throw socket_exception("socket::foreign::ip_address()");
			return inet_ntoa(sad.sin_addr);
		}

		/**
		 * @brief      gets the foreign port of a communicating socket
		 * @details    typically used to determine the port used by a socket returned by server::accept()
		 * @return     an unsigned short determining the local IPv4 port used by a communicating socket
		 */

		unsigned short foreign_port() const throw(socket_exception) {
			struct sockaddr_in sad;
			socklen_t len = sizeof(sad);
			if (getpeername(sockfd, (sockaddr*) &sad, &len) < 0)
				throw socket_exception("socket::foreign::ip_address()");
			return ntohs(sad.sin_port);
		}

	};

	std::map<int, int> socket::instances;

};

#endif /* __INCLUDE_NET_SOCKET__ */