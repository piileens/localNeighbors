#include <string>
#include <unordered_map>
#include <netinet/in.h>

#define REQUEST "REQUEST"
#define RESPONSE_PREFIX "RESPONSE="
#define UDP_PORT 5005

struct DeviceInfo
{
	std::string ipAddress;
	long long discoveryTime; // epoch time
};

class LocalNeighbors
{
public:
	LocalNeighbors();
	bool broadcast(std::string message);
	void listen();
	std::unordered_map<std::string,DeviceInfo> getConnectionInfo();
	std::string getMac();
	void updateLog();

	
private:
	std::unordered_map<std::string,DeviceInfo> connections;
	std::string macAddress;
};