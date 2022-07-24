//Copyright(c) 2022 Jashen Low
//
//Permission is hereby granted, free of charge, to any person obtaining
//a copy of this softwareand associated documentation files(the
//	"Software"), to deal in the Software without restriction, including
//	without limitation the rights to use, copy, modify, merge, publish,
//	distribute, sublicense, and /or sell copies of the Software, and to
//	permit persons to whom the Software is furnished to do so, subject to
//	the following conditions :
//
//The above copyright noticeand this permission notice shall be
//included in all copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
//LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
//OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
//WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once
#ifndef _NETSOCKET_H_
#define _NETSOCKET_H_

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(__NT__)
#define SOCKET_PLATFORM_WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#define GetSocketError (WSAGetLastError())
#elif defined(__APPLE__) || defined(__linux__) || defined(__unix__)
#define SOCKET_PLATFORM_UNIX

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define GetSocketError (errno)
#endif

#include <string>
#include <vector>

constexpr uint16_t	BUFFER_DEFAULT_SIZE	= 4096;
constexpr uint16_t	MAX_UDP_BUFFER_SIZE	= 65507;
constexpr uint32_t	MAX_TCP_BUFFER_SIZE	= 1048576;

#ifdef SOCKET_PLATFORM_WINDOWS
enum class WinsockError
{
	OK,
	FAIL_WINSOCK,
	FAIL_SOCKET,
	FAIL_BIND,
	LIMIT_REACHED
};
#endif

enum class SocketType
{
	UDP,
	TCP_CLIENT,
	TCP_SERVER
};

class NetSocket
{
public:
	struct SocketInfo
	{
		size_t		SocketFD;
		sockaddr_in Addr;
		std::string IPAddress;
		uint16_t	Port;

		SocketInfo()
		{
			Reset();
		}

		void Reset()
		{
			SocketFD = 0;

			Addr.sin_family = AF_INET;
			memset(&Addr.sin_addr, 0, sizeof(Addr.sin_addr));
			Addr.sin_port = 0;
			memset(Addr.sin_zero, '\0', sizeof(Addr.sin_zero));

			IPAddress.clear();
			Port = 0;
		}
	};

	struct LocalSocketInfo : SocketInfo
	{
		std::vector<SocketInfo> RemoteSocketInfo;	//This is used to store UDP sender info, TCP connected clients, and connected TCP server info.
		SocketType	Type = SocketType::UDP;
		IPPROTO		Protocol = IPPROTO::IPPROTO_UDP;
	};

	//Socket Constructor
	NetSocket()
	{
		rxBuffer.resize(MAX_UDP_BUFFER_SIZE);
		txBuffer.resize(BUFFER_DEFAULT_SIZE);

		udpTargetAddr.sin_family = AF_UNSPEC;	//To indicate that this is not configured.
		memset(&udpTargetAddr.sin_addr, 0, sizeof(udpTargetAddr.sin_addr));
		udpTargetAddr.sin_port = 0;
		memset(udpTargetAddr.sin_zero, '\0', sizeof(udpTargetAddr.sin_zero));
	}

	//Socket Destructor
	~NetSocket()
	{
		CloseSocket();
	}

	//Get all info about this socket
	const LocalSocketInfo& GetThisSocketInfo() const
	{
		return thisSocketInfo;
	}

	/*
	Returns a value which determines if the specified socket is using IPv4 or IPv6.
	Return values: AF_INET or AF_INET6.
	*/
	const uint16_t& GetIPVersion(const SocketInfo& socket) const
	{
		return thisSocketInfo.Addr.sin_family;
	}

	//Get transmit buffer size
	const uint32_t& GetTxBufferSize() const
	{
		return (uint32_t)txBuffer.size();
	}

