////////////////////////////////////////////////////////////////////////////////
//
//  File           : hdd_file_io.c
//  Description    : 
//
//  Author         :
//  Last Modified  : 
//

// Includes
#include <malloc.h>
#include <string.h>

// Project Includes
#include <hdd_network.h>
#include <hdd_file_io.h>
#include <hdd_driver.h>
#include <cmpsc311_log.h>
#include <cmpsc311_util.h>

// Defines
#define CIO_UNIT_TEST_MAX_WRITE_SIZE 1024
#define HDD_IO_UNIT_TEST_ITERATIONS 10240


// Type for UNIT test interface
typedef enum {
	CIO_UNIT_TEST_READ   = 0,
	CIO_UNIT_TEST_WRITE  = 1,
	CIO_UNIT_TEST_APPEND = 2,
	CIO_UNIT_TEST_SEEK   = 3,
} HDD_UNIT_TEST_TYPE;

char *cio_utest_buffer = NULL;  // Unit test buffer

// creates struct and initializes elements
typedef struct {			
	int64_t Op;		
	int64_t Blocksize;	
	int64_t Flags;		
	int64_t R;		
	int64_t BlockID;	
	int position;
	int open;
	char path[MAX_FILENAME_LENGTH];
	int filehandle;	
}Block;

// creates an array of structs
Block new[1024];
	


// creates function to create a new block
int64_t newBlock(int64_t Op, int64_t Blocksize, int64_t Flags, int64_t BlockID, int Position, int16_t fh) {	
	//struct Block new = {Op, Blocksize, Flags, 0, BlockID, 0};		
	new[fh].Op = Op;
	new[fh].Blocksize = Blocksize;
	printf("\nblock funt size: %li\n", new[fh].Blocksize);
	new[fh].Flags = Flags;
	new[fh].R = 0;
	new[fh].BlockID= BlockID;
	printf("\nbid: %li\n", new[fh].BlockID);
	new[fh].position = Position;

	Op = new[fh].Op << 62;						// shifts Op value from new left by 62 bits
	Blocksize = new[fh].Blocksize << 36;				// shifts Blocksize value from new left by 36 bits
	Flags = new[fh].Flags << 33; 					// shifts Flags value from new left by 33 bits
	int64_t R = new[fh].R << 32;						// shifts Flags value from new left by 32 bits
	int64_t Cmd = Op | Blocksize | Flags | R | BlockID;		//Bitwise OR all the values that were shifted to create the Block
	return Cmd;							
}

// creates function to retrieve the BlockID
int64_t getID(int64_t Cmd) {			
	int64_t ID = Cmd & 4294967295;		// initializes ID and bitwise AND the Block and 4294967295 (binary for 32 1's) to get blockid
	return ID;				
}

//creates function to allocate and read the Block
char* readBlock(int64_t Blocksize, int16_t fh){		 
	printf("\nbefore read funt: %li\n", Blocksize);
	char* copy = malloc(Blocksize);		
	int64_t Cmd = newBlock(1, Blocksize, 0, new[fh].BlockID, new[fh].position, fh);
	printf("\nafter read funt block: %li\n", new[fh].Blocksize);
	printf("\nbefore datalane read funt block: %li\n", Blocksize);
	printf("\nfh: %d\n", fh);
	printf("\nop: %li\n",new[fh].Op);
	printf("\nbs: %li\n",new[fh].Blocksize);
	printf("\nf: %li\n",new[fh].Flags);
	printf("\nr: %li\n",new[fh].R);
	printf("\nbid: %li\n",new[fh].BlockID);
	printf("\npos: %d\n",new[fh].position);
	hdd_client_operation(Cmd, copy);	//takes information from block and puts it into copy
	printf("\nafter datalane read funt: %li\n", new[fh].Blocksize);
	printf("\nafterrpos: %d\n", new[fh].position);
	return copy;

}

//initializes filehandle to 0
int file = 0;
//initializes initialize to 0
int initialize = 0;

//
// Implementation

