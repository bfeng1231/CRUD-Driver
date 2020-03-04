////////////////////////////////////////////////////////////////////////////////
//
//  File          : hdd_client.c
//  Description   : This is the client side of the CRUD communication protocol.
//
//   Author       : Patrick McDaniel
//  Last Modified : Thu Oct 30 06:59:59 EDT 2014
//

// Include Files
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>

// Project Include Files
#include <hdd_network.h>
#include <cmpsc311_log.h>
#include <cmpsc311_util.h>
#include <hdd_driver.h>




int sockfh = -1;

// function to create a buffer given the size
int64_t* createBuffer(int64_t size){
	int64_t* buffer = malloc(sizeof(size));
	return buffer;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : hdd_client_operation
// Description  : This the client operation that sends a request to the CRUD
//                server.   It will:
//
//                1) if INIT make a connection to the server
//                2) send any request to the server, returning results
//                3) if CLOSE, will close the connection
//
// Inputs       : cmd - the request opcode for the command
//                buf - the block to be read/written from (READ/WRITE)
// Outputs      : the response structure encoded as needed
HddBitResp hdd_client_operation(HddBitCmd cmd, void *buf) {
	
	// takes the packed bytes and separates them in to thier values
	int64_t flag = cmd >> 33;
	int64_t op = cmd >> 62;
	int64_t blocksize = cmd >> 36;
	flag = flag & 7;
	op = op & 3;
	blocksize = blocksize & 67108863;

	// checks if flag is set to initalize
	if (flag == HDD_INIT) {
		sockfh = socket(PF_INET, SOCK_STREAM, 0); //creates socket
		//if failed to create, return -1		
		if (sockfh == -1){
			return -1;
		}
		
		// assign values for client
		char *ip = HDD_DEFAULT_IP;
 		unsigned short port = HDD_DEFAULT_PORT;
		struct sockaddr_in caddr;
		caddr.sin_family = AF_INET;
		caddr.sin_port = htons(port);
		
		if(inet_aton(ip, &caddr.sin_addr) == 0){
			return -1;
		}

		if(connect(sockfh, (const struct sockaddr *)&caddr,sizeof(struct sockaddr)) == -1){
			return -1;
		}
	}

	// start send/write to server
	int64_t* data = createBuffer(sizeof(int64_t)); // calls function createBuffer and allocates it with the size of int64
	// calls the function to convert the packed bytes in order to communicate with server
	*data = htonll64(cmd);
	// calls write function to write size of int64 bytes to the buffer
	write(sockfh, data, sizeof(int64_t));
	
	// if the op code is create or overwrite, write blocksize into the block
	if (op == HDD_BLOCK_CREATE || op == HDD_BLOCK_OVERWRITE){
       		write(sockfh, buf, blocksize);
	}
		
	free(data);

	// start read data from the server
	data = createBuffer(sizeof(int64_t));// calls function createBuffer
	// calls function to read size of int64 bytes from the buffer
	int bufread = read(sockfh, data, sizeof(int64_t));
	// calls function to convert the
	int64_t net2host = ntohll64(*data);
	free (data);
	// gets the op code from the converted bytes
	int request = ((net2host >> 62) & 3);
	// gets the blocksize from the converted bytes
	blocksize = ((net2host >> 36) & 67108863);
	// if op code is read, read blocksize bytes from the block
	if (request == HDD_BLOCK_READ) {
        	bufread = read(sockfh, buf, blocksize);
		// while the amount of bytes read is less than the blocksize, increments amount read by the bytes that weren't read before
       		while (bufread < blocksize) {
            		bufread += read(sockfh, &((char *)buf)[bufread], blocksize - bufread);
        	}
	}
	
	// if the flag is save and close, calls function to disconnect
	if (flag == HDD_SAVE_AND_CLOSE){
		close(sockfh);
		sockfh = -1;
	}


	return net2host;
}


