#include<cppconn/driver.h>
#include<cppconn/exception.h>
#include<cppconn/resultset.h>
#include<cppconn/statement.h>
#include<cppconn/prepared_statement.h>
#define WIN32_LEAN_AND_MEAN	
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include<string>
#pragma comment (lib, "Ws2_32.lib")
#include"authentication.pb.h"
#include"sha256.h"
#include <iostream>


#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "5155"

std::string generateRandomSalt();
sql::Driver* driver;
sql::Connection* con;
sql::ResultSet* rs;
sql::PreparedStatement* pstmt;
SHA256 sha256;

int main(int argc, char** argv)
{

	srand(time(NULL));
	/*std::string salt = "salt";
	std::string
		query = "insert into web_auth(email,salt,hashed_password) values('" + salt + "','" + salt + "','" + salt + "');";
	
	std::cout << query << std::endl;*/

	std::string server = "127.0.0.1:3306";
	std::cout << "Enter database admin username : ";
	std::string username;
	std::getline(std::cin, username);
	std::cout << "Enter database admin password : ";
	std::string adminPassword;
	std::getline(std::cin, adminPassword);


	//std::string username = "root";
	//std::string password = "root";

	try
	{
		driver = get_driver_instance();
		con = driver->connect(server, username, adminPassword);
		printf("Successfully connected to our database!\n");

		pstmt = con->prepareStatement("CREATE SCHEMA IF NOT EXISTS Testing");
		int res = pstmt->executeUpdate();
		std::cout << "schema created" << std::endl;

		//TODO: set the schema
		con->setSchema("Testing");

		pstmt = con->prepareStatement("CREATE TABLE IF NOT EXISTS `web_auth` (`id` INT NOT NULL AUTO_INCREMENT, `email` VARCHAR(255), `salt` CHAR(64), `hashed_password` CHAR(64),PRIMARY KEY(`id`));");
		res = pstmt->executeUpdate();

		pstmt = con->prepareStatement("CREATE TABLE IF NOT EXISTS `user` (`userid` INT NOT NULL,`last_login` TIMESTAMP ,creation_date DATETIME DEFAULT CURRENT_TIMESTAMP, FOREIGN KEY (userid) references web_auth(id));");
		res = pstmt->executeUpdate();

		//pstmt = con->prepareStatement("INSERT INTO user (userid) VALUES(1);");
		//res = pstmt->executeUpdate();
		
	}

	catch (sql::SQLException& exception)
	{
		std::cout << "#ERR: SQLException in" << __FILE__;
		std::cout << "(" << __FUNCTION__ << ") on line" << __LINE__ << std::endl;
		std::cout << "# ERR: " << exception.what();
		std::cout << " (MySQL error code: " << exception.getErrorCode();
		std::cout << ", SQLState: " << exception.getSQLState() << ")" << std::endl;
		return 1;
	}

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

	iResult = 0;
	// #5 recv & send	(Blocking calls)
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;
	void* buf;
	int iSendResult;
	
	while (true)
	{
		iResult = recv(acceptSocket, recvbuf, recvbuflen, 0);
		buf = malloc(iResult);
		buf = (void*)recvbuf;
		server::createAccountWeb* account = new server::createAccountWeb();
		server::AuthenticateWeb* auth = new server::AuthenticateWeb();
		account->ParseFromArray(buf,iResult);
		std::cout << "email: "<<account->email() << std::endl;

		if (account->requestid() == 0)
		{
			std::string buffer;
			std::string query = "select * from web_auth where email = '" + account->email() + "';";

			pstmt = con->prepareStatement(query);
			rs = pstmt->executeQuery();
			if (rs->rowsCount() == 0)
			{
				std::cout << account->email() << std::endl;
				std::string salt = generateRandomSalt();
				std::string hashedPassword = sha256(salt + account->plainpassword());
				std::string query = "insert into web_auth(email,salt,hashed_password) values('" + account->email() + "','" + salt + "','" + hashedPassword + "');";
				pstmt = con->prepareStatement(query);
				int res = pstmt->executeUpdate();
				query = "select id from web_auth where email = '" + account->email() + "';";
				pstmt = con->prepareStatement(query);
				rs = pstmt->executeQuery();
				rs->next();
				int user_id = rs->getInt(1);
				query = "insert into user(userid) values(" +std::to_string(user_id)+ ");";
				pstmt = con->prepareStatement(query);
				res = pstmt->executeUpdate();
				account->set_type(server::createAccountWeb_reasonType_ACCOUNT_SUCCESS);
			}
			else
			{
				account->set_type(server::createAccountWeb_reasonType_ACCOUNT_ALREADY_EXISTS);
			}
			size_t size = account->ByteSizeLong();
			buf = malloc(size);
			account->SerializeToArray(buf, size);
			//buffer = account->SerializeAsString();
			iResult = send(acceptSocket, (char*)buf,size, 0);
		}
		else if (account->requestid() == 1)
		{
			auth->ParseFromArray(buf,iResult);
			std::string buffer;
			std::string query = "select * from web_auth where email = '" + auth->email() + "';";
			pstmt = con->prepareStatement(query);
			rs = pstmt->executeQuery();
			if (rs->rowsCount() == 0)
			{
				auth->set_type(server::AuthenticateWeb_reasonType_INVALID_CREDENTIALS);
			}
			else
			{
				query = "select salt,hashed_password from web_auth where email = '" + auth->email() + "';";
				//query = "select * from web_auth;";
				pstmt = con->prepareStatement(query);
				rs = pstmt->executeQuery();
				rs->next();
				std::string salt=rs->getString(1);
				std::string actualHashedPassword = rs->getString(2);
				std::string plainPassword = auth->plainpassword();
				//std::cout << "inside authentication\nsalt: " << salt << "\nplain password: " << plainPassword << std::endl;
				std::string hashedPassword = sha256(salt + plainPassword);
				if (hashedPassword == actualHashedPassword)
				{
					query = "select id from web_auth where email = '" + account->email() + "';";
					pstmt = con->prepareStatement(query);
					rs = pstmt->executeQuery();
					rs->next();
					int user_id = rs->getInt(1);
					query = "update user set last_login = current_timestamp where userid = " + std::to_string(user_id)+";";
					pstmt = con->prepareStatement(query);
					//rs = pstmt->executeQuery();
					int res = pstmt->executeUpdate();
					auth->set_type(server::AuthenticateWeb_reasonType_AUTHENTICATION_SUCCESS);
				}
				else
				{
					auth->set_type(server::AuthenticateWeb_reasonType_INVALID_CREDENTIALS);
				}
			}
			size_t size = auth->ByteSizeLong();
			buf = malloc(size);
			auth->SerializeToArray(buf, size);
			//buffer = auth->SerializeAsString();
			iResult = send(acceptSocket, (char*)buf, size, 0);
		}
		//std::cout << recvbuf << std::endl;
	}


	con->close();

}

std::string generateRandomSalt()
{
	std::string characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	std::string temp;
	int size = 1 + rand() % 10;
	for (int index = 0; index < size; index++)
	{
		temp += characters.at(rand() % characters.length());
	}

	return temp;
}