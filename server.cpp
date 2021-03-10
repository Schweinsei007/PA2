/**************************************************
 * Deepak Pokharel (dp1602)
 * 
 * Programming Assignment 2
 * This is a server program that sends files over UDP by using GBN protocol
 * October 13, 2020
 * ********************************************/
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <fstream>
#include "packet.h"
#include "packet.cpp"

using namespace std;

int main(int argc, char *argv[])
{

  struct sockaddr_in client, server;
  socklen_t clen = sizeof(client);
  socklen_t slen = sizeof(server);

  // create socket first
  int portno; // port number
  portno = atoi(argv[1]);
  if (portno < 1024 || portno > 65355)
  {
    perror("Wrong port no\n");
    return -1;
  }
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(portno);

  //data is sent over udp network
  int udp_socket = 0;
  if ((udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
  { // socket(int domain, int type, int protocol)
    cout << "Error in socket creation.\n";
  }
  server.sin_port = htons(portno); // set port number

  if ((bind(udp_socket, (struct sockaddr *)&server, sizeof(server))) < 0) // if binding error occurs
  {
    perror("binding");
    return -1;
  }

  // create out file to write character from a file
  ofstream outfile;
  outfile.open(argv[2]); // opens a file
  if (!outfile.is_open())
  {
    perror("Outfile open error");
  }

  // ack file to write ack sent to the client
  ofstream ackFile;
  ackFile.open("arrival.log");

  if (!ackFile.is_open())
  {
    perror("Arrivalfile open error");
  }

  char receiveData[37]; // to receive from packet
  char charData[30];    // character in the packet
  char ackData[42];     // to send ack data to client

  int type = 1;
  int seqnum = 0;
  int acktype = 0;
  int expectseq = 0;

  packet filepacket(type, seqnum, sizeof(charData), charData);

  while (1)
  {
    memset(receiveData, 0, sizeof(receiveData));
    memset(charData, 0, sizeof(charData));
    memset(ackData, 0, sizeof(ackData));

    // error in receiving from client
    if (recvfrom(udp_socket, receiveData, sizeof(receiveData), 0, (struct sockaddr *)&client, &clen) == -1)
    {
      perror("Receiving\n");
      return -1;
    }

    filepacket.deserialize((char *)receiveData); // deserialize to see sent data
    type = filepacket.getType();                 //get Type
    seqnum = filepacket.getSeqNum();

    ackFile << seqnum << "\n";
    printf("\n--------------------------------------\n");
    filepacket.printContents();

    printf("\nExpecting Rn: %d\n", expectseq);
    printf("sn: %d\n", seqnum);

    if (expectseq == seqnum) //  if expected sequence matches sequence number from client file
    {

      if (filepacket.getType() == 3) // if type sent by client is 3 then do EOT
      {

        acktype = 2; // set acktype to 2
        packet ackpacket(acktype, seqnum, 0, 0);
        ackpacket.serialize(ackData); //searialize to send data
        if (sendto(udp_socket, ackData, sizeof(ackData), 0, (struct sockaddr *)&client, sizeof(client)) == -1)
        {
          perror("Sending Error");
          return -1;
        }
        ackpacket.printContents(); //print sent data
        printf("--------------------------------------\n");
        break; // exit
      }

      outfile << filepacket.getData(); // write to the file

      packet ackpacket(acktype, seqnum, 0, 0); // send only acktype and seq number
      ackpacket.serialize((char *)ackData);    // searilize to send data in the socket
      //send ack to server
      if (sendto(udp_socket, ackData, sizeof(ackData), 0, (struct sockaddr *)&client, sizeof(client)) == -1)
      {
        printf("Sending Error\n");
        return -1;
      }

      ackpacket.printContents();
      expectseq = (expectseq + 1) % 8;
    }
    else // if sequence number doesnot resend the ack to the server
    {
      packet ackpacket(acktype, expectseq, 0, 0);
      ackpacket.serialize((char *)ackData);
      sendto(udp_socket, ackData, sizeof(ackData), 0, (struct sockaddr *)&client, sizeof(client));
      ackpacket.printContents();
      printf("-------------------------------------\n");
    }
  }

  outfile.close(); //close the file

  close(udp_socket); // close the socket
}