#include <winsock2.h>
#include <windows.h>
#include <string>
#include <iostream>

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"kernel32.lib")

#define BUF_SIZE 1024

HANDLE g_hIOCP = 0;
char g_buffer[BUF_SIZE] = { 0 };

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


}