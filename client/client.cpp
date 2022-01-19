#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <iostream>
#include <iomanip> 

#pragma comment(lib,"ws2_32.lib")

char g_buffer[1024];

int main()
{
	WSADATA wsaData;
	int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	std::cout << "WSAStartup() nRet = " << nRet << std::endl;
	if (nRet != NO_ERROR)
	{
		int nErr = WSAGetLastError();
		std::cout << "WSAStartup() ERROR! nErr = " << nErr << std::endl;
		return 1;
	}

	SOCKADDR_IN server;
	server.sin_family = AF_INET;
	server.sin_port = htons(6000);
	server.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	SOCKET hSocket = socket(PF_INET, SOCK_STREAM, 0);
	std::cout << "socket() hSocket = " << std::hex << hSocket << std::endl;
	nRet = connect(hSocket, (sockaddr*)&server, sizeof(server));
	std::cout << "connect() nRet = " << nRet << std::endl;
	if (nRet < 0)
	{
		int nErr = WSAGetLastError();
		std::cout << "connect() ERROR! nErr = " << nErr << std::endl;
		WSACleanup();
		return 2;
	}
	while (true)
	{
		std::cout << "senting string \"hello\" to server..." << std::endl;
		memset(g_buffer, NULL, sizeof(g_buffer));
		strcpy(g_buffer, "hello");
		nRet = send(hSocket, g_buffer, strlen(g_buffer) + 1, 0);
		std::cout << "send() nRet = " << nRet << std::endl;
		if (nRet == SOCKET_ERROR)
		{
			int nErr = WSAGetLastError();
			std::cout << "SOCKET_ERROR: " << std::dec << nErr << std::endl;
			break;
		}
		memset(g_buffer, NULL, sizeof(g_buffer));
		nRet = recv(hSocket, g_buffer, 1024, 0);
		std::cout << "recv() nRet = " << nRet << std::endl;
		if (nRet == SOCKET_ERROR)
		{
			int nErr = WSAGetLastError();
			std::cout << "SOCKET_ERROR: " << std::dec << nErr << std::endl;
			break;
		}
		std::cout << g_buffer << std::endl;
		Sleep(1000);
	}
	nRet = closesocket(hSocket);
	std::cout << "closesocket() nRet = " << nRet << std::endl;
	nRet = WSACleanup();
	std::cout << "WSACleanup() nRet = " << nRet << std::endl;
	system("pause");
	return 0;
}