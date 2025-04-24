#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <chrono>

const std::string logFile = "/tmp/neighbors.log";
const long long RECENT_THRESHOLD_SECONDS = 30;

int main ()
{
	std::ifstream neighborsFile(logFile);
	if (!neighborsFile.is_open()) {
		std::cerr << "Error: Could not open file " << logFile << std::endl;
		return -1;
	}

	const auto now = std::chrono::system_clock::now();
	long long epochNow = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
	std::string line;
	std::cout << "Neighbor devices:" << std::endl;
	while (std::getline(neighborsFile, line))
	{
		std::string mac, ip, epochStr;
		long long epoch = 0;
		std::stringstream ss(line);

		if (std::getline(ss, mac, ';') && std::getline(ss, ip, ';') && std::getline(ss, epochStr))
		{
			std::getline(ss, mac, ';');
			std::getline(ss,ip,';');
			std::getline(ss,epochStr);
			if (sscanf(epochStr.c_str(), "%lld", &epoch) == 1) 
			{
				if ( epochNow - epoch <= RECENT_THRESHOLD_SECONDS )
				{
					std::cout << "MAC=" << mac << " IP=" << ip << std::endl;
				}
			}
		}
		else
		{
			std::cerr << "Mallformed line: " << line << std::endl;
		}
	}
	
	return 0;
}