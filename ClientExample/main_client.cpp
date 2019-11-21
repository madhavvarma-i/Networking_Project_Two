#define WIN32_LEAN_AND_MEAN			// Strip rarely used calls

#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include<iostream>
#include<thread>
#include <vector>
#include<string.h>
#include <chrono>
#include<string>
#include"authentication.pb.h"
#include"protocol.pb.h"
#include<google/protobuf/io/zero_copy_stream.h>
#include<google/protobuf/io/zero_copy_stream_impl.h>
#include<google/protobuf/io/coded_stream.h>
//#include<buffer.h>

//std::getline(std::cin , line);

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "5150"

int receive(SOCKET connectSocket);
std::string myname;
//bufferClass* mybuffer = new bufferClass(20);
std::string buffer;
bool _isAut = false;

int main(int argc, char** argv)
{
	int rooms_joined = 0;
	WSADATA wsaData;
	int iResult;
	bool member;
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

	// #1 socket
	SOCKET connectSocket = INVALID_SOCKET;

	struct addrinfo* result = NULL;
	struct addrinfo* ptr = NULL;
	struct addrinfo hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// resolve the server address and port
	iResult = getaddrinfo("127.0.0.1", DEFAULT_PORT, &hints, &result);
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

	// #2 connect
	// Attempt to connect to the server until a socket succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
	{
		// Create a SOCKET for connecting to the server
		connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (connectSocket == INVALID_SOCKET)
		{
			printf("socket() failed with error code %d\n", WSAGetLastError());
			freeaddrinfo(result);
			WSACleanup();
			return 1;
		}

		// Attempt to connect to the server
		iResult = connect(connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR)
		{
			printf("connect() failed with error code %d\n", WSAGetLastError());
			closesocket(connectSocket);
			connectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}
	freeaddrinfo(result);

	if (connectSocket == INVALID_SOCKET)
	{
		printf("Unable to connect to the server!\n");
		WSACleanup();
		return 1;
	}
	printf("Successfully connected to the server on socket %d!\n", (int)connectSocket);
	std::thread t1(receive, connectSocket);
	t1.detach();

	std::vector<std::string> myrooms;

	server::AuthenticateWeb* accnt = new server::AuthenticateWeb();
	server::createAccountWeb* account = new server::createAccountWeb();

	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;
	int siz = account->ByteSizeLong() + 4;
	char* pkt = new char[siz];
	google::protobuf::io::ArrayOutputStream aos(pkt, siz);
	//CodedOutputStream* coded_output = new CodedOutputStream(&aos);
	//coded_output->WriteVarint32(account->ByteSizeLong());
	//account->SerializeToCodedStream(coded_output);

	printf("please select an option");
	while (true)
	{
		
		//std::cin.ignore(INT_MAX);
		std::string email;
		std::string password;
		size_t size;
		void* buf;
		std::vector<std::string>::iterator it;
		
		if (!_isAut)
		{
			printf("\n1.create user\n2.authenticate user\n");
			int myc = 0;
			std::cin >> myc;
			std::cin.ignore();
			switch (myc)
			{
			case 1:
			{
				printf("please enter email: ");
				//std::cin.ignore();

				//std::cin.ignore(1000,'\n');
				//getline(std::cin, email);
				std::cin >> email;
				printf("please enter password: ");
				//std::cin.ignore();
				//getline(std::cin, password);
				std::cin >> password;
				//std::cout << email << "," << password << std::endl;

				account->set_email(email);
				account->set_plainpassword(password);
				//std::cout << account->email() << std::endl;
				account->set_requestid(0);
				
				size_t size = account->ByteSizeLong();
				void* buf = malloc(size);
				account->SerializeToArray(buf, size);
				//account->SerializeToArray(&account,sizeof(account));
				account->Clear();
				iResult = send(connectSocket,(char *) buf, size, 0);
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				break;
			}
				
			case 2:
				printf("please enter email: ");
				//std::cin.ignore();

				//std::cin.ignore(1000,'\n');
				getline(std::cin, email);
				//std::cin >> roomname;
				printf("please enter password: ");
				//std::cin.ignore();

				getline(std::cin, password);


				accnt->set_email(email);
				accnt->set_plainpassword(password);
				accnt->set_requestid(1);
				
				size = accnt->ByteSizeLong();
				buf = malloc(size);
				accnt->SerializeToArray(buf, size);
				//account->SerializeToArray(&account,sizeof(account));
				accnt->Clear();
				iResult = send(connectSocket, (char*)buf, size, 0);
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				break;
			default:
				std::cout << "invalid choice" << std::endl;
				break;
			}

		}
		else
		{
			printf("please input your name: ");
			std::getline(std::cin, myname);
			// #3 write & read
			while (true)
			{
				printf("1.join room\n2.leave room\n3.send message to the room\n");
				int mychoice = 0;
				std::cin >> mychoice;
				std::cin.ignore();
				//std::cin.clear();
				//std::cin.ignore(INT_MAX);
				std::string roomname;
				std::string msg;
				chat::Client* client = new chat::Client();

				std::vector<std::string>::iterator it;
				switch (mychoice)
				{
				case 1:
					member = false;
					printf("please enter room name: ");
					//std::cin.ignore();
						
					//std::cin.ignore(1000,'\n');
					getline(std::cin, roomname);
					for (std::string temp : myrooms)
					{
						if (temp == roomname)
						{
							member = true;
							break;
						}
					}
					if (!member)
					{
						//std::cin >> roomname;
						client->set_name(myname);
						client->set_roomname(roomname);
						client->set_type(chat::Client_MessageType_JOIN_ROOM);
						client->set_requestid(2);
						//msg = client->SerializeAsString();
						size = client->ByteSizeLong();
						buf = malloc(size);
						client->SerializeToArray(buf, size);
						//account->SerializeToArray(&account,sizeof(account));
						client->Clear();
						iResult = send(connectSocket, (char*)buf, size, 0);

						//msg = mybuffer->Serializestring(msg);
						//iResult = send(connectSocket, msg.c_str(), sizeof(msg), 0);
						client->Clear();
						if (iResult > 0)
						{
							myrooms.push_back(roomname);
							printf("joined sucessfully\n");
							client->Clear();
						}
					}
					else
					{
						std::cout << "You are already a member of " << roomname << std::endl;
					}
					break;
				case 2:
					printf("please enter room name: ");
					//std::cin.ignore(1000, '\n');
					//std::cin >> roomname;
					//std::cin.ignore();
					std::getline(std::cin, roomname);//std::cin >> roomname;
					it = std::find(myrooms.begin(), myrooms.end(), roomname);
					if (it != myrooms.end())
					{
						client->set_name(myname);
						client->set_roomname(roomname);
						client->set_type(chat::Client_MessageType_LEAVE_ROOM);
						client->set_requestid(2);
						size = client->ByteSizeLong();
						buf = malloc(size);
						client->SerializeToArray(buf, size);
						//account->SerializeToArray(&account,sizeof(account));
						client->Clear();
						iResult = send(connectSocket, (char*)buf, size, 0);
						//msg = client->SerializePartialAsString();
						//iResult = send(connectSocket, msg.c_str(), sizeof(msg), 0);
						//client->Clear();
						int index = std::distance(myrooms.begin(), it);
						myrooms.erase(myrooms.begin() + index);
						std::cout << "Removed sucessfully" << std::endl;
					}
					else
					{
						std::cout << "you are not a member of " + roomname << std::endl;
					}
					break;
				case 3:
					std::cout << "please input room name: " << std::endl;
					//std::cin.ignore();
					std::getline(std::cin, roomname);//std::cin >> ;
					//std::cin >> roomname;
					it = std::find(myrooms.begin(), myrooms.end(), roomname);
					if (it != myrooms.end())
					{
						std::cout << "please input message: " << std::endl;
						/*std::cin.ignore(1000, '\n');
						std::getline(std::cin, msg);*/
						//std::cin >> msg;
						std::getline(std::cin, msg);
						client->set_message(msg);
						client->set_name(myname);
						client->set_roomname(roomname);
						client->set_type(chat::Client_MessageType_SEND_MESSAGE);
						client->set_requestid(2);
						size = client->ByteSizeLong();
						buf = malloc(size);
						client->SerializeToArray(buf, size);
						//account->SerializeToArray(&account,sizeof(account));
						client->Clear();
						iResult = send(connectSocket, (char*)buf, size, 0);
						//msg = client->SerializeAsString();
						//msg = mybuffer->Serializestring(msg);
						//iResult = send(connectSocket, msg.c_str(), sizeof(msg), 0);
						client->Clear();
						std::cout << "sent sucessfully" << std::endl;
					}
					else
					{
						std::cout << "you need to join the room in order to send the message to " + roomname << std::endl;
					}
					break;
				default:
					std::cout << "invalid choice" << std::endl;
					break;
				}
			}
		}
	}



	// #4 close
	closesocket(connectSocket);
	WSACleanup();

	return 0;
}

int receive(SOCKET connectSocket)
{
	int iResult = 0;
	void* buf;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;
	std::string msgstring, msgtype, msgowner, actlmsg, msgroom;
	while (true)
	{
		//printf("waiting to receive\n");
		
		iResult = recv(connectSocket, recvbuf, recvbuflen, 0);
		msgstring = recvbuf;
		chat::Client* client = new chat::Client();
		if (iResult > 0)
		{
			buf = malloc(iResult);
			buf = (void*)recvbuf;
			client->ParseFromArray(buf, iResult);
			if (client->requestid() == 0)
			{
				server::createAccountWeb* acc = new server::createAccountWeb();
				acc->ParseFromArray(buf,iResult);
				if (acc->type() == server::createAccountWeb_reasonType_ACCOUNT_SUCCESS)
				{
					printf("account successfully created\n");
				}
				else if (acc->type() == server::createAccountWeb_reasonType_ACCOUNT_ALREADY_EXISTS)
				{
					printf("account already exists");
				}
			}

			if (client->requestid() == 1)
			{
				server::AuthenticateWeb* Ac = new server::AuthenticateWeb();
				Ac->ParseFromArray(buf,iResult);
				if (Ac->type() == server::AuthenticateWeb_reasonType_AUTHENTICATION_SUCCESS)
				{
					printf("authentication is successful\n");
					_isAut = true;
				}
				else if (Ac->type() == server::AuthenticateWeb_reasonType_INVALID_CREDENTIALS)
				{
					printf("invalid credentials please try again\n");
					_isAut = false;
				}
				else
				{
					printf("internal server error");
					_isAut = false;
				}
			}
			else if (client->requestid() == 2)
			{
				if (client->name() != myname)
				{
					if (client->type() == chat::Client_MessageType_SEND_MESSAGE)
					{
						std::cout << "Client : " << client->name() << " | Room: " << client->roomname() << " | Message: " << client->message() << std::endl;
					}
					else if (client->type() == chat::Client_MessageType_JOIN_ROOM)
					{
						std::cout << "Client :" << client->name() << " joined the room :" <<client->roomname()<< std::endl;

					}
					else if (client->type() == chat::Client_MessageType_LEAVE_ROOM)
					{
						std::cout << "Client :" << client->name() << " left the room :"<<client->roomname() << std::endl;
					}
				}
			}
			else
			{

			}
		}
		else if (iResult == 0)
		{
			printf("Connection closed\n");
		}
		else
		{
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(connectSocket);
			WSACleanup();
			return 1;
		}
	}
}