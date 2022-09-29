
// TCPServerGUI.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols
#include <string>

#include "../../NetUtil.h"
#include "../../NetSocket.h"

#include <string>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <atomic>

// TCPServerApp:
// See TCPServerGUI.cpp for the implementation of this class
//

#define SERVER_PORT 28800
#define MAX_PENDING_CLIENTS 10
#define MAX_NUM_CLIENTS 100
constexpr const wchar_t* PREFIX_DISPNAME		= L"0=";
constexpr const wchar_t* PREFIX_ADD_CLIENT		= L"1=";
constexpr const wchar_t* PREFIX_REMOVE_CLIENT	= L"2=";
constexpr const wchar_t* PREFIX_SENDER			= L"3=";
constexpr const wchar_t* PREFIX_RECEIVER		= L"4=";
constexpr const wchar_t* PREFIX_SERVER_DOWN		= L"5=";
constexpr const wchar_t	DELIM					= L'|';

class TCPServerApp : public CWinApp
{
public:
	TCPServerApp();

	NetSocket* GetSocket();
	void LaunchAcceptingThread();
	void CloseAllThreads();


// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
private:
	NetSocket serverSocket;

	std::atomic<bool> threadRunFlag = false;
	std::thread acceptThread;

	std::unordered_map<size_t, std::thread> clientThreads;	//<SocketFD, <Client thread>
	std::mutex clientMutex;

	inline void AcceptIncomingClients(std::atomic<bool>& run, TCPServerApp* appInst);
	inline void ProcessClientMessages(std::atomic<bool>& run, NetSocket::SocketInfo client);
	inline void ConstructAddClientMessage(const wchar_t* name, const wchar_t* ipAddress, const wchar_t* hostName, std::wstring& outString);
	inline void TokenizeReceivedString(const std::wstring& string, const wchar_t& delim, std::vector<std::wstring>& tokens);
	inline void ConstructSenderReceiverDisplay(const wchar_t* senderIP, const wchar_t*receiverIP, const wchar_t* msg, CString& outString);
};

extern TCPServerApp theApp;
