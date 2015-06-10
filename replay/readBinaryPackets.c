#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>

uint8_t readByte(FILE *ptr);
void getBytes(uint8_t packet[], size_t n, FILE *ptr);
void printArray(uint8_t packet[], size_t n);
uint8_t checkPacketHeader(uint8_t packet[], size_t n);
void catch_function(int signo);

uint64_t bytesRead = 0;
int c = 0;

int main(int argc, char *argv[])
{
	// Handle Signal
	signal(SIGTSTP, catch_function);

	// Timing
	clock_t begin, end;
	double time_spent;
	begin = clock();

	uint8_t byte = 0x00;
	FILE *ptr;

	// Open
	ptr = fopen("binaryPackets","rb");

	// Read Zeroes
	int zeroSize = 100;
	uint8_t zeroPacket[zeroSize];
	getBytes(zeroPacket, sizeof(zeroPacket), ptr);

	// Read Boot Packet
	int bootSize = 70;
	uint8_t bootPacket[bootSize];
	getBytes(bootPacket, sizeof(bootPacket), ptr);

	uint8_t lastPacket[2098];

	while(!feof(ptr)){
		// Read Data Packet
		int dataSize = 2098;
		uint8_t dataPacket[dataSize];
		getBytes(dataPacket, sizeof(dataPacket), ptr);

		// Check Data Packet Header
		if(!checkPacketHeader(dataPacket, sizeof(dataPacket))){
			printf("Invalid Data Packet Header!\n");
			// Print
			printArray(lastPacket, sizeof(lastPacket));
			printArray(dataPacket, sizeof(dataPacket));
			break;
		} else {
			// printf("Good Data Packet Header\n");
		}
		memcpy(lastPacket, dataPacket, sizeof(dataPacket));
	}

	// Cleanup
	end = clock();
	time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("Finished: %"PRIu64" bytes read (%f MB) in %f seconds\n", bytesRead, ((bytesRead/1024.)/1024.), time_spent);
	fclose(ptr);
	return 0;
}

uint8_t readByte(FILE *ptr)
{
	bytesRead++;
	uint8_t byte = 0x00;

	// Read
	fread(&byte,sizeof(byte),1,ptr);
	return byte;
}

void getBytes(uint8_t packet[], size_t n, FILE *ptr)
{
	int p = 0;

	while(p < n){
		if(c == 0){
			// Read chunk size
			uint8_t l = readByte(ptr);
			uint8_t h = readByte(ptr);
			// Check for EOF
			if(feof(ptr)){
				printf("End Of File Reached\n");
				break;
			}
			c = l | (h << 8);
			// printf("Reading %i Bytes\n",c);
		} else {
			// Read into buffer
			packet[p] = readByte(ptr);
			// Check for EOF
			if(feof(ptr)){
				printf("End Of File Reached\n");
				break;
			}
			c--;
			p++;
		}
	}
}

void printArray(uint8_t packet[], size_t n)
{
	int i;
	for(i = 0; i < n; i++){
		printf("%x ",packet[i]);
	}
	printf("\n");
}

uint8_t checkPacketHeader(uint8_t packet[], size_t n)
{
	return (packet[0] == 254 && packet[1] == 254 && packet[2] == 254);
}

void catch_function(int signo)
{
	printf("Progress: %"PRIu64" bytes read (%f MB)\n", bytesRead, (bytesRead/1000000.));
}

