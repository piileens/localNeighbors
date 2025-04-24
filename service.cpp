#include <unordered_map>
#include "service.hpp"
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>

const std::string logFile = "/tmp/neighbors.log";

LocalNeighbors::LocalNeighbors()
{
	struct ifreq ifr;
	struct ifconf ifc;
	char buf[1024];
	macAddress = "";

	int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (sockfd == -1) 
	{ 
		std::cerr << "Socket creation failed." << std::endl;
		return;
	}

	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = buf;
	if (ioctl(sockfd, SIOCGIFCONF, &ifc) == -1) 
	{
		std::cerr << "Failed to get interface list" << std::endl;
		close(sockfd);
		return;
	}

	struct ifreq* it = ifc.ifc_req;
	const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));

	for (; it != end; ++it) 
	{
		strcpy(ifr.ifr_name, it->ifr_name);
		if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) == 0 )
		{
			if (! (ifr.ifr_flags & IFF_LOOPBACK))
			{
				if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) == 0)
				{
					break;
				}
			}
		}
	}
	unsigned char* mac = (unsigned char*)ifr.ifr_hwaddr.sa_data;
	std::stringstream ss;
	for (int i = 0; i < 6; ++i) {
		ss << std::hex << std::setw(2) << std::setfill('0') << (int)mac[i];
		if (i == 5) break;
		ss << ":";
	}
	macAddress = ss.str();
	close(sockfd);
}
bool LocalNeighbors::broadcast(std::string message)
{
	int sockfd;
	struct sockaddr_in broadcastAddr;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		std::cerr << "Socket creation failed." << std::endl;
		return false;
	}

	int broadcastEnable = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) == -1) 
	{
		std::cerr << "Setting broadcast option failed." << std::endl;
		close(sockfd);
		return false;
	}
	memset(&broadcastAddr, 0, sizeof(broadcastAddr));
	broadcastAddr.sin_family = AF_INET;
	broadcastAddr.sin_port = htons(UDP_PORT);
	broadcastAddr.sin_addr.s_addr = INADDR_BROADCAST;

	if (sendto(sockfd, message.c_str(), message.size(), 0, (struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr)) == -1)
	{
		std::cerr << "Sending broadcast message failed." << std::endl;
		close(sockfd);
		return false;
	} else
	{
		std::cout << "Broadcast message sent." << std::endl;
		
	}

	close(sockfd);

	return true;
}

void LocalNeighbors::listen()
{
	int sockfd;
	struct sockaddr_in serverAddr, clientAddr;
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		std::cerr << "Socket creation failed." << std::endl;
		return;
	}
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(UDP_PORT);
	serverAddr.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0

	if (bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1)
	{
		std::cerr << "Binding socket failed." << std::endl;
		close(sockfd);
		return;
	}
	struct timeval t;
	t.tv_sec = 5;
	t.tv_usec = 0;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t)) == -1) 
	{
		std::cerr << "Setting receive timeout option failed." << std::endl;
		close(sockfd);
		return;
	}
	socklen_t clientLen = sizeof(clientAddr);
	char buffer[1024];
	ssize_t recvLen = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&clientAddr, &clientLen);
	if (recvLen > 0) 
	{
		buffer[recvLen] = '\0';
		std::cout << buffer << std::endl;
		std::cout << inet_ntoa(clientAddr.sin_addr) << std::endl;
		std::string message = buffer;
		if (message.rfind(RESPONSE_PREFIX, 0) == 0) 
		{
			std::string macAddress = message.substr(strlen(RESPONSE_PREFIX));
			macAddress.erase(std::remove(macAddress.begin(), macAddress.end(), '\n'), macAddress.end());
			DeviceInfo devInfo;
			devInfo.ipAddress = inet_ntoa(clientAddr.sin_addr);
			const auto now = std::chrono::system_clock::now();
			devInfo.discoveryTime = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
			connections[macAddress] = devInfo;
			updateLog();
		}
		else if (message.rfind(REQUEST, 0) == 0)
		{
			usleep(10000);
			std::cout << "REQUEST RECEIVED, sending MAC " << std::endl;
			broadcast (RESPONSE_PREFIX + macAddress);
		}
	}

	close(sockfd);
}

std::string LocalNeighbors::getMac()
{
	return macAddress;
}

void LocalNeighbors::updateLog()
{
	std::ofstream neighborFile(logFile);

	for ( auto& conn : connections )
	{
		neighborFile << conn.first << ";" << conn.second.ipAddress << ";" << conn.second.discoveryTime << std::endl;
	}
}

std::unordered_map<std::string,DeviceInfo> LocalNeighbors::getConnectionInfo()
{
	return connections;
}