	//Set transmit buffer size
	void SetTxBufferSize(const uint32_t& newSize)
	{
		if (newSize != txBuffer.size())
			txBuffer.resize((newSize > maxBufferSize) ? maxBufferSize : newSize);
	}

#ifdef SOCKET_PLATFORM_WINDOWS
	//Initialize socket
	WinsockError Init(const char* localIP, const uint16_t& localPort, const SocketType& type, const bool& isNonBlocking)
	{
		maxBufferSize			= (type == SocketType::UDP) ? MAX_UDP_BUFFER_SIZE : MAX_TCP_BUFFER_SIZE;
		unsigned long blocking	= (unsigned long)isNonBlocking;
		WinsockError errorcode	= WinsockError::OK;

		//Set local socket details
		thisSocketInfo.IPAddress	= localIP;
		thisSocketInfo.Port			= localPort;
		thisSocketInfo.Type			= type;
		thisSocketInfo.Protocol		= (type == SocketType::UDP) ? IPPROTO::IPPROTO_UDP : IPPROTO::IPPROTO_TCP;

		thisSocketInfo.Addr.sin_family = AF_INET;
		inet_pton(AF_INET, localIP, &thisSocketInfo.Addr.sin_addr);
		thisSocketInfo.Addr.sin_port = htons(localPort);

		//Initialize Winsock
		WSADATA wsData;
		if (WSAStartup(MAKEWORD(2, 2), &wsData) != 0)
			errorcode = WinsockError::FAIL_WINSOCK;
		else
		{
			//Create socket
			thisSocketInfo.SocketFD = socket(AF_INET, (thisSocketInfo.Protocol == IPPROTO::IPPROTO_TCP) ? SOCK_STREAM : SOCK_DGRAM, thisSocketInfo.Protocol);
			
			if (thisSocketInfo.SocketFD == INVALID_SOCKET)
				errorcode = WinsockError::FAIL_SOCKET;
			else
			{
				//Bind socket to localIP and localPort
				if (bind(thisSocketInfo.SocketFD, (sockaddr*)&thisSocketInfo.Addr, sizeof(sockaddr_in)) == SOCKET_ERROR)
					errorcode = WinsockError::FAIL_BIND;
				else
				{
					//Set socket blocking flag
					ioctlsocket(thisSocketInfo.SocketFD, FIONBIO, &blocking);

					if (rxBuffer.empty())
						SetRxBufferSize(maxBufferSize);
					if (txBuffer.empty())
						SetTxBufferSize(BUFFER_DEFAULT_SIZE);

					thisSocketInfo.RemoteSocketInfo.clear();

					if ((thisSocketInfo.Type == SocketType::UDP) || (thisSocketInfo.Type == SocketType::TCP_CLIENT))
						thisSocketInfo.RemoteSocketInfo.resize(1);					
				}
			}
		}

		if (errorcode != WinsockError::OK)
			CloseSocket();

		return errorcode;
	}

	//Close the socket
	bool CloseSocket() const
	{
		if (closesocket(thisSocketInfo.SocketFD) != SOCKET_ERROR)
			return (WSACleanup() == 0);
		else
			return false;
	}

	/*
	Attempts to connect to a TCP server if this socket is a TCP client.
	Returns TRUE if connection is successful, FALSE if connection is unsuccessful or if this socket is a UDP socket.
	*/
	bool ConnectServerTCP(const char* serverIP, const uint16_t& serverPort)
	{
		if (thisSocketInfo.Type == SocketType::TCP_CLIENT)
		{
			SocketInfo& serverInfo = thisSocketInfo.RemoteSocketInfo.front();
			
			serverInfo.Addr.sin_family = AF_INET;
			inet_pton(AF_INET, serverIP, &serverInfo.Addr.sin_addr);
			serverInfo.Addr.sin_port = htons(serverPort);

			return (connect(thisSocketInfo.SocketFD, (sockaddr*)&serverInfo.Addr, sizeof(sockaddr_in)) != SOCKET_ERROR);
		}
		else
			return false;
	}

	//Check number of bytes waiting in the input buffer
	inline const int& CheckReceivedBytes()
	{
		ioctlsocket(thisSocketInfo.SocketFD, FIONREAD, (u_long*)&recvBytes);
		return recvBytes;
	}
	
	/*
	Call this to send any data type through the network.
	NOTE: For string types, ensure that dataSize = stringlength + 1.
	NOTE: For Unicode strings, ensure that dataSize = (stringlength + 1) * charsize.
	Arguments:
	  - data:       Pointer to the data to be sent.
	  - dataSize:   Size of the data to send.
	  - tartgetIP:  IP address of the receiver.
	  - targetPort: Port number of the receiver.
	*/
	template<typename T>
	bool SendDataUDP(T* data, const size_t& dataSize, const char* targetIP, const uint16_t& targetPort)
	{
		if ((data != nullptr) && (thisSocketInfo.Protocol == IPPROTO::IPPROTO_UDP))
		{
			char* dataPtr = nullptr;

			udpTargetAddr.sin_family = AF_INET;
			inet_pton(AF_INET, targetIP, &udpTargetAddr.sin_addr);
			udpTargetAddr.sin_port = htons(targetPort);

			if (IsUnicodeString(data))
			{
				if (dataSize > txBuffer.size())
					txBuffer.resize(dataSize + sizeof(T));

				memset(txBuffer.data(), '\0', txBuffer.size());
				memcpy(txBuffer.data(), data, dataSize);

				dataPtr = (char*)txBuffer.data();
			}
			else
				dataPtr = (char*)data;
			
			return (sendto(thisSocketInfo.SocketFD, dataPtr, (int)dataSize, 0, (sockaddr*)&udpTargetAddr, sizeof(sockaddr)) != SOCKET_ERROR);
		}
		else
			return false;
	}

