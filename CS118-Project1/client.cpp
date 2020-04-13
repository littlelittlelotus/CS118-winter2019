
//Xiaohe YANG
//504640747
//CS118
#include <string>
#include <thread>
#include <iostream>

#include <fstream>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include <errno.h>


#define BUFFER_SIZE 4096
#define TIMEOUT 15


int main(int argc, char* argv[])
{
    int cv;
	char *host_name, *port, *file_name;
	struct addrinfo hints, *servinfo, *p;

	int sock_fd;
	fd_set write_fds;
	FD_ZERO(&write_fds);

	struct timeval timeout;
	timeout.tv_sec = TIMEOUT;
	timeout.tv_usec = 0;

	char ipstr[INET_ADDRSTRLEN] = {'\0'};

	// Usage check
	if(argc != 4){
		std::cerr << "ERROR: USAGE: ./client <HOSTNAME-OR-IP> <PORT> <FILENAME>\n";
		exit(EXIT_FAILURE);
	}

	host_name = argv[1];
	port = argv[2];
	file_name = argv[3];


	// Port number check.
	if(strtol(port, NULL, 0) < 1024 ){
		std::cerr << "ERROR: port number: Out of range.";
		exit(EXIT_FAILURE);
	}

	// make sure the struct is empty
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;     
	// TCP stream sockets
	hints.ai_socktype = SOCK_STREAM;

	if ((cv = getaddrinfo(host_name, port, &hints, &servinfo)) != 0) {
    	std::cerr << "ERROR: getaddrinfo error: " << gai_strerror(cv) <<std::endl;
    	exit(EXIT_FAILURE);
	}
	
	// loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            std::cerr << "ERROR: failed to establish client socket " <<std::endl;
            continue;
        }

        cv = fcntl(sock_fd, F_SETFL, O_NONBLOCK);
    	if(cv == -1){
    		perror("ERROR: fcntl");
            std::cerr << "ERROR: fcntl failed "  <<std::endl;
    		continue;
    	}

        cv = connect(sock_fd, p->ai_addr, p->ai_addrlen);
        if (cv == -1 && errno != EINPROGRESS) {
        	close(sock_fd);
            std::cerr << "ERROR: client failed to connect to server"  <<std::endl;
 			continue;
    	}
    
        FD_SET(sock_fd, &write_fds);
        cv = select(sock_fd + 1, NULL, &write_fds, NULL, &timeout);
        if(cv == -1){
            std::cerr << "ERROR: calling select "  <<std::endl;
        	close(sock_fd);
        	exit(EXIT_FAILURE);
        }
        else if(cv == 0){
        	std::cerr << "ERROR: connection timeout" << std::endl;
        	close(sock_fd);
        	exit(EXIT_FAILURE);
        }

        break;
    }

	if (p == NULL) {
        std::cerr << "ERROR: client: failed to connect" << std::endl;
        exit(EXIT_FAILURE);
    }


    struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
    inet_ntop(p->ai_family, &(ipv4->sin_addr), ipstr, sizeof(ipstr));
	std::cout << "Set up a connection from: " << ipstr << ":" 
	<< ntohs(ipv4->sin_port) <<std::endl;

	// All done with this result link list
    freeaddrinfo(servinfo);


	char buf[BUFFER_SIZE];
	int bytes_read = 0, bytes_sent = 0, total_bytes_read = 0, total_bytes_sent = 0;


	std::ifstream ifs(file_name, std::fstream::binary);
	if(ifs.fail()){
		std::cerr << "ERROR: " << file_name << " cannot be opened." << std::endl;
		exit(EXIT_FAILURE);
	}

	// Main file send loop
	while(true){
		memset(buf, '\0', sizeof(buf));

		bytes_read = ifs.read(buf, sizeof(buf)).gcount();

		if(bytes_read > 0){

			total_bytes_read += bytes_read;

			bytes_sent = send(sock_fd, buf, bytes_read, 0);
			if (bytes_sent == -1 && errno != EAGAIN && errno != EWOULDBLOCK){
                std::cerr << "ERROR: failed to send file " << gai_strerror(cv) <<std::endl;
				exit(EXIT_FAILURE);
			}

			cv = select(sock_fd + 1, NULL, &write_fds, NULL, &timeout);
        	if(cv == -1){
                std::cerr << "ERROR: failed to call select " << gai_strerror(cv) <<std::endl;
        		close(sock_fd);
        		exit(EXIT_FAILURE);
        	}
        	else if(cv == 0){
        		close(sock_fd);
        		std::cerr << "ERROR: send timeout" << std::endl;
        		exit(EXIT_FAILURE);
        	}
        	//std::cout << "send " << bytes_sent << "bytes" << std::endl;
			total_bytes_sent += bytes_sent;
		}
		else
			break; // Reach EOF
	}



	
	ifs.close();

	close(sock_fd);

	exit(0);
}
