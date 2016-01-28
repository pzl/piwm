#include <stdio.h>
#include <stdlib.h> //calloc
#include <string.h> // memset, memcpy
#include <unistd.h> //close, gethostname
#include <errno.h>

#include <sys/types.h> //getaddrinfo()
#include <sys/socket.h>
#include <netdb.h>     //getaddrinfo()

#include <arpa/inet.h>

#include "net.h"

#define PORT "18455"
#define MAX_BACKED_UP_CLIENTS 10

#define BUF_ALLOC_SIZE 4096

#define PACK_HEAD_SIZE 4

/*
	Both functions return pointer to start of packet, change buf_end,
	and change next_boundary to the FOLLOWING packet. Not the one being
	requested. They will detect clean packet ends and reset buf_end to the start
	and next_boundary to NULL accordingly.

	returning NULL means reading is seriously interrupted or impossible, dump the client
*/
static char *recv_new_packet(int sock, Buffer *);
static char *recv_finish_packet(int sock, Buffer *);
static uint32_t packet_size(char *packet);


static uint32_t packet_size(char *packet){
	uint32_t raw_size,
			 size;

	memcpy(&raw_size, packet, sizeof(uint32_t));
	size = ntohl(raw_size);

	return size;
}

/*
	@param:
		sock: socket to read from
		buffer: internal buffer, just keep giving it back to us and don't screw with it
		bufp: we will point this to the start of your requested packet, excluding the header

	@return
		0: Client disconnected or serious error. You should shut down the socket
	   >0: length of packet (in bytes), excluding the header
*/
uint32_t get_packet(int sock, Buffer *buffer, char **bufp) {
	uint32_t pack_size = 0;


	//on first read, buf will be uninitialized
	if (buffer->buf == NULL){
		printf("initializing buffer\n");
		buffer->buf = calloc(BUF_ALLOC_SIZE,1); //init the space we'll need to read everything into

		buffer->next_boundary = NULL; //when NULL, means there isn't a partial packet to pull from
		buffer->buf_end = buffer->buf; //we haven't read anything, so the end is the beginning
	}


	printf("Buffer start addr: %p, Buffer next read addr: %p, Buffer next packet start: %p\n", buffer->buf, buffer->buf_end, buffer->next_boundary);

	//if there are pending packets existing in the buffer
	if (buffer->next_boundary != NULL){
		printf("there may be cached packets\n");

		//check length, download anything necessary to finish
		if (buffer->buf_end - buffer->next_boundary < PACK_HEAD_SIZE){
			printf("don't know the size of cached packet, have to read more\n");
			//not enough of packet read to determine size
			*bufp = recv_finish_packet(sock, buffer);
		} else {
			//we can read the size, see if we have the whole packet already
			pack_size = packet_size(buffer->next_boundary);
			printf("cached packet has size %zu bytes\n", pack_size);


			if ((uint32_t)(buffer->buf_end - buffer->next_boundary) < pack_size){
				printf("but we only have %d read. reading more\n", (buffer->buf_end - buffer->next_boundary));
				// didn't receive enough to match packet size, ask for more
				*bufp = recv_finish_packet(sock,buffer);
			} else {
				printf("cached packet is fully downloaded already.\n");
				//it is fully downloaded already
				*bufp = buffer->next_boundary; //point to the start of it
				buffer->next_boundary += pack_size; // point to the following packet

				// if there *is* no following packet already downloaded, mark everything clean
				if (buffer->next_boundary >= buffer->buf_end){
					printf("no cached packets after this, resetting end and boundary\n");
					buffer->buf_end = buffer->buf;
					buffer->next_boundary = NULL;
				}

			}
		}

	} else {
		printf("no cached packets, reading new ones\n");
		//no packets in the buffer, wait for more
		*bufp = recv_new_packet(sock, buffer);
	}

	if (*bufp == NULL){
		return 0; //client disconnected, or fatal error happened.
	}

	pack_size = packet_size(*bufp);
	*bufp += PACK_HEAD_SIZE;

	printf("returning packet at %p with size %zu (%zu with header)\n", *bufp, (pack_size - PACK_HEAD_SIZE ),pack_size);

	return pack_size - PACK_HEAD_SIZE;
}


