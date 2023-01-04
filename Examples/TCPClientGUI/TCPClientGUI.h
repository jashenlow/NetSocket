
// TCPClientGUI.h : main header file for the PROJECT_NAME application
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

// TCPClientApp:
// See TCPClientGUI.cpp for the implementation of this class
//


#define SERVER_PORT 28800
#define NONBLOCK_TIMEOUT 2.0
constexpr const wchar_t* PREFIX_DISPNAME		= L"0=";
constexpr const wchar_t* PREFIX_ADD_CLIENT		= L"1=";
constexpr const wchar_t* PREFIX_REMOVE_CLIENT	= L"2=";
constexpr const wchar_t* PREFIX_SENDER			= L"3=";
constexpr const wchar_t* PREFIX_RECEIVER		= L"4=";
constexpr const wchar_t* PREFIX_SERVER_DOWN		= L"5=";
constexpr const wchar_t	DELIM					= L'|';

class TCPClientApp : public CWinApp
{
public:
	TCPClientApp();

	NetSocket* GetSocket();
	void StartNetworkThread();
	void CloseNetworkThread();
	void SetConnected(const bool& flag);
	bool IsConnected();
	void ConstructSenderReceiverDisplay(const wchar_t* sender, const wchar_t* receiver, const wchar_t* msg, CString& outString);


// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
private:
	NetSocket clientSocket;

	bool connectedToServer	= false;
	bool threadRunFlag		= false;
	std::thread networkThread;

	inline void ProcessNetworkMessages(bool& run);
	
};

extern TCPClientApp theApp;
