
//Xiaohe YANG
//504640747
//CS118
#include <string>
#include <thread>
#include <iostream>
#include <fstream>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <vector>
// needed to enable timeout
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#define BUFFER_SIZE 4096
#define BACKLOG 10
#define MAX_THREAD_NUM 10
#define TIMEOUT 10

int sock_fd;
bool client_connected = true;
std::vector<std::thread> connections;

static void terminate_func(int signum){
	close(sock_fd);
	client_connected = false;
	for (unsigned int i = 0; i < connections.size(); i++){
		connections[i].join();
	}
	exit(0);
}

void connect_func(int client_socketFd, int connection_count, char *dir){
    int rv;
	fd_set read_fds;
	FD_ZERO(&read_fds);
	FD_SET(client_socketFd, &read_fds);
	char buf[BUFFER_SIZE] = {0};
	std::string filename(dir);
	struct timeval timeout, *mtv;
	timeout.tv_sec = TIMEOUT;
	timeout.tv_usec = 0;
    mtv = &timeout;

	// Prepare filename to be the number of connections
	if(filename[filename.length() - 1] == '/')
		filename += (std::to_string(connection_count) + ".file");
	else
		filename += ('/' + std::to_string(connection_count) + ".file");

	// Open a file that the client sent
	std::ofstream ofs(filename, std::fstream::binary);
	if(ofs.fail()){
		std::cerr << "ERROR: " << filename << " cannot be opened." << std::endl;
		close(client_socketFd);
		return;
	}
	
	// Write to file named by the number of connections
	int total_bytes_received = 0;
	while(client_connected) {
		memset(buf, '\0', sizeof(buf));
        
		int bytes_received = recv(client_socketFd, buf, sizeof(buf), 0);
		// Error occured in recv
		if(bytes_received == -1 && errno != EAGAIN && errno != EWOULDBLOCK){
            std::cerr << "ERROR: failed to receive message from client "  << std::endl;
			ofs.close();
			close(client_socketFd);
			return;
		}
        //Successfully written to file
        else if(bytes_received == 0){
			break;
        }

        // non-blocking pooling, with timeout = 15s
		rv = select(client_socketFd + 1, &read_fds, NULL, NULL, mtv);
        
        if(rv < 0){
            std::cerr << "ERROR: select error " << gai_strerror(rv) << std::endl;
        	ofs.close();
       		close(client_socketFd);
       		return;
        }else if (rv > 0){
            std::cout<<"Connection succeeded!" << std::endl;
        }
       	else {
       		std::cerr << "ERROR: receive timeout" << std::endl;
       		char error_msg[23] = "ERROR: receive timeout";
       		ofs.write(error_msg, 23);
       		ofs.close();
       		close(client_socketFd);
       		return;
       	}
		total_bytes_received += bytes_received;
		//Wirte received bytes to file
		ofs.write(buf, bytes_received);
	}
    std::cout << total_bytes_received << " bytes received." <<std::endl;
	std::cout << "Connection "<< connection_count<< " finished writing to " << filename << std::endl;
	
	ofs.close();
	close(client_socketFd);
	return;
}

int main(int argc, char *argv[])
{
	struct sigaction act;
	int rv;
	
	char *port, *dir;
	struct addrinfo hints, *servinfo, *p;
	
	int yes = 1;
	struct sockaddr_in clientAddr;
	socklen_t clientAddrSize = sizeof(clientAddr);
	char ipstr[INET_ADDRSTRLEN] = {'\0'};
	int client_socketFd;
	int connection_count = 0;

	// Register signals. Exit with code 0.
	act.sa_handler = terminate_func;
	rv = sigaction(SIGQUIT, &act, NULL);
	if(rv == -1){
		exit(0);
	}
	rv = sigaction(SIGTERM, &act, NULL);
	if(rv == -1){
		exit(0);
	}

	// Usage check
	if(argc != 3){
		std::cerr << "ERROR: USAGE: ./server <PORT> <FILE-DIR>\n";
		exit(EXIT_FAILURE);
	}

	port = argv[1];
	dir = argv[2];
	
	// Port number sanity check.
	if(strtol(port, NULL, 0) < 1024 ){
		std::cerr << "ERROR: port number: Out of range.";
		exit(EXIT_FAILURE);
	}
    
	memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;
    
    if ((rv = getaddrinfo("0.0.0.0", port, &hints, &servinfo)) != 0) {
    	std::cerr << "ERROR: getaddrinfo error: " << gai_strerror(rv) << std::endl;
    	exit(EXIT_FAILURE);
	}

	// loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
    
        if ((sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            std::cerr << "ERROR: server socket establishment failed" << std::endl;
            continue;
        }
        
        if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
        	close(sock_fd);
            std::cerr << "ERROR: setsocketopt failed " << std::endl;
            exit(EXIT_FAILURE);
        }

        if (bind(sock_fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sock_fd);
            std::cerr << "ERROR: server bind failed "  << std::endl;
            continue;
        }

        break;
    }

    // All done with this result link list
    freeaddrinfo(servinfo);

    // Did not find a usable IP address
    if (p == NULL) {
        std::cerr << "ERROR: client: failed to connect" << std::endl;
        exit(EXIT_FAILURE);
    }

	// Set socket to listen, which allows 10 connection requests
	if (listen(sock_fd, BACKLOG) == -1) {
        std::cerr << "ERROR: error setting listening feature " << std::endl;
		exit(EXIT_FAILURE);
	}

	std::cout << "server: Server waiting for connections..." << std::endl;

	// accept new connections, always checking this condition
	while (true){

		client_socketFd = accept(sock_fd, (struct sockaddr*)&clientAddr, &clientAddrSize);
		if(client_socketFd == -1){
            std::cerr << "ERROR: failed to accept clients "  << std::endl;
			continue;
		}

		rv = fcntl(client_socketFd, F_SETFL, O_NONBLOCK);
    	if(rv == -1){
            std::cerr << "ERROR: fcntl failed "  << std::endl;
    		close(client_socketFd);
    		continue;
    	}

		connection_count++;

		// Print to standard output the connection info
		inet_ntop(clientAddr.sin_family, &clientAddr.sin_addr, ipstr, sizeof(ipstr));
        std::cout << "Total number of connections: " << connection_count << std::endl;
		std::cout << "About to accept a connection: " << ipstr << ":" << ntohs(clientAddr.sin_port) << std::endl;
		
		connections.push_back(std::thread (connect_func, client_socketFd, connection_count, dir));
	}

}
