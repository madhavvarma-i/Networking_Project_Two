#define WIN32_LEAN_AND_MEAN			// Strip rarely used calls
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <thread>
#include<vector>
#include<map>
#include<iostream>
//#include<buffer.h>
#include "authentication.pb.h"
#include "protocol.pb.h"

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "5150"
#define DEFAULT_PORT_FOR_AUTHENTICATION "5155"
SOCKET clientSocket = INVALID_SOCKET;

int send_and_receive(SOCKET acceptSocket);
std::vector<std::string> rooms;
std::vector<SOCKET> myclients;
std::map<std::string, std::vector<SOCKET>> roomsandclients;
std::map<std::string, SOCKET> clientsocketinfo;
//bufferClass* mybuffer = new bufferClass(20);
//void broadcast(std::string msgroom,std::string msgstring);
void broadcast(std::string room, const char* buf, size_t size);

int main(int argc, char** argv)
{

	//Authentication server connection code

	WSADATA wsaData;
	int iResult = 0;
	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		// Something went wrong, tell the user the error id
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}
	else
	{
		//printf("WSAStartup() was successful!\n");
	}
	//int iResult = 0;
	

	struct addrinfo* result = NULL;
	struct addrinfo* ptr = NULL;
	struct addrinfo hints1;

	ZeroMemory(&hints1, sizeof(hints1));
	hints1.ai_family = AF_UNSPEC;
	hints1.ai_socktype = SOCK_STREAM;
	hints1.ai_protocol = IPPROTO_TCP;

	iResult = getaddrinfo("127.0.0.1", DEFAULT_PORT_FOR_AUTHENTICATION, &hints1, &result);
	if (iResult != 0)
	{
		printf("getaddrinfo() failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}
	else
	{
		//printf("getaddrinfo() successful!\n");
	}

	for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
	{
		// Create a SOCKET for connecting to the server
		clientSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (clientSocket == INVALID_SOCKET)
		{
			printf("socket() failed with error code %d\n", WSAGetLastError());
			freeaddrinfo(result);
			WSACleanup();
			return 1;
		}

		// Attempt to connect to the server
		iResult = connect(clientSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR)
		{
			printf("connect() failed with error code %d\n", WSAGetLastError());
			closesocket(clientSocket);
			clientSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}
	freeaddrinfo(result);

	if (clientSocket == INVALID_SOCKET)
	{
		printf("Unable to connect to the server!\n");
		WSACleanup();
		return 1;
	}
	printf("Successfully connected to the server on socket %d!\n", (int)clientSocket);
	//iResult = send(clientSocket, "connected from chat server", 26, 0);


	while (true)
	{
		WSADATA wsaData;
		int iResult;

		// Initialize Winsock
		iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iResult != 0)
		{
			// Something went wrong, tell the user the error id
			printf("WSAStartup failed with error: %d\n", iResult);
			return 1;
		}
		else
		{
			//printf("WSAStartup() was successful!\n");
		}

		// #1 Socket
		SOCKET listenSocket = INVALID_SOCKET;
		SOCKET acceptSocket = INVALID_SOCKET;
		

		struct addrinfo* addrResult = NULL;
		struct addrinfo hints;

		// Define our connection address info 
		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_PASSIVE;

		// Resolve the server address and port
		iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &addrResult);
		if (iResult != 0)
		{
			printf("getaddrinfo() failed with error %d\n", iResult);
			WSACleanup();
			return 1;
		}
		else
		{
			//printf("getaddrinfo() is good!\n");
		}

		// Create a SOCKET for connecting to the server
		listenSocket = socket(
			addrResult->ai_family,
			addrResult->ai_socktype,
			addrResult->ai_protocol
		);
		if (listenSocket == INVALID_SOCKET)
		{
			// https://docs.microsoft.com/en-us/windows/win32/winsock/windows-sockets-error-codes-2
			printf("socket() failed with error %d\n", WSAGetLastError());
			freeaddrinfo(addrResult);
			WSACleanup();
			return 1;
		}
		else
		{
			//printf("socket() is created!\n");
		}

		// #2 Bind - Setup the TCP listening socket
		iResult = bind(
			listenSocket,
			addrResult->ai_addr,
			(int)addrResult->ai_addrlen
		);
		if (iResult == SOCKET_ERROR)
		{
			printf("bind failed with error: %d\n", WSAGetLastError());
			freeaddrinfo(addrResult);
			closesocket(listenSocket);
			WSACleanup();
			return 1;
		}
		else
		{
			//printf("bind() is good!\n");
		}

		// We don't need this anymore
		freeaddrinfo(addrResult);

		// #3 Listen
		iResult = listen(listenSocket, SOMAXCONN);
		if (iResult == SOCKET_ERROR)
		{
			printf("listen() failed with error %d\n", WSAGetLastError());
			closesocket(listenSocket);
			WSACleanup();
			return 1;
		}
		else
		{
			//printf("listen() was successful!\n");
		}

		// #4 Accept		(Blocking call)
		printf("Waiting for client to connect...\n");
		acceptSocket = accept(listenSocket, NULL, NULL);
		if (acceptSocket == INVALID_SOCKET)
		{
			printf("accept() failed with error: %d\n", WSAGetLastError());
			closesocket(listenSocket);
			WSACleanup();
			return 1;
		}
		else
		{
			//printf("accept() is OK!\n");
			//myvector.push_back(acceptSocket);
			//printf("Accepted client on socket %d\n", acceptSocket);
		}

		// No longer need server socket
		closesocket(listenSocket);
		//printf("press 0 to broadcast");
		//int choice;
		//std::cin >> choice;
		
		//if (choice == 0)
		//{
		//	for (SOCKET acpskt : myvector)
		//	{
		//		int result = send(acpskt, "broadcasting", 12, 0);
		//	}
		//}
		
		std::thread t1(send_and_receive, acceptSocket);
		t1.detach();
		//printf("waiting for new clients \n");

	}
	
	return 0;
}

