
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

// TCPServerApp:
// See TCPServerGUI.cpp for the implementation of this class
//

#define SERVER_PORT 28800
#define MAX_PENDING_CLIENTS 10
#define MAX_NUM_CLIENTS 10
constexpr const wchar_t* PREFIX_DISPNAME	= L"displayname=";
constexpr const wchar_t* PREFIX_SENDER		= L"sender=";
constexpr const wchar_t* PREFIX_RECEIVER	= L"receiver=";
constexpr const wchar_t	DELIM				= L'|';


struct AppUIControls
{
	CIPAddressCtrl* IPAddress	= nullptr;
	CListBox*		Clients		= nullptr;
	CListBox*		Messages	= nullptr;
	CButton*		StartServer = nullptr;
	CButton*		StopServer	= nullptr;
};

struct ClientDisplayInfo
{
	std::wstring DisplayName;
	char* IPAddress = nullptr;
	char* HostName	= nullptr;
};

class TCPServerApp : public CWinApp
{
public:
	TCPServerApp();

	AppUIControls* UIControls();
	NetSocket* GetSocket();
	std::mutex* GetClientThreadMutex();
	void AddDialogClient(const NetSocket::SocketInfo& client, const wchar_t* displayName);
	void RemoveDialogClient(const NetSocket::SocketInfo& client);
	void UpdateDialogClient(const NetSocket::SocketInfo& client, const wchar_t* newDisplayName);
	void PrintDialogMessage(const wchar_t* msg);
	void LaunchAcceptingThread();

	void CloseAllThreads() noexcept;


// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
private:
	AppUIControls uiCtrl;
	NetSocket serverSocket;

	bool threadMasterFlag = false;
	std::thread acceptThread;
	std::unordered_map<size_t, std::thread> clientThreads;	//<SocketFD, <Client thread>
	std::mutex clientThreadMutex;
	std::mutex dispMutex;

	std::unordered_map<size_t, std::pair<int, std::wstring>> clientsDisplay;	//<SocketFD, <Client list index, Display text>>

	inline void AcceptIncomingClients(TCPServerApp* appInst);
	inline void ProcessClientMessages(NetSocket::SocketInfo client);
};

extern TCPServerApp theApp;
