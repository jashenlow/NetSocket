// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the NETSOCKETMANAGER_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// NETSOCKETMANAGER_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#pragma once
#ifndef _NET_SOCKET_MANAGER_H_
#define _NET_SOCKET_MANAGER_H_

#ifdef NETSOCKETMANAGER_EXPORTS
#define NETSOCKETMANAGER_API __declspec(dllexport)
#else
#define NETSOCKETMANAGER_API __declspec(dllimport)
#endif

#define WIN32_LEAN_AND_MEAN
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#include <map>
#include <string>

constexpr uint16_t BUFFER_DEFAULT_SIZE = 4096;
constexpr uint16_t TCP_MAX_CONNECTIONS = 100;

enum class SocketError : uint8_t
{
	OK,
	FAIL_WINSOCK,
	FAIL_SOCKET,
	FAIL_BIND,
	LIMIT_REACHED
};

enum class SocketBlockFlag : uint8_t
{
	BLOCKING,
	NON_BLOCKING
};



struct SocketStruct
{
	sockaddr_in LocalAddr;
	char* TxBuffer = nullptr;
	char* RxBuffer = nullptr;
	uint16_t TxBufferSize = 0;
	uint16_t RxBufferSize = 0;
	SocketBlockFlag BlockingType;
};

struct UDPSocketStruct : SocketStruct
{
	sockaddr_in TargetAddr;
	SOCKET Handle;
};

struct TCPSocketStruct : SocketStruct
{
	SOCKET LocalHandle;
	std::map<SOCKET, sockaddr_in> Peers;
	uint16_t PeerLimit;
};

class NETSOCKETMANAGER_API CNetSocketManager 
{
public:
	CNetSocketManager(void);
	~CNetSocketManager(void);

	SOCKET InitSocketUDP(const char* ip, const uint16_t& port, const SocketBlockFlag& blockFlag);
	bool InitSocketUDP(SOCKET socket, const char* ip, const uint16_t& port, const SocketBlockFlag& blockFlag);
	
	SOCKET InitTCPClient(const char* ip, const uint16_t& port, const SocketBlockFlag& blockFlag);
	bool InitTCPClient(SOCKET socket, const char* ip, const uint16_t& port, const SocketBlockFlag& blockFlag);

	SOCKET InitTCPServer(const char* ip, const uint16_t& port, const SocketBlockFlag& blockFlag);
	bool InitTCPServer(SOCKET socket, const char* ip, const uint16_t& port, const SocketBlockFlag& blockFlag);

	bool CloseSocket(SOCKET socket);
	bool CloseAllSockets();

	uint16_t GetRxBufferSize(SOCKET socket);
	void SetRxBufferSize(SOCKET socket, const uint16_t& newSize);
	
	uint16_t GetTxBufferSize(SOCKET socket);
	void SetTxBufferSize(SOCKET socket, const uint16_t& newSize);

	uint16_t& GetMaxTCPConnections(SOCKET socket);
	void SetMaxTCPConnections(SOCKET socket, const uint16_t& maxNum);

	bool ConnectToTCPServer(SOCKET socket, const char* serverIP, const uint16_t& serverPort);

	bool SendData(SOCKET socket, const IPPROTO& protocol, void* data, const uint16_t& dataSize, const char* targetIP, const uint16_t& targetPort);
	
	uint16_t CheckReceivedBytes(SOCKET socket, const IPPROTO& protocol);
	uint16_t ReceiveData(SOCKET socket, const IPPROTO& protocol, void* data, const uint16_t& dataSize);
	uint16_t ReceiveData(SOCKET socket, const IPPROTO& protocol, void* data, const uint16_t& dataSize, std::string& senderIP, uint16_t& senderPort);


protected:

private:
	std::map<SOCKET, UDPSocketStruct> lvUDPSocketMap;	//(Local Handle, Socket Data)
	std::map<SOCKET, TCPSocketStruct> lvTCPSocketMap;	//(Local Handle, Socket Data)

	inline bool ResizeBuffer(char* buffer, uint16_t& size, const uint16_t& newSize);
};
#endif //_NET_SOCKET_MANAGER_H_
