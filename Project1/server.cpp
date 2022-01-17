#include <winsock2.h>
#include <windows.h>
#include <string>
#include <iostream>

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"kernel32.lib")

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


}