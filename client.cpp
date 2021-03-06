/****************************************************
 Deepak Pokharel (dp1602)
 Data Communication
 Professor - Maxwell Young
 Programming assigment 2: Client.cpp to demonstrate 
 GBN Protocol
****************************************************/

#include <stdlib.h>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
#include <fstream>
#include <arpa/inet.h>
#include "packet.h"
#include <math.h>
#include <time.h>
#include <unistd.h>
#include "packet.cpp"

using namespace std;

int main(int argc, char *argv[])
{
	//output the usage when length is not 4
	if (argc != 4)
	{
		printf("%s \n", "Usage: ./client localhost <port number> <filename>");
		exit(1);
	}



	//server address
	struct hostent *s;
	s = gethostbyname(argv[1]);
	int portNo; //port number
	struct sockaddr_in server;
	socklen_t slen = sizeof(server);
	portNo = atoi(argv[2]);


	if(portNo < 1024 || portNo > 65355){
		cout << "Port Error!!!! Must be between 1024 and 65355!!" << endl;
		exit(1);
	}

	bzero((char *)&server, sizeof(server));
	server.sin_family = AF_INET;	 //IPv4
	server.sin_port = htons(portNo); //to network byte order

	bcopy((char *)s->h_addr,
		  (char *)&server.sin_addr.s_addr,
		  s->h_length);

	// setting up a socket
	int udpSocket = 0;
	if ((udpSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		std::cout << "Socket Error: \n";
		exit(1);
	}

	//file descriptor
	FILE *file;					//holding the file to read
	file = fopen(argv[3], "r"); // filename

	//file descriptor
	ofstream seqNumLog; //for clientsequm.log
	ofstream ackLog;	//for clientack.log

	seqNumLog.open("clientseqnum.log"); //opening file
	ackLog.open("clientack.log");		//opening file

	//file opened correctly
	if (file == NULL)
	{
		cout << "Error opening file " << argv[3] << " !" << endl;
		exit(1);
	}
	if (!seqNumLog.good())
	{
		std::cout << "Error opening file clientseqnum.log!" << std::endl;
		exit(1);
	}
	if (!ackLog.good())
	{
		std::cout << "Error opening file clientack.log!" << std::endl;
		exit(1);
	}

	int type = 0;	  //packet type
	int seqnum = -1;  //packet seqnum
	char data[8][31]; //2d array for holding 30 char data

	//setting 2d data array to zero
	for (int j = 0; j < 8; j++)
	{
		memset(data[j], '\0', sizeof(data[j]));
	}

	int length = 30;							  //packet length
	char dataPacket[42];						  //data packet total length
	memset(dataPacket, '\0', sizeof(dataPacket)); //emptying dataPacket Length

	int nextSeqNum = 1; //next sequence number
	int base = 0;		//base
	int N = 7;			//window size

	//creating packet
	packet pack = packet(0, 0, 0, NULL);

	//timer
	struct timeval timer;

	//setting up for select
	fd_set fileDescriptors;
	FD_ZERO(&fileDescriptors);
	FD_SET(udpSocket, &fileDescriptors);
	
	bool endOfFileReached = false; //end of file reached
	int outstandingPacket = 0;	   //no of outstanding packet at the given time
	int receivedData[42];
	cout << "-----------------------------------------" << endl;
	while (1)
	{
		//if end of file is reached and outstanding packet is zero
		if (endOfFileReached && outstandingPacket == 0)
		{
			//increase seqnum
			seqnum++;
			seqnum = seqnum % 8;

			pack = packet(3, seqnum, 0, NULL); //make packet
			pack.serialize(dataPacket);		   //serialize packet

			printf("Sending EOT packet to the server\n");
			//send the packet
			sendto(udpSocket, dataPacket, sizeof(dataPacket), 0, (struct sockaddr *)&server, sizeof(server));

			//print contents;
			pack.printContents();
			//write down the seqenece nubmer;
			seqNumLog << pack.getSeqNum() << endl;
		}

		//if outstanding packet is less than 7 and end of file is not reached
		while (outstandingPacket < 7 && !endOfFileReached)
		{
			
			//starting sequence number
			seqnum++;
			seqnum = seqnum % 8;
			type = 1; 


			//setting to zero
			memset(&dataPacket, 0, sizeof(dataPacket));

			//reading a file
			int readCount = fread(data[seqnum], 1, length, file);
			if (readCount == length)
			{	

				//making a packet
				pack = packet(type, seqnum, length, data[seqnum]);
				pack.serialize(dataPacket);	//serializing a packet

				//send the message to server
				sendto(udpSocket, dataPacket, sizeof(dataPacket), 0, (struct sockaddr *)&server, sizeof(server));
				

				//adding outstanding packets
				outstandingPacket = (outstandingPacket + 1) % 8;
				nextSeqNum = (seqnum + 1) % 8;

				//printing contents
				cout << outstandingPacket << endl;
				pack.printContents();
				cout << "SB: " << base << endl;
				cout << "NS: " << nextSeqNum << endl;
				cout << "Number of Outstanding Packets: " << outstandingPacket << endl;
				cout << "-----------------------------------------" << endl;
				

				//write to seqNumLog file
				seqNumLog << pack.getSeqNum() << endl;
			}
			else
			{
				
				data[seqnum][readCount] = '\0';

				//packing last data
				pack = packet(type, seqnum, readCount, data[seqnum]);

				//serializng the packet
				pack.serialize(dataPacket);

				//send the message to server
				sendto(udpSocket, dataPacket, sizeof(dataPacket), 0, (struct sockaddr *)&server, sizeof(server));
				
				//print contents
				pack.printContents();
				cout << "SB: " << base << endl;
				cout << "NS: " << nextSeqNum << endl;
				cout << "Number of Outstanding Packets: " << outstandingPacket + 1 << endl;

				//writing to the seqnumlog file
				seqNumLog << pack.getSeqNum() << endl;

				//updating outstanding Packet and nextseqnumber
				outstandingPacket = (outstandingPacket + 1) % 8;
				nextSeqNum = (seqnum + 1) % 8;
				endOfFileReached = true;
				break;
			}
		}

		//setting timers
		timer.tv_sec = 2;
		timer.tv_usec = 0;

		//non-blocking listening
		int m = udpSocket;
		int setTimer = select(m + 1, &fileDescriptors, NULL, NULL, &timer);

		//udpSocket is ready
		if (setTimer > 0)
		{	

			//receiving data
			int c = recvfrom(udpSocket, receivedData, sizeof(receivedData), 0, (struct sockaddr *)&server, &slen);
			if (c < 0)
			{

				perror("Receiving error\n");
			}

			//packing data
			packet pack(0, 0, 0, NULL);
			

			//deserializing received data
			pack.deserialize((char *)receivedData);

			pack.printContents();

			//if ack received is type 2
			if (pack.getType() == 2)
			{
				printf("EOT packet received. Exiting----------------------------\n");
				ackLog << pack.getSeqNum() << endl;
				break;
			}
			else
			{	
				//updating base and outstanding packet
				int ackseq = pack.getSeqNum();

				if (ackseq >= base || (ackseq < (base + N) % 8 && ackseq >= 0))
				{
					base = (pack.getSeqNum() + 1) % 8;

					if (nextSeqNum > pack.getSeqNum())
					{
						outstandingPacket = (nextSeqNum - base) % 8;
					}
					else
					{
						outstandingPacket = (N - (abs(nextSeqNum - ackseq))) % 8;
					}
				}


				//printing contents
				printf("SB: %d\n", base);
				printf("NS: %d\n", nextSeqNum);
				printf("Number of outstanding packets: %d\n", outstandingPacket);
				printf("------------------------------------------------------\n");
				ackLog << pack.getSeqNum() << endl;
			}
		}
		//udpsocket error
		else if (setTimer < 0)
		{
			std::cout << "Socket Error \n";
			exit(1);
		}
		//timeout occured
		else
		{
			cout << "\n**********Timeout Occured*******************" << endl;
			cout << "Resending all packets....." << endl;


			//resending the packet from same way as before
			for (int i = (seqnum - 6) % 8; i < seqnum + 1; ++i)
			{
				if (data[i] != NULL)
				{
					pack = packet(type, i, length, data[i]);

					pack.serialize(dataPacket);

					//send the message to server
					sendto(udpSocket, dataPacket, sizeof(dataPacket), 0, (struct sockaddr *)&server, sizeof(server));
					cout << "-----------------------------------------" << endl;
					pack.printContents();
					seqNumLog << pack.getSeqNum() << endl;
				}
			}
		}
	}

	//closing the files and socket
	seqNumLog.close();
	ackLog.close();
	close(udpSocket);
}