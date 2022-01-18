#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <windows.h>
#include <string>
#include <iostream>

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"kernel32.lib")

#define BUF_SIZE 1024

enum class IO_OPERATION
{
	IO_READ,IO_WRITE
};

struct IO_DATA
{
	IO_OPERATION opCode;
	OVERLAPPED OverLapped;
	SOCKET client;
	WSABUF wsaBuf;
	int nBytes;
};

HANDLE g_hIOCP = 0;
char g_buffer[BUF_SIZE] = { 0 };

DWORD WINAPI WorkerThread(LPVOID context)
{
	while (true)
	{
		DWORD dwIoSize = 0;
		void* lpComletionKey = NULL;
		LPOVERLAPPED lpOverlapped = NULL;
		BOOL bRet = GetQueuedCompletionStatus(g_hIOCP, &dwIoSize, (PULONG_PTR)&lpComletionKey, (LPOVERLAPPED*)&lpOverlapped, INFINITE);
		std::cout << "GetQueuedCompletionStatus() bRet= " << bRet;
		std::cout << ", dwIoSize = " << dwIoSize<< ", Key = " << lpComletionKey << std::endl;
		IO_DATA* lpIOContext = CONTAINING_RECORD(lpOverlapped, IO_DATA, OverLapped);
		if (dwIoSize == 0)
		{
			std::cout << "Client disconnected." << std::endl;
			int nRet = closesocket(lpIOContext->client);
			std::cout << "closesocket() nRet = " << nRet << "." << std::endl;
			delete lpIOContext;
			continue;
		}
		std::cout << "dwThreadID" << std::dec << GetCurrentThreadId() << std::endl;
		std::cout << "opCode = " << std::hex << (int)lpIOContext->opCode << std::endl;
		if (lpIOContext->opCode == IO_OPERATION::IO_READ)
		{
			std::cout << "Client IO_READ" << std::endl;
			ZeroMemory(&lpIOContext->OverLapped, sizeof(lpIOContext->OverLapped));
			lpIOContext->wsaBuf.buf = g_buffer;
			lpIOContext->wsaBuf.len = (ULONG)strlen(g_buffer) + 1;
			lpIOContext->opCode = IO_OPERATION::IO_WRITE;
			lpIOContext->nBytes = (int)strlen(g_buffer) + 1;
			DWORD dwFlags = 0;
			DWORD nBytes = (DWORD)strlen(g_buffer) + 1;
			int nRet = WSASend(lpIOContext->client, &lpIOContext->wsaBuf, 1, &nBytes, dwFlags, &(lpIOContext->OverLapped), NULL);
			if (nRet == SOCKET_ERROR)
			{
				int nErr = WSAGetLastError();
				if (ERROR_IO_PENDING == nErr)
				{
					std::cout << "WSASend Failed! nErr = " << nErr << "." << std::endl;
					nRet = closesocket(lpIOContext->client);
					std::cout << "closesocket() nRet = " << nRet << std::endl;
					delete lpIOContext;
					continue;
				}
			}
			memset(g_buffer, NULL, sizeof(g_buffer));
		}
		else if (lpIOContext->opCode == IO_OPERATION::IO_WRITE)
		{
			std::cout << "Client IO_WRITE" << std::endl;
			DWORD dwFlags = 0;
			DWORD nBytes = sizeof(g_buffer);
			lpIOContext->opCode = IO_OPERATION::IO_READ;
			lpIOContext->wsaBuf.buf = g_buffer;
			lpIOContext->wsaBuf.len = nBytes;
			lpIOContext->nBytes = nBytes;
			ZeroMemory(&lpIOContext->OverLapped, sizeof(lpIOContext->OverLapped));
			int nRet = WSARecv(lpIOContext->client, &lpIOContext->wsaBuf, 1, &nBytes, &dwFlags, &(lpIOContext->OverLapped), NULL);
			if (nRet == SOCKET_ERROR)
			{
				int nErr = WSAGetLastError();
				if (ERROR_IO_PENDING == nErr)
				{
					std::cout << "WSARecv Failed! nErr = " << nErr << "." << std::endl;
					nRet = closesocket(lpIOContext->client);
					std::cout << "closesocket() nRet = " << nRet << std::endl;
					delete lpIOContext;
					continue;
				}
			}
			std::cout << lpIOContext->wsaBuf.buf << std::endl;
		}
	}
	std::cout << "WordkerThread() end." << std::endl;
	return 0;
}

