//Create sockets for the server.
#include <sys/socket.h>
#define PortNumber	1234
#define MaxConnects	8
#define BuffSize	256
#define ConversationLen	3
#define Host		"localhost"

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
		report("Socket Issue", 1); //need to check if we have function named "report" or not. 
	}	
		
	//bind(): tells the O.S. to listen for incoming connections on THIS machine's IP and port.
	//Binding in memory = The server program is assigning itself a local network address (IP) and prot to listen 
	//Set the local address and port in memory by filling a struct "sockaddr_in"
	//struct sockaddr_in is a structure that represents an IPv4 internet address in socket programming. 
	//It holds info in regards to the address family, port, and IP address that a socket will use
	//when binding to a network address or when connecting to a remote server. 
	
	//This structure holds IP address and port information for IPv4 sockets.
	struct sockaddr_in socketAddress;

	//Initialize (clear) the socketAddress structure by setting all its bytes to zero.
	//memset() is a standard C library function that fills a block of memory with specific value.
	memset(&socketAddress, 0, sizeof(socketAddress));

	//Tell the socket API what kind of addresses I am using
	socketAddress.sin_family = AF_INET;

	//host-to-network-LONG: Converts the IP address to network byte order
	//I am trying to binding the socket to all available network interfaces on my machine.
	//(including localhost, Wifi, Ethernet, etc.)
	
	socketAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	
	//
	socketAddress.sin_port = htons(PortNumber);
			
	//BINDING
	//fileDescriptor: the integer representing my socket
	//(struct sockaddr *) &socketAddress: I created a pointer to my socketAddress structure, and 
	//cast that into sockaddr* type
	//Tell the O.S. to attach the socket (identified by fileDescriptor) to the address and port specified in
	//socketAddress structure. 
	if(bind(fileDescriptor, (struct sockaddr *) &socketAddress, sizeof(socketAddress)) < 0)
	{
		report("Binding Issue", 1);
	}
	
	//LISTENING
	//Listen for socket connections and limit the queue of incoming connections
}	