static char *recv_new_packet(int sock, Buffer *buffer){
	uint32_t pack_size;
	ssize_t n_recvd;

	printf("starting recv_new_packet, resetting read ptr to %p\n", buffer->buf);

	buffer->buf_end = buffer->buf; //reset "read up to here" pointer

	while ( (n_recvd = recv(sock, buffer->buf_end, (BUF_ALLOC_SIZE - (buffer->buf_end - buffer->buf)), 0)) > 0) {
		/*printf("<");
		for (int i=0; i<n_recvd; i++){
			printf("[%02X]", buffer->buf_end[i]);
		}
		printf(">\n");*/

		buffer->buf_end += n_recvd;
		printf("read %zu raw bytes, moving read ptr to %p\n", n_recvd, buffer->buf_end);



		//check for oversized packets
		if ((buffer->buf_end - buffer->buf) >= BUF_ALLOC_SIZE){
			fprintf(stderr, "packet is larger than buffer!\n");
			return NULL;
		}

		//didn't even get full SIZE header, ask for more
		if ( buffer->buf_end - buffer->buf < PACK_HEAD_SIZE){
			printf("wasn't enough to get header, repeating read\n");
			continue;
		}

		//get the final packet size
		pack_size = packet_size(buffer->buf);
		printf("packet size header read as %zu\n", pack_size);

		//have we read enough to encompass the packet?
		if ( (uint32_t)(buffer->buf_end - buffer->buf) < pack_size){
			printf("total read (%d) less than packet size (%zu). repeating read\n",(buffer->buf_end - buffer->buf),pack_size);
			continue; //not enough yet
		} else {
			printf("read enough for the packet. breaking read loop\n");
			break; //got it!
		}

	}
	if (n_recvd == 0) {
		fprintf(stderr, "client disconnect\n");
		return NULL;
	} else if (n_recvd < 0){
		perror("recv (new packet)");
		return NULL;
	}


	if ((uint32_t)(buffer->buf_end - buffer->buf) < pack_size) { 
		printf("packet under sized. freaking out\n");
		return NULL; //we never got the whole packet, and didn't catch it above. SHUT IT DOWN
	} else if ((uint32_t)(buffer->buf_end - buffer->buf) > pack_size) {
		printf("read more than one packet. Read: %d, packet is only %zu\n", (buffer->buf_end - buffer->buf),pack_size);
		buffer->next_boundary = buffer->buf + pack_size;  //we started reading something else!
	} else {
		printf("read exactly packet length. resetting buf_end and boundary pointers\n");
		//we matched packet length exactly. Next packet can start at buf start
		buffer->buf_end = buffer->buf;
		buffer->next_boundary = NULL;
	}

	//since this is a new packet, it should be placed at the start of the buffer
	return buffer->buf;
}

static char *recv_finish_packet(int sock, Buffer *buffer) {
	char *pack_start;
	uint32_t pack_size;
	ssize_t n_recvd;

	pack_start = buffer->buf_end;

	while ( (n_recvd = recv(sock, buffer->buf_end, (BUF_ALLOC_SIZE - (buffer->buf_end - buffer->next_boundary)), 0)) > 0) {
		buffer->buf_end += n_recvd;

		//check for oversized packets
		if ( buffer->buf_end - buffer->buf >= BUF_ALLOC_SIZE ){
			fprintf(stderr, "packet is larger than buffer!\n");
			return NULL;
		}

		//didn't even get full SIZE header, ask for more
		if ( buffer->buf_end - pack_start < PACK_HEAD_SIZE){
			continue;
		}

		//get the final packet size
		pack_size = packet_size(buffer->buf);

		//have we read enough to encompass the packet?
		if ((uint32_t)(buffer->buf_end - pack_start) < pack_size){
			continue; //not enough yet
		} else {
			break; // we got it! 
		}


	}
	if (n_recvd == 0) {
		fprintf(stderr, "client disconnect\n");
		return NULL;
	} else if (n_recvd < 0){
		perror("recv (finish packet)");
		return NULL;
	}

	if ((uint32_t)(buffer->buf_end - pack_start) < pack_size) {
		return NULL; //didn't get the whole packet, and didn't catch it in the loop. panic
	} else if ((uint32_t)(buffer->buf_end - pack_start) > pack_size) {
		buffer->next_boundary = pack_start + pack_size; //we started reading into the next packet
	} else {
		//we matched packet length exactly. next packet can start at buffer start
		buffer->buf_end = buffer->buf;
		buffer->next_boundary = NULL;
	}

	return pack_start;
}

int setup_socket(void) {
	int sock;
	int result;
	struct addrinfo hints,
					*res;

	// ------ SERVER CONNECTION SETUP

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET6; // IPv4 or IPV6, whatever
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // we will be the server, accept()ing conns

	if ( (result = getaddrinfo(NULL, PORT, &hints, &res)) != 0){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(result));
		return -1;
	}

	if ((sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0){
		fprintf(stderr, "failed creating socket");
		return -1;
	}

	int reuse=1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0){
		perror("setsockopt");
		return -1;
	}
	if (bind(sock, res->ai_addr, res->ai_addrlen) < 0){
		perror("failed binding socket");
		return -1;
	}

	if (listen(sock, MAX_BACKED_UP_CLIENTS) < 0){
		fprintf(stderr, "failed to listen()");
		return -1;
	}
	printf("listening on port %s\n", PORT);


	/*
	char myname[2048];
	if (gethostname(myname, 2048) < 0) {
		perror("gethostname");
	}  // uname -m
	*/



	return sock;
}


void get_client_name(struct sockaddr_storage *remote) {
	char clientname[2048];
	int port;


	if (remote->ss_family == AF_INET) { //IPv4
		struct sockaddr_in *s = (struct sockaddr_in *)remote;
		port = ntohs(s->sin_port);
		if (inet_ntop(AF_INET, &s->sin_addr, clientname, 2048) == NULL){
			perror("inet_ntop");
		}
	} else {
		struct sockaddr_in6 *s = (struct sockaddr_in6 *)remote;
		port = ntohs(s->sin6_port);
		if (inet_ntop(AF_INET6, &s->sin6_addr, clientname, 2048) == NULL){
			perror("inet_ntop 6");
		}
	}

	printf("from %s:%d\n", clientname,port);
}