	/*
	Call this function if the expected data has a fixed size(like structs), or if you simply want to copy the data into a buffer.
	Returns the number of bytes received.
	Arguments:
	  - data:       Pointer to where you would like to store the received data.
	  - dataSize:   The expected data size to be received. Set to SIZE_MAX if the expected data size is unknown.
	                If set to SIZE_MAX, ensure that the "data" argument points to a buffer large enough to avoid potential overruns.
	  - senderIP:   The IP address of the sender.
	  - senderPort: The sender's port number.
	*/
	int ReceiveDataUDP(void* data, const size_t& dataSize, std::string& senderIP, uint16_t& senderPort)
	{
		if ((data != nullptr) && (thisSocketInfo.Protocol == IPPROTO::IPPROTO_UDP))
		{
			int udpSenderLen		= sizeof(sockaddr);
			SocketInfo& senderInfo	= thisSocketInfo.RemoteSocketInfo.front();
			
			memset(rxBuffer.data(), '\0', rxBuffer.size());
			recvBytes = recvfrom(thisSocketInfo.SocketFD, (char*)rxBuffer.data(), (int)rxBuffer.size(), 0, (sockaddr*)&senderInfo.Addr, &udpSenderLen);
			
			GetIPFromSockAddr(senderInfo);
			senderIP = senderInfo.IPAddress;
			GetPortFromSockAddr(senderInfo);
			senderPort = senderInfo.Port;

			if ((recvBytes > 0) && (recvBytes != SOCKET_ERROR))
			{
				//Check if all received data should be copied into the "data" argument without checking.
				if (dataSize == SIZE_MAX)
					memcpy(data, rxBuffer.data(), recvBytes);
				else
				{
					//Check if received data size is the expected size.
					if (recvBytes == dataSize)
						memcpy(data, rxBuffer.data(), recvBytes);
				}

				return recvBytes;
			}
			else
				return 0;
		}
		else
			return 0;
	}

	/*
	Call this function if the expected data has a fixed size(like structs), or if you simply want to copy the data into a buffer.
	Returns the number of bytes received.
	Arguments:
	  - data:     Pointer to where you would like to store the received data.
	  - dataSize: The expected data size to be received. Set to SIZE_MAX if the expected data size is unknown.
			      If set to SIZE_MAX, ensure that the "data" argument points to a buffer large enough to avoid potential overruns.
	*/
	int ReceiveDataUDP(void* data, const size_t& dataSize)
	{
		if ((data != nullptr) && (thisSocketInfo.Protocol == IPPROTO::IPPROTO_UDP))
		{
			int udpSenderLen = sizeof(sockaddr);
			SocketInfo& senderInfo = thisSocketInfo.RemoteSocketInfo.front();

			memset(rxBuffer.data(), '\0', rxBuffer.size());
			recvBytes = recvfrom(thisSocketInfo.SocketFD, (char*)rxBuffer.data(), (int)rxBuffer.size(), 0, (sockaddr*)&senderInfo.Addr, &udpSenderLen);

			if ((recvBytes > 0) && (recvBytes != SOCKET_ERROR))
			{
				//Check if all received data should be copied into the "data" argument without checking.
				if (dataSize == SIZE_MAX)
					memcpy(data, rxBuffer.data(), recvBytes);
				else
				{
					//Check if received data size is the expected size.
					if (recvBytes == dataSize)
						memcpy(data, rxBuffer.data(), recvBytes);
				}

				return recvBytes;
			}
			else
				return 0;
		}
		else
			return 0;
	}