int send_and_receive(SOCKET acceptSocket)
{
	int iResult = 0;
	// #5 recv & send	(Blocking calls)
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;
	chat::Client* client = new chat::Client();
	server::createAccountWeb* account = new server::createAccountWeb();
	server::AuthenticateWeb* accnt = new server::AuthenticateWeb();
	const std::string mymessage;
	int iSendResult;
	std::string buffer;
	do
	{
		void* buf;
		printf("Waiting to receive data from the client...\n");
		iResult = recv(acceptSocket,recvbuf,recvbuflen, 0);
		client->ParseFromArray((void*)recvbuf, iResult);
		
		//std::cout << "received: "<<recvbuf << std::endl;
		if (iResult > 0)
		{
			buf = malloc(iResult);
			buf = (void*)recvbuf;
			if (client->requestid() == 0)
			{
				iResult = send(clientSocket, (char*)buf, iResult, 0);
				
				
				//client->Clear();
				if (iResult > 0)
				{
					iResult = recv(clientSocket, recvbuf, recvbuflen, 0);

					buf = malloc(iResult);
					buf = (void*)recvbuf;
					
					if (iResult > 0)
					{
						iResult = send(acceptSocket, (char*)buf, iResult, 0);
					}
				}
			}

			else if (client->requestid() == 1)
			{
				iResult = send(clientSocket, (char*)buf, iResult, 0);
				client->Clear();
				if (iResult > 0)
				{
					iResult = recv(clientSocket, recvbuf, sizeof(recvbuf), 0);

					buf = malloc(iResult);
					buf = (void*)recvbuf;

					if (iResult > 0)
					{
						iResult = send(acceptSocket, (char*)buf, iResult, 0);

					}
				}

			}
			else
			{
				if (client->type() == chat::Client_MessageType_JOIN_ROOM)
				{
					if (rooms.empty())
					{
						rooms.push_back(client->roomname());
						std::vector<SOCKET> temp;
						temp.push_back(acceptSocket);
						roomsandclients.insert(std::pair<std::string,std::vector<SOCKET>>(client->roomname(), temp));
						clientsocketinfo.insert(std::pair<std::string, SOCKET>(client->name(), acceptSocket));
					}
					else 
					{
						std::vector<std::string>::iterator it = std::find(rooms.begin(), rooms.end(), client->roomname());
							if (it != rooms.end())
							{	
								int index = std::distance(rooms.begin(), it);
								std::string mapIndex = rooms[index];
								std::vector<SOCKET> temp = roomsandclients[mapIndex];
								temp.push_back(acceptSocket);
								roomsandclients[mapIndex] = temp;
								clientsocketinfo.insert(std::pair<std::string, SOCKET>(client->name(), acceptSocket));
								broadcast(rooms[index], (char*)buf,iResult);
							}
							else
							{
								rooms.push_back(client->roomname());
								std::vector<SOCKET> temp;
								temp.push_back(acceptSocket);
								roomsandclients.insert(std::pair<std::string, std::vector<SOCKET>>(client->roomname(), temp));
								clientsocketinfo.insert(std::pair<std::string, SOCKET>(client->name(), acceptSocket));
								broadcast(client->roomname(), (char*)buf,iResult);
							}
					}	
				}
				else if (client->type() == chat::Client_MessageType_LEAVE_ROOM)
				{
					std::vector<std::string>::iterator it = std::find(rooms.begin(), rooms.end(), client->roomname());
					if (it != rooms.end())
					{
						int index = std::distance(rooms.begin(), it);
						std::string mapIndex = rooms[index];

						std::vector<SOCKET> temp = roomsandclients[mapIndex];
						//printf(" size before erasing %d", temp.size());
						SOCKET temp1 = clientsocketinfo[client->name()];
						std::cout << "removing socket :" << temp1 << std::endl;
						std::vector<SOCKET>::iterator it = std::find(temp.begin(), temp.end(), temp1);
						index = std::distance(temp.begin(), it);
						temp.erase(temp.begin() + index);
						roomsandclients[mapIndex] = temp;
						//printf(" size after erasing %d", temp.size());
						broadcast(client->roomname(), (char*)buf,iResult);
					}
				}
				else if(client->type() == chat::Client_MessageType_SEND_MESSAGE)
				{
					broadcast(client->roomname(), (char*)buf,iResult);
				}
			}
			
		}
		else if (iResult < 0)
		{
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(acceptSocket);
			WSACleanup();
			return 1;
		}
		else // iResult == 0
		{
			printf("Connection closing...\n");
		}
	} while (iResult > 0);

	// #6 close
	iResult = shutdown(acceptSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(acceptSocket);
		WSACleanup();
		return 1;
	}

	// cleanup
	closesocket(acceptSocket);
	WSACleanup();
}

void broadcast(std::string room,const char *buf,size_t size)
{
	
	

		std::vector<SOCKET> temp = roomsandclients[room];
		for (SOCKET acpskt : temp)
		{
			//msgstring = mybuffer->Serializestring(msgstring);
			int iResult = send(acpskt, buf, size, 0);
			if (iResult < 0)
				printf("error while connecting to client on socket %d", acpskt);
		}
}