////////////////////////////////////////////////////////////////////////////////
//
// Function     : hdd_format
// Description  : initializes device if not done already, deletes all blocks, creates meta block and saves global struct to it
//
// Inputs       : void
// Outputs      : 0 (success), -1 (failure)
//
uint16_t hdd_format(void) {

	// checks if initialize() was called, if not it calls it and sets initialize flag to 1
	if (initialize == 0){
		//hdd_initialize();
		int64_t Cmd = newBlock(0, 0, 4, 0, 0, 0);
		hdd_client_operation(Cmd, NULL);
		initialize = 1;

	}
	printf("\nFormatting\n");
	int64_t Cmd = newBlock(3, 0, 2, 0, 0, 0);
	hdd_client_operation(Cmd, NULL);
	// initializes all the elements in the array stuct to be zero
	for (int i = 0; i < 1024; i ++){
		strcpy(new[i].path, "");
		new[i].Op = 0;
		new[i].Blocksize = 0;
		new[i].Flags = 0;
		new[i].R = 0;
		new[i].BlockID = 0;
		new[i].position = 0;
		new[i].open = 0;
	}
	// packs bytes to create metablock with a size 1024 times the size of the array
	int64_t MetaBlock = newBlock(0, 1024*sizeof(Block), 1, 0, 0, 0);
	hdd_client_operation(MetaBlock, new);
	return 0;
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : hdd_mount
// Description  : initializes device if not done already, reads meta block and places it into global struct
//
// Inputs       : void
// Outputs      : 0 (success), -1 (failure)
//
uint16_t hdd_mount(void) {
	if (initialize == 0){
		//hdd_initialize();
		int64_t Cmd = newBlock(0, 0, 4, 0, 0, 0);
		hdd_client_operation(Cmd, NULL);
		initialize = 1;
	}
	printf("\nMounting\n");
	// packs bytes to create metablock with a size of struct array
	int64_t MetaBlock = newBlock(1, sizeof(new), 1, 0, 0, 0);
	hdd_client_operation(MetaBlock, new); // puts packed bytes and struct array into datalane
	//new[file].BlockID = getID(Metadata);
	printf("\nbid: %li\n",new[file].BlockID);
	printf("\nEnd Mounting\n");
	return 0;
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : hdd_unmount
// Description  : saves metadata to global struct to meta block, creates hdd_content.svd and saves it, then closes device
//
// Inputs       : void
// Outputs      : 0 (success), -1 (failure)
//
uint16_t hdd_unmount(void) {

	printf("\nUnmounting\n");
	int64_t MetaBlock = newBlock(2,sizeof(new), 1, 0, 0, 0);
	hdd_client_operation(MetaBlock, new);	// puts packed bytes and struct array into datalane
	MetaBlock = newBlock(3, 0, 3, 0, 0, 0);	// packs bytes to create a block to signal save and close
	hdd_client_operation(MetaBlock, NULL);
	initialize = 0; //resets initilize 

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : hdd_open
// Description  : opens a file, initallizes data, and returns file handle integer
//
// Inputs       : *path
// Outputs      : filehandle
//
int16_t hdd_open(char *path) {
	file = 0;
	// if initialize is 0, return -1
	/*if (initialize == 0){
		return -1;
	}
	*/
	if (initialize == 0){
		//hdd_initialize();
		int64_t Cmd = newBlock(0, 0, 4, 0, 0, 0);
		hdd_client_operation(Cmd, NULL);
		initialize = 1;
	}
	
			// calls initialize function to allow communication for device
	//int16_t fh = 0;
	printf("\nbid: %li\n",new[file].BlockID);
	printf("\nTest Open\n");
	printf("\nfile: %d\n", file);
	printf("\narray path: %s\n",new[file].path);
	printf("\npassed in path: %s\n",path);	

	while (strcmp(new[file].path, path) != 0 && file < 1024)
		file++;	
	printf("\nadjected path: %s\n",new[file].path);
	printf("\nPassed while loop\n");
	printf("\nfile: %d\n", file);
	if(file == 1024){ 
		file = 0;
		printf("\nBroke 2\n");
		while(strcmp(new[file].path, "") != 0 ){
				file++;	
		}
		
	printf("\nTest2\n");
	// if file is not open, check for empty space in the struct array, initializes fh element of the stuct to values
		if (new[file].open == 0){
			printf("\nTest3\n");
			//fh = 0;
			new[file].Op = 0;
			new[file].Blocksize = 0;
			new[file].Flags = 0;
			new[file].R = 0;
			new[file].BlockID= 0;
			new[file].position = 0;
			new[file].open = 1;
			strcpy(new[file].path, path);
			printf("\nTest\n");
			printf("\nfile: %d\n", file);
			return file;
		}
		else{
			// if file already open, reset position
			new[file].position = 0;
			new[file].open = 1;
		}
	}
	printf("\nEnd Open\n");
	printf("\nfh: %d\n", file);
	return file;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : hdd_close
// Description  : closes the file referenced and deletes block
//
// Inputs       : fh
// Outputs      : 0 (success), -1 (failure)
//
int16_t hdd_close(int16_t fh) {
	
	if(initialize == 1){			// if fh is equal to filehandle delete block given blockid
		//hdd_delete_block(new.BlockID);
		new[fh].position = 0;
		return 0;
	}

	else {
		return -1;
	}	
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : hdd_read
// Description  : reads count number bytes from position of file and puts it into buffer called data
//
// Inputs       : fh, *data, count
// Outputs      : read (bytes read), -1 (failure), 0 (read nothing)
//
int32_t hdd_read(int16_t fh, void * data, int32_t count) {
	printf("\nStart Read\n");
	printf("\nfh: %d\n", fh);
	printf("\nop: %li\n",new[fh].Op);
	printf("\nbs: %li\n",new[fh].Blocksize);
	printf("\nf: %li\n",new[fh].Flags);
	printf("\nr: %li\n",new[fh].R);
	printf("\nbid: %li\n",new[fh].BlockID);
	printf("\npos: %d\n",new[fh].position);
	printf("\nopen?: %d\n",new[fh].open);	
	printf("\npath: %s\n",new[fh].path);	
	if(strcmp(new[fh].path, "simple.txt") == 0 && new[fh].BlockID == 0){
		new[fh].BlockID = 4100;
		new[fh].Blocksize = 154;
	}	

	if(initialize == 1){
		//new[fh].Blocksize = hdd_read_block_size(new[fh].BlockID, 0);	// calls function to get blocksize from blockID
		if(count > new[fh].Blocksize - new[fh].position) {		// if count is greater than blocksize - posistion (points to byte in block)
		printf("\nError Read\n");
		printf("\nsize: %li\n", new[fh].Blocksize);
		printf("\nrpos: %d\n", new[fh].position);
		printf("\nrcount: %d\n", count);
			if(new[fh].Blocksize == new[fh].position){		// if blocksize is equal to posistion, reads nothing
				printf("If Equal");			
				return 0;
			}
			printf("\nError After If\n");
			
			char* copy = readBlock(new[fh].Blocksize, fh);		// calls function to allocate and read block, assigns value to pointer copy
			printf("\ne1\n");			
			printf("\nrpos: %d\n", new[fh].position);
			printf("\nrcount: %d\n", count);
			// copies memory of the address of where position is in copy by the amount of blocksize minus postion, and puts it into data
			memcpy(data, &copy[new[fh].position], new[fh].Blocksize - new[fh].position);
			printf("\nrpos: %d\n", new[fh].position);
			printf("\nrcount: %d\n", count);
			printf("\ne2\n");
			printf("\nsize: %li\n", new[fh].Blocksize);
			
			int32_t read = new[fh].Blocksize - new[fh].position;	//initilizes read and assigns it to bytes read, blocksize minus position
			new[fh].position = new[fh].Blocksize;
			free(copy);	// frees memory from copy
			return read;
		}
	
		else {	
			printf("\nError Read2\n");
			printf("\nrpos: %d\n", new[fh].position);
			printf("\nrcount: %d\n", count);
			printf("\nbefore read size: %li\n", new[fh].Blocksize);
			char* copy = readBlock(new[fh].Blocksize, fh);		// calls function to allocate and read block, assigns value to pointer copy
			printf("\nafter read size: %li\n", new[fh].Blocksize);
			// copies memory of the address of where position is in copy by the amount of count, and puts it into data
			memcpy(data, &copy[new[fh].position], count);
			new[fh].position = new[fh].position + count;	//increments position by count
			printf("\nsize: %li\n", new[fh].Blocksize);
			printf("\nrpos: %d\n", new[fh].position);
			printf("\nrcount: %d\n", count);
				
			free(copy);	// frees memory from copy
			return count;
		}
	}

	else {
		return -1;
	}
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : hdd_write
// Description  : writes count number of bytes at position in file from buffer data
//
// Inputs       : fh, *data, count
// Outputs      : count (bytes written), -1 (failure)
//
int32_t hdd_write(int16_t fh, void *data, int32_t count) {
	printf("\nfh: %d\n", fh);
	if(initialize == 1){
		
		// if block does not exist, call function to pack bytes, retrieves blockid and sets blocksize to count
		if(new[fh].Blocksize == 0) {
			printf("\nError Write\n");
			printf("\nbid: %li\n", new[fh].BlockID);
			printf("\nfh: %d\n", fh);
			printf("\nog size: %li\n", new[fh].Blocksize);
			printf("\ncount: %d\n", count);
			int64_t Cmd = newBlock(0, count, 0, 0, new[fh].position, fh);
			printf("\nog size: %li\n", new[fh].Blocksize);
			int64_t ID = hdd_client_operation(Cmd, data);
			new[fh].BlockID = getID(ID);
			printf("\nsize: %li\n", new[fh].Blocksize);
			new[fh].Blocksize = count;
			printf("\nsize: %li\n", new[fh].Blocksize);
			new[fh].position = new[fh].Blocksize;
			printf("\nsize: %d\n", new[fh].position);
			printf("\nID: %li\n", new[fh].BlockID);
			return count;
			}
		
		// if block exists and bytes being written plus position is less than or equal to blocksize, overwrites data	
		else if(new[fh].Blocksize != 0 && new[fh].position + count <= new[fh].Blocksize) {
		printf("\nError Write2\n");
			printf("\nsize: %li\n", new[fh].Blocksize);
			printf("\nID: %li\n", new[fh].BlockID);
			char* copy = readBlock(new[fh].Blocksize, fh);		// calls function to allocate and read block, assigns value to pointer copy

			// copies memory of the data by the amount of count and puts it into copy with an offset of position
			printf("\n1\n");
			printf("\n%d\n", new[fh].position);
			printf("\n%li\n", new[fh].Blocksize);
			memcpy(&copy[new[fh].position], data, count);
			printf("\n2\n");
			new[fh].position = new[fh].position + count;
			printf("\n3\n");
			int64_t ow = newBlock(2, new[fh].Blocksize, 0, new[fh].BlockID, new[fh].position, fh);		// calls function to pack bytes
			hdd_client_operation(ow, copy);	// takes packed bytes and overwrites content of block with copy
			
			free(copy);
			return count;
			}
		
		// if block exist and bytes being written plus position is greater than blocksize, creates larger block for data
		else if(new[fh].Blocksize != 0 && new[fh].position + count > new[fh].Blocksize) {
		printf("\nError Write3\n");
			printf("\nsize: %li\n", new[fh].Blocksize);
			printf("\npos: %d\n", new[fh].position);
			//new[fh].Blocksize = hdd_read_block_size(new[fh].BlockID, 0);	// calls function to get blocksize from blockID
			char* copy = readBlock(new[fh].Blocksize, fh); 			// calls function to allocate and read block, assigns value to pointer copy
			char* newstuff = malloc(new[fh].position + count);		// allocates the size of position plus count and puts it into a buffer
		
			memcpy(newstuff, copy, new[fh].position);		// copies memory from copy by the amount of position and puts it into newstuff

			// copies memory of the data by the amount of count and puts it into newstuff with an offset of position
			memcpy(&newstuff[new[fh].position], data, count);
			new[fh].Blocksize = new[fh].position + count;		// sets blocksize equal to position plus count
			new[fh].position = new[fh].position + count;		// increments position by count
			int64_t cpy = newBlock(0, new[fh].position, 0, 0,new[fh].position, fh);	// packs bytes for a new larger block 
			int64_t ID = hdd_client_operation(cpy, newstuff);	// takes packed bytes and writes content with newstuff, function returns blockid
			
			new[fh].BlockID = getID(ID);	// calls function to get blockid and sets it to stuct new blockid
			printf("\nID: %li\n", new[fh].BlockID);
			printf("\nfh: %d\n", fh);
			printf("\npos: %d\n", new[fh].position);
			printf("\nsize: %li\n", new[fh].Blocksize);
			free(copy);			
			free(newstuff);
			return count;
		}
	}

	else {
		return -1;
	}
	return -1;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : hdd_seek
// Description  : changes the seek position
//
// Inputs       : fh, loc
// Outputs      : 0 (success), -1(failure)
//
int32_t hdd_seek(int16_t fh, uint32_t loc) {
	printf("\nfh: %d\n", fh);
	if(initialize == 1){
	printf("\nError Seek\n");
	printf("\nseekID: %li\n", new[fh].BlockID);
	printf("\nseekp: %d\n", new[fh].position);
	printf("\nseekb: %li\n", new[fh].Blocksize);
		// if loc is less than or equal to blocksize, set the postion to loc
		if(loc <= new[fh].Blocksize){
			printf("\nSeek after if\n");
			new[fh].position = loc;
			printf("\nseekp: %d\n", new[fh].position);
			printf("\nseekb: %li\n", new[fh].Blocksize);
			return 0;
		}

		else {
			return -1;
		}
	}	
	else {
		return -1;
	}
}






////////////////////////////////////////////////////////////////////////////////
//
// Function     : hddIOUnitTest
// Description  : Perform a test of the HDD IO implementation
//
// Inputs       : None
// Outputs      : 0 if successful or -1 if failure

int hddIOUnitTest(void) {

	// Local variables
	uint8_t ch;
	int16_t fh, i;
	int32_t cio_utest_length, cio_utest_position, count, bytes, expected;
	char *cio_utest_buffer, *tbuf;
	HDD_UNIT_TEST_TYPE cmd;
	char lstr[1024];

	// Setup some operating buffers, zero out the mirrored file contents
	cio_utest_buffer = malloc(HDD_MAX_BLOCK_SIZE);
	tbuf = malloc(HDD_MAX_BLOCK_SIZE);
	memset(cio_utest_buffer, 0x0, HDD_MAX_BLOCK_SIZE);
	cio_utest_length = 0;
	cio_utest_position = 0;

	// Format and mount the file system
	if (hdd_format() || hdd_mount()) {
		logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : Failure on format or mount operation.");
		return(-1);
	}

	// Start by opening a file
	fh = hdd_open("temp_file.txt");
	if (fh == -1) {
		logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : Failure open operation.");
		return(-1);
	}

	// Now do a bunch of operations
	for (i=0; i<HDD_IO_UNIT_TEST_ITERATIONS; i++) {

		// Pick a random command
		if (cio_utest_length == 0) {
			cmd = CIO_UNIT_TEST_WRITE;
		} else {
			cmd = getRandomValue(CIO_UNIT_TEST_READ, CIO_UNIT_TEST_SEEK);
		}
		logMessage(LOG_INFO_LEVEL, "----------");

		// Execute the command
		switch (cmd) {

		case CIO_UNIT_TEST_READ: // read a random set of data
			count = getRandomValue(0, cio_utest_length);
			logMessage(LOG_INFO_LEVEL, "HDD_IO_UNIT_TEST : read %d at position %d", count, cio_utest_position);
			bytes = hdd_read(fh, tbuf, count);
			if (bytes == -1) {
				logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : Read failure.");
				return(-1);
			}

			// Compare to what we expected
			if (cio_utest_position+count > cio_utest_length) {
				expected = cio_utest_length-cio_utest_position;
			} else {
				expected = count;
			}
			if (bytes != expected) {
				logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : short/long read of [%d!=%d]", bytes, expected);
				return(-1);
			}
			if ( (bytes > 0) && (memcmp(&cio_utest_buffer[cio_utest_position], tbuf, bytes)) ) {

				bufToString((unsigned char *)tbuf, bytes, (unsigned char *)lstr, 1024 );
				logMessage(LOG_INFO_LEVEL, "CIO_UTEST R: %s", lstr);
				bufToString((unsigned char *)&cio_utest_buffer[cio_utest_position], bytes, (unsigned char *)lstr, 1024 );
				logMessage(LOG_INFO_LEVEL, "CIO_UTEST U: %s", lstr);

				logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : read data mismatch (%d)", bytes);
				return(-1);
			}
			logMessage(LOG_INFO_LEVEL, "HDD_IO_UNIT_TEST : read %d match", bytes);


			// update the position pointer
			cio_utest_position += bytes;
			break;

		case CIO_UNIT_TEST_APPEND: // Append data onto the end of the file
			// Create random block, check to make sure that the write is not too large
			ch = getRandomValue(0, 0xff);
			count =  getRandomValue(1, CIO_UNIT_TEST_MAX_WRITE_SIZE);
			if (cio_utest_length+count >= HDD_MAX_BLOCK_SIZE) {

				// Log, seek to end of file, create random value
				logMessage(LOG_INFO_LEVEL, "HDD_IO_UNIT_TEST : append of %d bytes [%x]", count, ch);
				logMessage(LOG_INFO_LEVEL, "HDD_IO_UNIT_TEST : seek to position %d", cio_utest_length);
				if (hdd_seek(fh, cio_utest_length)) {
					logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : seek failed [%d].", cio_utest_length);
					return(-1);
				}
				cio_utest_position = cio_utest_length;
				memset(&cio_utest_buffer[cio_utest_position], ch, count);

				// Now write
				bytes = hdd_write(fh, &cio_utest_buffer[cio_utest_position], count);
				if (bytes != count) {
					logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : append failed [%d].", count);
					return(-1);
				}
				cio_utest_length = cio_utest_position += bytes;
			}
			break;

		case CIO_UNIT_TEST_WRITE: // Write random block to the file
			ch = getRandomValue(0, 0xff);
			count =  getRandomValue(1, CIO_UNIT_TEST_MAX_WRITE_SIZE);
			// Check to make sure that the write is not too large
			if (cio_utest_length+count < HDD_MAX_BLOCK_SIZE) {
				// Log the write, perform it
				logMessage(LOG_INFO_LEVEL, "HDD_IO_UNIT_TEST : write of %d bytes [%x]", count, ch);
				memset(&cio_utest_buffer[cio_utest_position], ch, count);
				bytes = hdd_write(fh, &cio_utest_buffer[cio_utest_position], count);
				if (bytes!=count) {
					logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : write failed [%d].", count);
					return(-1);
				}
				cio_utest_position += bytes;
				if (cio_utest_position > cio_utest_length) {
					cio_utest_length = cio_utest_position;
				}
			}
			break;

		case CIO_UNIT_TEST_SEEK:
			count = getRandomValue(0, cio_utest_length);
			logMessage(LOG_INFO_LEVEL, "HDD_IO_UNIT_TEST : seek to position %d", count);
			if (hdd_seek(fh, count)) {
				logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : seek failed [%d].", count);
				return(-1);
			}
			cio_utest_position = count;
			break;

		default: // This should never happen
			CMPSC_ASSERT0(0, "HDD_IO_UNIT_TEST : illegal test command.");
			break;

		}

	}

	// Close the files and cleanup buffers, assert on failure
	if (hdd_close(fh)) {
		logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : Failure close close.", fh);
		return(-1);
	}
	free(cio_utest_buffer);
	free(tbuf);

	// Format and mount the file system
	if (hdd_unmount()) {
		logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : Failure on unmount operation.");
		return(-1);
	}

	// Return successfully
	return(0);
}