	/*
	Call this function if the expected data is a normal ASCII string, or a Unicode string.
	Returns the number of bytes received.
	Arguments:
	  - strData:    Reference to the string variable used. Examples: std::string, std::wstring, std::u16string, std::u32string.
	  - senderIP:   The IP address of the sender.
	  - senderPort: The sender's port number.
	*/
	template<typename T>
	int ReceiveDataUDP(std::basic_string<T>& strData, std::string& senderIP, uint16_t& senderPort)
	{
		if (thisSocketInfo.Protocol == IPPROTO::IPPROTO_UDP)
		{
			int udpSenderLen = sizeof(sockaddr);
			SocketInfo& senderInfo = thisSocketInfo.RemoteSocketInfo.front();

			memset(rxBuffer.data(), '\0', rxBuffer.size());
			recvBytes = recvfrom(thisSocketInfo.SocketFD, (char*)rxBuffer.data(), (int)rxBuffer.size(), 0, (sockaddr*)&senderInfo.Addr, &udpSenderLen);
			
			GetIPFromSockAddr(senderInfo);
			senderIP = senderInfo.IPAddress;
			GetPortFromSockAddr(senderInfo);
			senderPort = senderInfo.Port;

			if ((recvBytes > 0) && (recvBytes != SOCKET_ERROR))
			{
				strData.assign((T*)rxBuffer.data());
				return recvBytes;
			}
			else
				return 0;
		}
		else
			return 0;
	}

	/*
	Call this function if the expected data is a normal ASCII string, or a Unicode string.
	Returns the number of bytes received.
	Arguments:
	  - strData: Reference to the string variable used. Examples: std::string, std::wstring, std::u16string, std::u32string.
	*/
	template<typename T>
	int ReceiveDataUDP(std::basic_string<T>& strData)
	{
		if (thisSocketInfo.Protocol == IPPROTO::IPPROTO_UDP)
		{
			int udpSenderLen = sizeof(sockaddr);
			SocketInfo& senderInfo = thisSocketInfo.RemoteSocketInfo.front();

			memset(rxBuffer.data(), '\0', rxBuffer.size());
			recvBytes = recvfrom(thisSocketInfo.SocketFD, (char*)rxBuffer.data(), (int)rxBuffer.size(), 0, (sockaddr*)&senderInfo.Addr, &udpSenderLen);

			if ((recvBytes > 0) && (recvBytes != SOCKET_ERROR))
			{
				strData.assign((T*)rxBuffer.data());
				return recvBytes;
			}
			else
				return 0;
		}
		else
			return 0;
	}
	

#elif SOCKET_PLATFORM_UNIX
	//Initialize socket
	uint16_t Init(const char* localIP, const uint16_t& localPort, const SocketType& type, const bool& isNonBlocking)
	{
		//TODO
	}

	//Close the socket
	bool CloseSocket()
	{
		return (close(localSocket) == 0);
	}

	//Check number of bytes waiting in the input buffer
	inline uint16_t CheckReceivedBytes()
	{
		ioctl(localSocket, FIONREAD, &recvBytes);
		return recvBytes;
	}
#endif

protected:
	//Get receive buffer size
	const uint32_t& GetRxBufferSize() const
	{
		return (uint32_t)rxBuffer.size();
	}

	//Set receive buffer size
	void SetRxBufferSize(const uint32_t& newSize)
	{
		if (newSize != rxBuffer.size())
			rxBuffer.resize((newSize > maxBufferSize) ? maxBufferSize : newSize);
	}

	//Get an IP address of a socket in string representation
	inline void GetIPFromSockAddr(SocketInfo& socket)
	{
		if (socket.IPAddress.size() != INET_ADDRSTRLEN)
			socket.IPAddress.resize(INET_ADDRSTRLEN);

		inet_ntop(AF_INET, &socket.Addr.sin_addr, &socket.IPAddress[0], INET_ADDRSTRLEN);
	}
	
	//Get the port number of a socket
	inline void GetPortFromSockAddr(SocketInfo& socket) const
	{
		socket.Port = ntohs(socket.Addr.sin_port);
	}

	template<typename T>
	inline bool IsUnicodeString(T* data)
	{
		size_t type = typeid(T).hash_code();

		return (
			(type == typeid(wchar_t).hash_code()) ||
			(type == typeid(char16_t).hash_code()) ||
			(type == typeid(char32_t).hash_code())
			);
	}

private:
	LocalSocketInfo thisSocketInfo;
	sockaddr_in		udpTargetAddr;
	
	std::vector<unsigned char> rxBuffer, txBuffer;
	uint32_t	maxBufferSize = MAX_UDP_BUFFER_SIZE;
	int recvBytes = 0;
};
#endif //_NETSOCKET_H_
