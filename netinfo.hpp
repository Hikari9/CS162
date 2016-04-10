/**
 * Contains namespace methods for net-cpp to
 * acquire network information.
 *
 * @author Rico Tiongson
 */


#ifndef __INCLUDE_NETINFO__
#define __INCLUDE_NETINFO__ 1

#include <cstring>
#include <string>

#include <stropts.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/netdevice.h>
#include <stropts.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <unistd.h>
#include "net.hpp"

namespace net {
	using namespace std;

	/**
	 * Ask for a map of all available IP Addresses.
	 * @param  ipver 		[default: AF_INET] 	the version of the IP Address to return; can be AF_INET or AF_INET6
	 * @param  use_cache	[default: true]		a boolean describing whether cache should be used or not
	 * @return 				a constant reference to the map of ip addresses (const std::map<std::string, std::string>&)
	 */
	const map<string, string>& ip_all(int ipver = AF_INET, bool use_cache = true) {
		static map<int, map<string, string> > ip_cache;
		if (use_cache && ip_cache.count(ipver))
			return ip_cache[ipver];
		map<string, string>& cache = ip_cache[ipver];
		net::socket socket;
		struct ifconf conf;
		struct ifreq ifr[50];
		conf.ifc_buf = (char*) ifr;
		conf.ifc_len = sizeof ifr;
		if (ioctl(socket.sock, SIOCGIFCONF, &conf) < 0) {
			perror("net::ip_address::ioctl()");
			return cache;
		}
		// acquire entries
		size_t entries = conf.ifc_len / sizeof(ifreq);
		size_t iplen = ipver == AF_INET6 ? INET6_ADDRSTRLEN : INET_ADDRSTRLEN;
		char *ip = new char[iplen];
		for (int i = 0; i < entries; ++i) {
			struct sockaddr_in *sad = (sockaddr_in*) &ifr[i].ifr_addr;
			if (!inet_ntop(ipver, &sad->sin_addr, ip, iplen)) {
				perror("net::ip_address::inet_ntop()");
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
	 * Ask for the current network card's IP Address.
	 * @param  ipver 		[default: AF_INET] 	the version of the IP Address to return; can be AF_INET or AF_INET6
	 * @param  key   		[default: "eth0"] 	the key of the network card used to get the IP Address; common ones are "lo" or "eth0"
	 * @param  use_cache	[default: true]		a boolean describing whether cache should be used or not
	 * @return       		a string referring to the IP Address of the host computer
	 */
	string ip_address(int ipver = AF_INET, const char* key = "eth0", bool use_cache = true) {
		const map<string, string>& driver = ip_all(ipver, use_cache);
		return driver.count(key) ? driver.find(key)->second : "";
	}

}

#endif /* __INCLUDE_NETINFO__ */
