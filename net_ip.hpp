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

namespace net {

	using namespace std;

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
			// perror("net::ip_address::ioctl()");
			return cache;
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
	 * @param[in]  key        [default: NULL] the key of the network card used to get the ip address; if NULL is passed, it will return the first match in the list ["eth0", "wlan", "lo"] in that order
	 * @param[in]  use_cache  [default: true] a boolean describing whether cache should be used or not; updates current cache if set to false
	 * @return     a c-string referring to the IP address of the host computer
	 */
	
	const char* ip_address(int ipver = AF_INET, const char* key = NULL, bool use_cache = true) {
		const map<string, string>& driver = ip_all(ipver, use_cache);
		if (key == NULL) {
			if (driver.count("eth0"))
				return driver.find("eth0")->second.c_str();
			if (driver.count("wlan0"))
				return driver.find("wlan0")->second.c_str();
			if (driver.count("lo"))
				return driver.find("lo")->second.c_str();
			return NULL;
		}
		return driver.count(key) ? driver.find(key)->second : "";
	}

}

#endif /* __INCLUDE_NETINFO__ */
