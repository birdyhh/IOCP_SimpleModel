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
	//��ȡ��������������Ӧ����CP�Ĵ�����ͬ�������߳�
	int nThreadCount = GetCpuCoreCount() * 2;
	std::cout << "nThreadCount = " << nThreadCount << "." << std::endl;

	//��ʼ��server�׽��ֺ͵�ַ
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

	//�󶨶˿ڲ�����
	nRet = bind(hSocket, (sockaddr*)&server, sizeof(server));
	std::cout << "bind() nRet = " << nRet << "." << std::endl;
	nRet = listen(hSocket, nThreadCount);
	std::cout << "listen() nRet = " << nRet << "." << std::endl;

	//����CP
	g_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, nThreadCount);
	std::cout << "CreateIOCP() = " << g_hIOCP << "." << std::endl;

	//�����̣߳����󶨵�WorkerThread����
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
			//�̵߳��������ں��߳̾�����������ڲ�һ���������̺߳�رվ����������δ���߳̽���ʱ��ʱ�ͷ��ڴ���Դ
		}
	}


}