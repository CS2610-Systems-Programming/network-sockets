//Create sockets for the server.
#include <sys/socket.h>

int main()
{
	//socket() function creates the socket by taking in 3 arguments
	//and returns an integer for the file descriptor. 
	//File Descriptor: an integer that the OS uses to track open files/ sockets. 
	//AF_INET: Address Family - Internet. 
	//Used for network communication over IPv4.
	//Example: Use in communication between two machines over the Internet or LAN. 
	//Different than AF_LOCAL (Local IPC). AF_LOCAL is used for Inter-Process Communication (IPC)
	//on the same machine. Communications use file paths, not IP Address. This creates a socket that communicate
	//via a file. 
	
	int fileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
	if(fileDescriptor < 0)
	{
		report("socket", 1); //need to check if we have function named "report" or not. 
	}

	//bind(): tells the O.S. to listen for incoming connections on THIS machine's IP and port.
	//Binding in memory = The server program is assigning itself a local network address (IP) and prot to listen 
	//Set the local address and port in memory by filling a struct "sockaddr_in"
	//struct sockaddr_in is a structure that represents an IPv4 internet address in socket programming. 
	//It holds info in regards to the address family, port, and IP address that a socket will use
	//when binding to a network address or when connecting to a remote server. 
	
	struct sockaddr_in socketAddress;
	//Initialize (clear) the socketAddress structure by setting all its bytes to zero.
	//memset() is a standard C library function that fills a block of memory with specific value.
	memset(&socketAddress, 0, sizeof(socketAddress));
	
	
}