int GetCpuCoreCount()
{
	SYSTEM_INFO sysInfo = { 0 };
	GetSystemInfo(&sysInfo);
	return sysInfo.dwNumberOfProcessors;
}

int main()
{
	//获取核心数量，后续应用在CP的创建相同数量的线程
	int nThreadCount = GetCpuCoreCount() * 2;
	std::cout << "nThreadCount = " << nThreadCount << "." << std::endl;

	//初始化server套接字和地址
	WSADATA wsaData;
	int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	std::cout << "WSAStartup() nRet = " << nRet << "." << std::endl;
	if (nRet != NO_ERROR)
	{
		int nErr = WSAGetLastError();
		return 1;
	}
	SOCKET hSocket = WSASocket(PF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	std::cout << "WSASocket() hSocket = " << std::hex << hSocket << "." << std::endl;

	SOCKADDR_IN server;
	server.sin_family = AF_INET;
	server.sin_port = htons(6000);
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	//绑定端口并监听
	nRet = bind(hSocket, (sockaddr*)&server, sizeof(server));
	std::cout << "bind() nRet = " << nRet << "." << std::endl;
	nRet = listen(hSocket, nThreadCount);
	std::cout << "listen() nRet = " << nRet << "." << std::endl;

	//创建CP
	g_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, nThreadCount);
	std::cout << "CreateIOCP() = " << g_hIOCP << "." << std::endl;

	//创建线程，并绑定到WorkerThread函数
	for (int i = 0; i < nThreadCount; i++)
	{
		HANDLE hThread = 0;
		DWORD dwThreadID = 0;
		hThread = CreateThread(NULL, 0, WorkerThread, 0, 0, &dwThreadID);
		std::cout << "CreateThread() hThread = " << std::hex << hThread;
		std::cout << ", dwThreadID = " << std::dec << dwThreadID << std::endl;
		if (hThread)
		{
			CloseHandle(hThread);
			//线程的生命周期和线程句柄的生命周期不一样，创建线程后关闭句柄，可以在未来线程结束时及时释放内存资源
		}
	}
	
	while (hSocket)
	{
		SOCKET hClient = accept(hSocket, NULL, NULL);
		std::cout << "accept() hClient = " << std::hex << hClient << "." << std::endl;
		HANDLE hIocpTemp = CreateIoCompletionPort((HANDLE)hClient, g_hIOCP, hClient, 0);
		std::cout << "ConnectIOCP() hIocpTemp = " << std::hex << hIocpTemp << "." << std::endl;
		if (hIocpTemp == NULL)
		{
			int nErr = WSAGetLastError();
			std::cout << "ConnectIOCP() hIocpTemp = " << std::hex << hIocpTemp << "." << std::endl;
			nRet = closesocket(hClient);
			std::cout << "closesocket() nRet = " << nRet << "." << std::endl;
			break;
		}
		else
		{
			IO_DATA* data = new IO_DATA;
			std::cout << "new data =  " << data << "." << std::endl;
			memset(data, 0, sizeof(IO_DATA));
			data->nBytes = 0;
			data->opCode = IO_OPERATION::IO_READ;
			memset(g_buffer, NULL, BUF_SIZE);
			data->wsaBuf.buf = g_buffer;
			data->wsaBuf.len = BUF_SIZE;
			data->client = hClient;
			DWORD nBytes = BUF_SIZE, dwFlags = 0;
			int nRet = WSARecv(hClient, &(data->wsaBuf), 1, &nBytes, &dwFlags, &(data->OverLapped), NULL);
			if (nRet == SOCKET_ERROR)
			{
				int nErr = WSAGetLastError();
				if (ERROR_IO_PENDING != nErr)
				{
					std::cout << "WSARecv FAILED! nErr = " << nErr << "." << std::endl;
					nRet = closesocket(hClient);
					std::cout << "closesocket() nRet = " << nRet << "." << std::endl;
					delete data;
				}
			}
		}
	}
	nRet = closesocket(hSocket);
	std::cout << "closesocket() nRet = " << nRet << "." << std::endl;
	nRet = WSACleanup();
	std::cout << "WSACleanup() nRet = " << nRet << "." << std::endl;
	system("pause");
	return 0;
}