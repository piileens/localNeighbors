#include <iostream>
#include "service.hpp"
#include <unistd.h>


int main()
{
	LocalNeighbors connection;
	while ( true )
	{
		connection.listen();
		connection.broadcast(REQUEST);
	}
	return 0;
}
