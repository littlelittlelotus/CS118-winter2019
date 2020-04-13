XIAOHE YANG, 504640737
Design:
# A server and client model to run TCP protocol was implemented. The network can support multiple clients connecting at the same time, small and large files, delayed network, and the server/client will time out after the connected clients don't sent any files in 15s. The files sent from client would be saved in a directpory specified by the server. The server will keep track of how many bytes received and how many files have been received, by labeling the received file the number it is received. When receiving a SIGNAL interrupt, the network shuts down with exit code 0. Other cases of normal disconnection, exit code is also 0. In case of error, error code would be printed to cerror with exit code EXIT_FAILURE.

Problems:
	1.non-blocking socket has EAGAIN and EWOULDBLOCK error codes, I read some online posts about it
	https://www.ibm.com/developerworks/community/blogs/IMSupport/entry/WHY_DOES_SEND_RETURN_EAGAIN_EWOULDBLOCK?lang=en
	2.Correct usage of select(). http://www.cnblogs.com/kex1n/p/7455987.html
	3.Usage of TC to test timeout. Asked TA for help, used Week3 discussion clides as well.

Additional libraries:
	#include <fcntl.h>
        #include <errno.h>
        #include <vector>
	#include <fstream>
	#include <signal.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <sys/select.h>
	#include <sys/time.h>
	#include <unistd.h>
	#include <string.h>
	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <netdb.h>

Acknowledgement:

	1. Cplusplus reference for thread and fstream.
	2. https://www.geeksforgeeks.org/socket-programming-cc/
	3. TCP Connecton credit Beej's Socket Programming Guide.
	   https://beej.us/guide/bgnet/examples/showip.c
	4. Linus Man Page https://linux.die.net/man/8/tc
 	5. This Berkely implementation about socket programming
	https://www.eecs.yorku.ca/course_archive/2011-12/W/3214/showipv4.c
	6. https://www.mkssoftware.com/docs/man3/select.3.asp







