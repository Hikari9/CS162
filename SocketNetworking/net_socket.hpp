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
 * Also in this header are two namespace function that can get the
 * ip address of your network card. To get the default ip address
 * of your localhost, (eth0, wlan0, or lo), call net::ip_address().
 * You can list ip addresses by calling net::ip_all(), which returns
 * a std::map of ip addresses on your network card.
 *
 * @namespace  	net
 * @author 		Rico Tiongson
 * @package  	SocketNetworking
 */

#ifndef __INCLUDE_NET_SOCKET__
#define __INCLUDE_NET_SOCKET__

#include <cstring>		// strerror()
#include <cstdio>		// std::sprintf()
#include <cerrno>		// std::errno
#include <string>		// std::string
#include <map>			// std::map
#include <exception>	// std::exception
#include <unistd.h>		// close()
#include <sys/types.h>	// sockaddr, sockaddr_in, socklen_t
#include <sys/socket.h>	// socket()
#include <sys/ioctl.h>	// ioctl()
#include <arpa/inet.h> 	// inet_ntoa(), ntohs(), inet_ntop()
#include <linux/netdevice.h> // ifconf, ifreq

namespace net {

	using namespace std;

	/**
	 * @brief      an exception handler class for socket errors.
	 */

	class socket_exception : public exception {
	public:
		const char* linker;
		socket_exception(const char* linker): linker(linker) {}
		virtual const char* what() const throw() {return (string(linker) + ": " + strerror(errno)).c_str();}
	};

	/**
	 * @brief       a lightweight wrapper class for TCP IPv4 sockets
	 */
	
	class socket {

	private:

		/**
		 * @brief      a map of the number of socket instances for each socket file descriptor
		 */

		static map<int, int> instances;

	protected:

		/**
		 * @brief      the socket file descriptor
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
		 * @details    originally threw a socket_exception if the current sockfd was originally opened
		 *             removed the error check because socket::good() can address this problem
		 */

		virtual void close() {
			if (good()) {
				instances.erase(sockfd);
				::close(sockfd);
				// if (::close(sockfd) < 0)
					// throw socket_exception("socket::close()");
			}
			sockfd = -1;
		}

		/**
		 * @brief      checks if this socket's file descriptor is less than another's file descriptor
		 * @param[in]  sock  the socket to compare with
		 * @return     true if this socket is less than the other
		 */
	
		bool operator < (const socket& sock) const {
			return sockfd < sock.sockfd;
		}

		/**
		 * @brief      gets the local IP address of the host socket
		 * @throw      a socket_exception if the socket cannot get the host's name
		 * @return     a const char pointer to the local IPv4 address of the host socket
		 */

		virtual const char* ip() const throw(socket_exception)  {
			struct sockaddr_in sad;
			socklen_t len = sizeof(sad);
			if (getsockname(sockfd, (sockaddr*) &sad, &len) < 0)
				throw socket_exception("socket::ip()");
			return inet_ntoa(sad.sin_addr);
		}

		/**
		 * @brief      gets the local port of the socket
		 * @throw      a socket_exception if the socket cannot get the port
		 * @return     an unsigned short determining the local IPv4 port used by the socket
		 */

		virtual unsigned short port() const throw(socket_exception) {
			struct sockaddr_in sad;
			socklen_t len = sizeof(sad);
			if (getsockname(sockfd, (sockaddr*) &sad, &len) < 0)
				throw socket_exception("socket::port()");
			return ntohs(sad.sin_port);
		}

	};

	map<int, int> socket::instances;

	/**
	 * @brief      ask for a map of all available ip addresses
	 * @param[in]  ipver      [default: AF_INET] the version of the ip address to return; can be AF_INET or AF_INET6
	 * @param[in]  use_cache  [default: true] a boolean describing whether cache should be used or not; updates current cache if set to false
	 * @return     a constant reference to the map of ip addresses (const std::map<std::string, std::string>&)
	 */

	const map<string, string>& ip_all(int ipver = AF_INET, bool use_cache = true) {
		static map<int, map<string, string> > ip_cache;
		if (use_cache && ip_cache.count(ipver))
			return ip_cache[ipver];
		map<string, string>& cache = ip_cache[ipver];
		int sockfd = ::socket(ipver, SOCK_STREAM, 0);
		struct ifconf conf;
		struct ifreq ifr[50];
		conf.ifc_buf = (char*) ifr;
		conf.ifc_len = sizeof ifr;
		if (ioctl(sockfd, SIOCGIFCONF, &conf) < 0) {
			::close(sockfd);
			throw socket_exception("net::ip_address()");
		}
		::close(sockfd);
		// acquire entries
		size_t entries = conf.ifc_len / sizeof(ifreq);
		size_t iplen = ipver == AF_INET6 ? INET6_ADDRSTRLEN : INET_ADDRSTRLEN;
		char *ip = new char[iplen];
		for (int i = 0; i < entries; ++i) {
			struct sockaddr_in *sad = (sockaddr_in*) &ifr[i].ifr_addr;
			if (!inet_ntop(ipver, &sad->sin_addr, ip, iplen)) {
				// perror("net::ip_address::inet_ntop()");
				continue;
			}
			// ip address to cache
			const char* driver = ifr[i].ifr_name;
			cache[driver] = ip;
		}
		delete[] ip;
		return cache;
	}

	/**
	 * @brief      ask for the current network card's ip address
	 * @param[in]  ipver      [default: AF_INET] the version of the ip address to return; can be AF_INET or AF_INET6
	 * @param[in]  key        [default: NULL] the key of the network card used to get the ip address; if NULL is passed, it will return the first match in the list ["eth#", "wlan", "lo"] in that order
	 * @param[in]  use_cache  [default: true] a boolean describing whether cache should be used or not; updates current cache if set to false
	 * @return     a c-string referring to the IP address of the host computer
	 */
	
	const char* ip_address(int ipver = AF_INET, const char* key = NULL, bool use_cache = true) {
		const map<string, string>& driver = ip_all(ipver, use_cache);
		if (key == NULL) {
			// try each combination of eth# and wlan# up to digit 9
			char key_buf[6];
			for (char digit = '0'; digit <= '9'; ++digit) {
				sprintf(key_buf, "eth%c", digit);
				if (driver.count(key_buf))
					return driver.find(key_buf)->second.c_str();
				sprintf(key_buf, "wlan%c", digit);
				if (driver.count(key_buf))
					return driver.find(key_buf)->second.c_str();
			}
			// lastly, try loopback
			if (driver.count("lo"))
				return driver.find("lo")->second.c_str();
			// no network card
			return NULL;
		}
		return driver.count(key) ? driver.find(key)->second.c_str() : "";
	}

};

#endif /* __INCLUDE_NET_SOCKET__ */