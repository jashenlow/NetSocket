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

#define GetSocketError WSAGetLastError()
#define ConnectingErrCode WSAEWOULDBLOCK
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

#define GetSocketError errno
#define ConnectingErrCode EINPROGRESS
#endif

#include <iostream>
#include <string>
#include <vector>

constexpr uint16_t	BUFFER_DEFAULT_SIZE	= 4096;
constexpr uint16_t	MAX_UDP_BUFFER_SIZE	= 65507;
constexpr uint32_t	MAX_TCP_BUFFER_SIZE	= 1048576;
constexpr uint32_t  DEFAULT_NUM_REMOTE	= 100;

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
		char HostName[NI_MAXHOST];
		char Service[NI_MAXSERV];
		char			IPAddress[INET_ADDRSTRLEN];
		sockaddr_in		Addr;	//IPv4	
		size_t			SocketFD;		
		uint16_t		Port;

		SocketInfo()
		{
			Reset();
		}

		void Reset()
		{
			memset(HostName, '\0', sizeof(HostName));
			memset(Service, '\0', sizeof(Service));
			memset(IPAddress, '\0', sizeof(char) * INET_ADDRSTRLEN);
			
			Addr.sin_family = AF_INET;
			memset(&Addr.sin_addr, 0, sizeof(Addr.sin_addr));
			Addr.sin_port = 0;
			memset(Addr.sin_zero, '\0', sizeof(Addr.sin_zero));

			SocketFD	= 0;
			Port		= 0;
		}

		bool operator==(const SocketInfo& other)
		{
			return (
				//(strcmp(HostName, other.HostName) == 0) &&
				//(strcmp(Service, other.Service) == 0) &&
				//(strcmp(IPAddress, other.IPAddress) == 0) &&
				(Addr.sin_addr.s_addr == other.Addr.sin_addr.s_addr) &&
				(Addr.sin_port == other.Addr.sin_port) &&
				(SocketFD == other.SocketFD)// &&
				//(Port == other.Port)
				);
		}

		bool operator!=(const SocketInfo& other)
		{
			return (
				//(strcmp(HostName, other.HostName) != 0) ||
				//(strcmp(Service, other.Service) != 0) ||
				//(strcmp(IPAddress, other.IPAddress) != 0) ||
				(Addr.sin_addr.s_addr != other.Addr.sin_addr.s_addr) ||
				(Addr.sin_port != other.Addr.sin_port) ||
				(SocketFD != other.SocketFD)// ||
				//(Port != other.Port)
				);
		}
	};

	struct LocalSocketInfo : SocketInfo
	{
		SocketType	Type		= SocketType::UDP;
		IPPROTO		Protocol	= IPPROTO::IPPROTO_UDP;
		unsigned long Blocking	= 0;	//0 = Non-blocking, 1 = blocking.	
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
		CloseThisSocket();
	}

	//Get all info about this socket
	const LocalSocketInfo& GetThisSocketInfo() const
	{
		return thisSocketInfo;
	}

	//Get the list of remote socket info.
	const std::vector<SocketInfo>& GetRemoteSocketInfo() const
	{
		return remoteSocketInfo;
	}

	const uint32_t& GetMaxRemoteSockets() const
	{
		return maxRemoteSocketInfo;
	}

	void SetMaxRemoteSockets(const uint32_t& num)
	{
		maxRemoteSocketInfo = num;
	}

	//Returns the current hostname of the specified socket.
	const char* GetHostName(const SocketInfo& socket) const
	{
		return socket.HostName;
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
	WinsockError Init(const char* localIP, const uint16_t& localPort, const SocketType& type, const bool& isNonBlocking = false)
	{
		connectedToServer		= false;
		maxBufferSize			= (type == SocketType::UDP) ? MAX_UDP_BUFFER_SIZE : MAX_TCP_BUFFER_SIZE;
		WinsockError errorcode	= WinsockError::OK;

		//Set local socket details
		thisSocketInfo.Blocking			= (unsigned long)isNonBlocking;
		snprintf(thisSocketInfo.IPAddress, INET_ADDRSTRLEN, "%s", localIP);
		thisSocketInfo.Port				= localPort;
		thisSocketInfo.Type				= type;
		thisSocketInfo.Protocol			= (type == SocketType::UDP) ? IPPROTO::IPPROTO_UDP : IPPROTO::IPPROTO_TCP;

		thisSocketInfo.Addr.sin_family = AF_INET;
		inet_pton(thisSocketInfo.Addr.sin_family, localIP, &thisSocketInfo.Addr.sin_addr);
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
				if (type != SocketType::TCP_CLIENT)	//TCP clients don't need to bind.
				{
					//Bind socket to localIP and localPort
					if (bind(thisSocketInfo.SocketFD, (sockaddr*)&thisSocketInfo.Addr, sizeof(sockaddr_in)) == SOCKET_ERROR)
						errorcode = WinsockError::FAIL_BIND;
				}

				if (errorcode == WinsockError::OK)
				{
					//Set socket blocking flag
					ioctlsocket(thisSocketInfo.SocketFD, FIONBIO, &thisSocketInfo.Blocking);

					if (rxBuffer.empty())
						SetRxBufferSize(maxBufferSize);
					if (txBuffer.empty())
						SetTxBufferSize(BUFFER_DEFAULT_SIZE);

					//Force close all remote sockets
					if (!remoteSocketInfo.empty())
					{
						for (auto& sockInfo : remoteSocketInfo)
							closesocket(sockInfo.SocketFD);

						remoteSocketInfo.clear();
					}

					if (thisSocketInfo.Type == SocketType::TCP_CLIENT)
						remoteSocketInfo.resize(1);

					//Get the hostname of this machine.
					ResolveHostName(thisSocketInfo);
				}
			}
		}

		if (errorcode != WinsockError::OK)
			CloseThisSocket();

		return errorcode;
	}

	//Close the socket
	bool CloseThisSocket() const
	{
		if (closesocket(thisSocketInfo.SocketFD) != SOCKET_ERROR)
			return (WSACleanup() == 0);
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
	bool SendUDP(T* data, const size_t& dataSize, const char* targetIP, const uint16_t& targetPort)
	{
		//NOTE: Using a templated type for the "data" argument since void* doesn't accept const types.
		if ((data != nullptr) && (thisSocketInfo.Protocol == IPPROTO::IPPROTO_UDP))
		{
			udpTargetAddr.sin_family = AF_INET;
			inet_pton(udpTargetAddr.sin_family, targetIP, &udpTargetAddr.sin_addr);
			udpTargetAddr.sin_port = htons(targetPort);
			
			return (sendto(thisSocketInfo.SocketFD, (char*)data, (int)dataSize, 0, (sockaddr*)&udpTargetAddr, sizeof(sockaddr)) != SOCKET_ERROR);
		}
		else
			return false;
	}

	/*
	Call this function if the expected data has a fixed size(like structs), or if you simply want to copy the data into a buffer.
	Returns the number of bytes received.
	Arguments:
	  - data:           Pointer to where you would like to store the received data.
	  - dataSize:       The expected data size to be received. Set to SIZE_MAX if the expected data size is unknown.
	                    If set to SIZE_MAX, ensure that the "data" argument points to a buffer large enough to avoid potential overruns.
	  - senderIP:       The IP address of the sender.
	  - senderPort:     The sender's port number.
	  - storeSenderInfo: If set to true, the sender's info is saved and can be retrieved by calling GetRemoteSocketInfo(). Applicable if this socket is a UDP server.
	*/
	int ReceiveUDP(void* data, const size_t& dataSize, std::string& senderIP, uint16_t& senderPort, bool storeSenderInfo = false)
	{
		if ((data != nullptr) && (thisSocketInfo.Protocol == IPPROTO::IPPROTO_UDP))
		{
			int udpSenderLen = sizeof(sockaddr);
			SocketInfo senderInfo;
			
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

				if (storeSenderInfo)
				{
					if (remoteSocketInfo.size() < maxRemoteSocketInfo)
					{
						ResolveHostName(senderInfo);
						CheckAndAppendSocketInfo(senderInfo);
					}
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
	  - data:           Pointer to where you would like to store the received data.
	  - dataSize:       The expected data size to be received. Set to SIZE_MAX if the expected data size is unknown.
			            If set to SIZE_MAX, ensure that the "data" argument points to a buffer large enough to avoid potential overruns.
	  - storeSenderInfo: If set to true, the sender's info is saved and can be retrieved by calling GetRemoteSocketInfo(). Applicable if this socket is a UDP server.
	*/
	int ReceiveUDP(void* data, const size_t& dataSize, bool storeSenderInfo = false)
	{
		if ((data != nullptr) && (thisSocketInfo.Protocol == IPPROTO::IPPROTO_UDP))
		{
			int udpSenderLen = sizeof(sockaddr);
			SocketInfo senderInfo;

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

				if (storeSenderInfo)
				{
					if (remoteSocketInfo.size() < maxRemoteSocketInfo)
					{
						GetIPFromSockAddr(senderInfo);
						GetPortFromSockAddr(senderInfo);
						ResolveHostName(senderInfo);

						CheckAndAppendSocketInfo(senderInfo);
					}
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
	  - strData:        Reference to the string variable used. Examples: std::string, std::wstring, std::u16string, std::u32string.
	  - senderIP:       The IP address of the sender.
	  - senderPort:     The sender's port number.
	  - storeSenderInfo: If set to true, the sender's info is saved and can be retrieved by calling GetRemoteSocketInfo(). Applicable if this socket is a UDP server.
	*/
	template<typename T>
	int ReceiveUDP(std::basic_string<T>& strData, std::string& senderIP, uint16_t& senderPort, bool storeSenderInfo = false)
	{
		if (thisSocketInfo.Protocol == IPPROTO::IPPROTO_UDP)
		{
			int udpSenderLen = sizeof(sockaddr);
			SocketInfo senderInfo;

			memset(rxBuffer.data(), '\0', rxBuffer.size());
			recvBytes = recvfrom(thisSocketInfo.SocketFD, (char*)rxBuffer.data(), (int)rxBuffer.size(), 0, (sockaddr*)&senderInfo.Addr, &udpSenderLen);
			
			GetIPFromSockAddr(senderInfo);
			senderIP = senderInfo.IPAddress;
			GetPortFromSockAddr(senderInfo);
			senderPort = senderInfo.Port;

			if ((recvBytes > 0) && (recvBytes != SOCKET_ERROR))
			{
				strData.assign((T*)rxBuffer.data());
				
				if (storeSenderInfo)
				{
					if (remoteSocketInfo.size() < maxRemoteSocketInfo)
					{
						ResolveHostName(senderInfo);
						CheckAndAppendSocketInfo(senderInfo);
					}
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
	  - strData:        Reference to the string variable used. Examples: std::string, std::wstring, std::u16string, std::u32string.
	  - storeSenderInfo: If set to true, the sender's info is saved and can be retrieved by calling GetRemoteSocketInfo(). Applicable if this socket is a UDP server.
	*/
	template<typename T>
	int ReceiveUDP(std::basic_string<T>& strData, bool storeSenderInfo = false)
	{
		if (thisSocketInfo.Protocol == IPPROTO::IPPROTO_UDP)
		{
			int udpSenderLen = sizeof(sockaddr);
			SocketInfo senderInfo;

			memset(rxBuffer.data(), '\0', rxBuffer.size());
			recvBytes = recvfrom(thisSocketInfo.SocketFD, (char*)rxBuffer.data(), (int)rxBuffer.size(), 0, (sockaddr*)&senderInfo.Addr, &udpSenderLen);

			if ((recvBytes > 0) && (recvBytes != SOCKET_ERROR))
			{
				strData.assign((T*)rxBuffer.data());

				if (storeSenderInfo)
				{
					if (remoteSocketInfo.size() < maxRemoteSocketInfo)
					{
						GetIPFromSockAddr(senderInfo);
						GetPortFromSockAddr(senderInfo);
						ResolveHostName(senderInfo);

						CheckAndAppendSocketInfo(senderInfo);
					}
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
	Call this function to append received data to a stream, and if the expected data is a normal ASCII string, or a Unicode string.
	Returns the number of bytes received.
	Arguments:
	  - streamData: Reference to the stream variable used.
	  - senderIP:   The IP address of the sender.
	  - senderPort: The sender's port number.
	  - storeSenderInfo: If set to true, the sender's info is saved and can be retrieved by calling GetRemoteSocketInfo(). Applicable if this socket is a UDP server.
	*/
	template<typename T>
	int ReceiveUDP(std::basic_iostream<T>& streamData, std::string& senderIP, uint16_t& senderPort, bool storeSenderInfo = false)
	{
		if (thisSocketInfo.Protocol == IPPROTO::IPPROTO_UDP)
		{
			int udpSenderLen = sizeof(sockaddr);
			SocketInfo senderInfo;

			memset(rxBuffer.data(), '\0', rxBuffer.size());
			recvBytes = recvfrom(thisSocketInfo.SocketFD, (char*)rxBuffer.data(), (int)rxBuffer.size(), 0, (sockaddr*)&senderInfo.Addr, &udpSenderLen);

			GetIPFromSockAddr(senderInfo);
			senderIP = senderInfo.IPAddress;
			GetPortFromSockAddr(senderInfo);
			senderPort = senderInfo.Port;

			if ((recvBytes > 0) && (recvBytes != SOCKET_ERROR))
			{
				streamData << (T*)rxBuffer.data();
				
				if (storeSenderInfo)
				{
					if (remoteSocketInfo.size() < maxRemoteSocketInfo)
					{
						ResolveHostName(senderInfo);
						CheckAndAppendSocketInfo(senderInfo);
					}
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
	Call this function to append received data to a stream, and if the expected data is a normal ASCII string, or a Unicode string.
	Returns the number of bytes received.
	Arguments:
	  - streamData: Reference to the stream variable used.
	  - storeSenderInfo: If set to true, the sender's info is saved and can be retrieved by calling GetRemoteSocketInfo(). Applicable if this socket is a UDP server.
	*/
	template<typename T>
	int ReceiveUDP(std::basic_iostream<T>& streamData, bool storeSenderInfo = false)
	{
		if (thisSocketInfo.Protocol == IPPROTO::IPPROTO_UDP)
		{
			int udpSenderLen = sizeof(sockaddr);
			SocketInfo senderInfo;

			memset(rxBuffer.data(), '\0', rxBuffer.size());
			recvBytes = recvfrom(thisSocketInfo.SocketFD, (char*)rxBuffer.data(), (int)rxBuffer.size(), 0, (sockaddr*)&senderInfo.Addr, &udpSenderLen);	

			if ((recvBytes > 0) && (recvBytes != SOCKET_ERROR))
			{
				streamData << (T*)rxBuffer.data();

				if (storeSenderInfo)
				{
					if (remoteSocketInfo.size() < maxRemoteSocketInfo)
					{
						GetIPFromSockAddr(senderInfo);
						GetPortFromSockAddr(senderInfo);
						ResolveHostName(senderInfo);

						CheckAndAppendSocketInfo(senderInfo);
					}
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
	Attempts to connect to a TCP server if this socket is a TCP client.
	Returns the error code of the last operation. A value of 0 means connection was successful.
	Arguments:
	  - server: This argument accepts either a domain name or an IP address.
	  - serverPort: The port number of the server to connect to.
	*/
	int ConnectToServerTCP(const char* server, const uint16_t& serverPort)
	{
		if (thisSocketInfo.Type == SocketType::TCP_CLIENT)
		{
			if (remoteSocketInfo.size() != 1)
				remoteSocketInfo.resize(1);

			SocketInfo& serverInfo = remoteSocketInfo.front();

			serverInfo.Addr.sin_family = AF_INET;
			
			snprintf(serverInfo.Service, sizeof(serverInfo.Service), "%d", serverPort);
			
			/*
			Check if the "server" argument is a domain name or IP by calling inet_pton.
			Return value of 1 refers to a valid IP address.
			*/
			if (inet_pton(serverInfo.Addr.sin_family, server, &serverInfo.Addr.sin_addr) != 1)
			{
				//Invalid IP address has been entered, treating it as a domain name.
				addrinfo hints = {};
				hints.ai_family		= AF_UNSPEC;
				hints.ai_flags		= AI_CANONNAME;
				hints.ai_socktype	= SOCK_STREAM;
				hints.ai_protocol	= IPPROTO::IPPROTO_TCP;

				addrinfo* addrRes = nullptr;
				
				if (getaddrinfo(server, serverInfo.Service, &hints, &addrRes) == 0)
				{
					if ((addrRes != nullptr) && (addrRes->ai_addr != nullptr))
						serverInfo.Addr = *(sockaddr_in*)addrRes->ai_addr;							
					else
						return GetSocketError;
				}
				else
					return GetSocketError;

				if (addrRes != nullptr)
					freeaddrinfo(addrRes);
			}
			else
			{
				//Valid IP address has been entered.
				serverInfo.Addr.sin_port = htons(serverPort);
			}

			GetIPFromSockAddr(serverInfo);
			GetPortFromSockAddr(serverInfo);
			ResolveHostName(serverInfo);

			bool ret = (connect(thisSocketInfo.SocketFD, (sockaddr*)&serverInfo.Addr, sizeof(sockaddr)) != SOCKET_ERROR);
			
			if (!ret && (GetSocketError != ConnectingErrCode))
				serverInfo.Reset();

			connectedToServer = ret;
		}
		
		return GetSocketError;
	}

	/*
	NOTE: This will be a blocking call due to calling of getsockopt, so call this function asynchronously if you have to.
	Return values:
	  1:  Successful connection.
	  0:  Timeout expired.
	  -1: Connection failed.
	*/
	int CheckForConnectNonBlockTCP(const double& timeoutSec)
	{
		if (thisSocketInfo.Type == SocketType::TCP_CLIENT)
		{
			fd_set writeFD;
			FD_ZERO(&writeFD);
			FD_SET(thisSocketInfo.SocketFD, &writeFD);

			timeval timeoutVal = { 0, 0 };
			timeoutVal.tv_sec	= (long)timeoutSec;
			timeoutVal.tv_usec	= (long)((timeoutSec - (double)timeoutVal.tv_sec) * 1000 * 1000);

			int result = select((int)thisSocketInfo.SocketFD, nullptr, &writeFD, nullptr, &timeoutVal);

			if (result > 0)	//Successful
			{
				//Check if this socket is writeable
				int errCode = 0;
				socklen_t errCodeLen = sizeof(errCode);

				int canWrite = getsockopt(thisSocketInfo.SocketFD, SOL_SOCKET, SO_ERROR, (char*)&errCode, &errCodeLen);
				connectedToServer = true;

				return (canWrite == 0) ? 1 : canWrite;	//1 = success, SOCKET_ERROR = fail.
			}
			else if (result == 0) //timeout expired
				return 0;
			else
				return SOCKET_ERROR;
		}
		else
			return SOCKET_ERROR;
	}

	/*
	Call this to send any data type to the connected TCP server.
	NOTE: For string types, ensure that dataSize = stringlength + 1.
	NOTE: For Unicode strings, ensure that dataSize = (stringlength + 1) * charsize.
	Arguments:
	  - data:       Pointer to the data to be sent.
	  - dataSize:   Size of the data to send.
	*/
	template<typename T>
	bool SendToServerTCP(T* data, const size_t& dataSize)
	{
		//NOTE: Using a templated type for the "data" argument since void* doesn't accept const types.
		if ((data != nullptr) && connectedToServer && (thisSocketInfo.Type == SocketType::TCP_CLIENT))
			return (send(thisSocketInfo.SocketFD, (char*)data, (int)dataSize, 0) != SOCKET_ERROR);
		else
			return false;
	}

	/*
	Call this function if the expected data has a fixed size(like structs), or if you simply want to copy the data into a buffer.
	Returns the number of bytes received.
	Arguments:
	  - data:     Pointer to where you would like to store the received data.
	  - maxSize:  The maximum expected data size to be received. Set to SIZE_MAX if the expected data size is unknown.
				  If set to SIZE_MAX, ensure that the "data" argument points to a buffer large enough to avoid potential overruns.
	*/
	int ReceiveFromServerTCP(void* data, const size_t& maxSize)
	{
		if ((data != nullptr) && connectedToServer && (thisSocketInfo.Type == SocketType::TCP_CLIENT))
		{
			memset(rxBuffer.data(), '\0', rxBuffer.size());
			recvBytes = recv(thisSocketInfo.SocketFD, (char*)rxBuffer.data(), (int)rxBuffer.size(), 0);

			if ((recvBytes > 0) && (recvBytes != SOCKET_ERROR))
			{
				//Check if all received data should be copied into the "data" argument without checking.
				if (maxSize == SIZE_MAX)
					memcpy(data, rxBuffer.data(), recvBytes);
				else
				{
					//Check if received data size is the expected size.
					if ((size_t)recvBytes <= maxSize)
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
	  - strData:  Reference to the string variable used. Examples: std::string, std::wstring, std::u16string, std::u32string.
	  - maxSize:  The maximum expected data size to be received. Set to SIZE_MAX if the expected data size is unknown.
				  If set to SIZE_MAX, ensure that the "data" argument points to a buffer large enough to avoid potential overruns.
	*/
	template<typename T>
	int ReceiveFromServerTCP(std::basic_string<T>& strData)
	{
		if (connectedToServer && (thisSocketInfo.Type == SocketType::TCP_CLIENT))
		{
			memset(rxBuffer.data(), '\0', rxBuffer.size());
			recvBytes = recv(thisSocketInfo.SocketFD, (char*)rxBuffer.data(), (int)rxBuffer.size(), 0);

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
	Call this function to append received data to a stream, and if the expected data is a normal ASCII string, or a Unicode string.
	Returns the number of bytes received.
	Arguments:
	  - streamata: Reference to the stream variable used.
	  - maxSize:   The maximum expected data size to be received. Set to SIZE_MAX if the expected data size is unknown.
				   If set to SIZE_MAX, ensure that the "data" argument points to a buffer large enough to avoid potential overruns.
	*/
	template<typename T>
	int ReceiveFromServerTCP(std::basic_iostream<T>& streamData)
	{
		if (connectedToServer && (thisSocketInfo.Type == SocketType::TCP_CLIENT))
		{
			memset(rxBuffer.data(), '\0', rxBuffer.size());
			recvBytes = recv(thisSocketInfo.SocketFD, (char*)rxBuffer.data(), (int)rxBuffer.size(), 0);

			if ((recvBytes > 0) && (recvBytes != SOCKET_ERROR))
			{
				streamData << ((T*)rxBuffer.data());
				return recvBytes;
			}
			else
				return 0;
		}
		else
			return 0;
	}

	//Sets this socket into TCP listening mode, only if it was initialized as a TCP server.
	bool StartListeningTCP()
	{
		if (thisSocketInfo.Type == SocketType::TCP_SERVER)
			return (listen(thisSocketInfo.SocketFD, SOMAXCONN) != SOCKET_ERROR);
		else
			return false;
	}

	/*
	Stops this socket from listening for incoming TCP connections, if this socket was initialized as a TCP server.
	NOTE: This call will close this socket.
	*/
	bool StopListeningTCP()
	{
		if (thisSocketInfo.Type == SocketType::TCP_SERVER)
			return CloseThisSocket();
		else
			return false;
	}

	//Closes the connection between this socket and a connected client by speciofying the client's Socket File Descriptor.
	bool DropClientConnectionTCP(const size_t& sockFD)
	{
		if (thisSocketInfo.Type == SocketType::TCP_SERVER)
		{
			for (auto sockIter = remoteSocketInfo.begin(); sockIter != remoteSocketInfo.end(); sockIter++)
			{
				if (sockIter->SocketFD == sockFD)
				{
					closesocket(sockFD);
					sockIter = remoteSocketInfo.erase(sockIter);
					break;
				}
			}

			return true;
		}
		else
			return false;
	}




#elif SOCKET_PLATFORM_UNIX
	//Initialize socket
	uint16_t Init(const char* localIP, const uint16_t& localPort, const SocketType& type, const bool& isNonBlocking)
	{
		//TODO
	}

	//Close the socket
	bool CloseThisSocket()
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
		inet_ntop(AF_INET, &socket.Addr.sin_addr, socket.IPAddress, INET_ADDRSTRLEN);
	}
	
	//Get the port number of a socket
	inline void GetPortFromSockAddr(SocketInfo& socket) const
	{
		socket.Port = ntohs(socket.Addr.sin_port);
	}

	inline bool ResolveHostName(SocketInfo& socket)
	{
		memset(socket.Service, '\0', sizeof(socket.Service));
		memset(socket.HostName, '\0', sizeof(socket.HostName));	

		return (getnameinfo((sockaddr*)&socket.Addr, sizeof(sockaddr), socket.HostName, NI_MAXHOST, socket.Service, NI_MAXSERV, 0) == 0);
	}

	inline void CheckAndAppendSocketInfo(const SocketInfo& socket)
	{
		if (std::find(remoteSocketInfo.begin(), remoteSocketInfo.end(), socket) == remoteSocketInfo.end())
			remoteSocketInfo.emplace_back(socket);
	}

private:
	/*
	RemoteSocketInfo is used to store the following based on the current configuration of this socket:
		[UDP]: Info of the last sender from the last received message.
		[TCP_CLIENT]: Info of the server this socket is connected to.
		[TCP_SERVER]: Info of all clients currently connected to this socket.
	*/
	std::vector<SocketInfo> remoteSocketInfo;
	uint32_t maxRemoteSocketInfo = DEFAULT_NUM_REMOTE;
	
	LocalSocketInfo thisSocketInfo;
	sockaddr_in		udpTargetAddr;
	
	std::vector<unsigned char> rxBuffer, txBuffer;
	uint32_t maxBufferSize = MAX_UDP_BUFFER_SIZE;
	int recvBytes = 0;
	bool connectedToServer = false;
};
#endif //_NETSOCKET_H_
