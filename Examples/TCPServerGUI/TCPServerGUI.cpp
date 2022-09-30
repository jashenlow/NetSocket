
// TCPServerGUI.cpp : Defines the class behaviors for the application.
//

#include "pch.h"
#include "framework.h"
#include "TCPServerGUI.h"
#include "TCPServerGUIDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// TCPServerApp

BEGIN_MESSAGE_MAP(TCPServerApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// TCPServerApp construction

TCPServerApp::TCPServerApp()
{
	// support Restart Manager
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

NetSocket* TCPServerApp::GetSocket()
{
	return &serverSocket;
}

void TCPServerApp::LaunchAcceptingThread()
{
	threadRunFlag = true;

	acceptThread = std::thread(&TCPServerApp::AcceptIncomingClients, this, std::ref(threadRunFlag), this);
}

void TCPServerApp::CloseAllThreads()
{
	if (threadRunFlag)
	{
		threadRunFlag = false;

		if (acceptThread.joinable())
			acceptThread.join();

		for (auto& iter : clientThreads)
		{
			if (iter.second.joinable())
				iter.second.join();
		}

		clientThreads.clear();
	}
}


// The one and only TCPServerApp object

TCPServerApp theApp;


// TCPServerApp initialization

BOOL TCPServerApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();


	AfxEnableControlContainer();

	// Create the shell manager, in case the dialog contains
	// any shell tree view or shell list view controls.
	CShellManager *pShellManager = new CShellManager;

	// Activate "Windows Native" visual manager for enabling themes in MFC controls
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	//SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	CTCPServerGUIDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	/*if (nResponse == -1)
	{
		TRACE(traceAppMsg, 0, "Warning: dialog creation failed, so application is terminating unexpectedly.\n");
		TRACE(traceAppMsg, 0, "Warning: if you are using MFC controls on the dialog, you cannot #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS.\n");
	}*/

	// Delete the shell manager created above.
	if (pShellManager != nullptr)
	{
		delete pShellManager;
	}

#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
	ControlBarCleanUp();
#endif

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

inline void TCPServerApp::AcceptIncomingClients(std::atomic<bool>& run, TCPServerApp* appInst)
{
	CTCPServerGUIDlg* p_Dialog = (CTCPServerGUIDlg*)m_pMainWnd;

	while (run)
	{
		if (serverSocket.AcceptIncomingClientTCP())
		{
			const NetSocket::SocketInfo& newClient = serverSocket.GetRemoteSocketInfo().back();
			
			bool isDuplicate = false;

			//Delete any thread objects that aren't running.
			for (const auto& iter : clientThreads)
			{
				if (!iter.second.joinable())
					clientThreads.erase(iter.first);
			}

			//Drop client if the IP and HostName already exists
			for (auto& socket : serverSocket.GetRemoteSocketInfo())
			{
				if ((strcmp(newClient.IPAddress, socket.IPAddress) == 0) &&
					(strcmp(newClient.HostName, socket.HostName) == 0) &&
					(newClient.SocketFD != socket.SocketFD))
				{
					clientMutex.lock();
					serverSocket.DropClientConnectionTCP(newClient.SocketFD);
					clientMutex.unlock();

					isDuplicate = true;
					break;
				}
			}

			if (!isDuplicate)
			{
				if (clientThreads.size() < MAX_NUM_CLIENTS)
				{
					PostMessage(m_pMainWnd->m_hWnd, WM_ADD_CLIENT, (WPARAM)&newClient, NULL);
					clientThreads.insert({ newClient.SocketFD, std::thread(&TCPServerApp::ProcessClientMessages, appInst, std::ref(threadRunFlag), newClient) });
				}
				else
				{
					clientMutex.lock();
					serverSocket.DropClientConnectionTCP(newClient.SocketFD);
					clientMutex.unlock();
				}
			}
		}
		else
			std::this_thread::sleep_for(std::chrono::milliseconds(10));	
	}
}

inline void TCPServerApp::ProcessClientMessages(std::atomic<bool>& run, NetSocket::SocketInfo client)
{
	std::wstring receivedMsg;
	CTCPServerGUIDlg* p_Dialog = (CTCPServerGUIDlg*)m_pMainWnd;
	bool isNonBlocking = (serverSocket.GetThisSocketInfo().NonBlocking == 1);

	while (run)
	{
		int receivedBytes = serverSocket.ReceiveFromClientTCP(client, receivedMsg);

		if (receivedBytes > 0)
		{		
			//Determine the type of message received
			if (receivedMsg.find(PREFIX_DISPNAME) != std::wstring::npos)
			{
				std::wstring newName = receivedMsg.substr(wcslen(PREFIX_DISPNAME), receivedMsg.length() - wcslen(PREFIX_DISPNAME));
				PostMessage(m_pMainWnd->m_hWnd, WM_UPDATE_CLIENT, (WPARAM)&client, (LPARAM)newName.c_str());
				
				CString dispMsg = L"[";
				dispMsg += CA2CT(client.IPAddress);
				dispMsg += L"] DISPLAY NAME = ";
				dispMsg += newName.c_str();
				PostMessage(m_pMainWnd->m_hWnd, WM_APPEND_MESSAGE, (WPARAM)dispMsg.GetString(), NULL);

				std::wstring sendMsg;
				ConstructAddClientMessage(newName.c_str(), CA2CT(client.IPAddress), CA2CT(client.HostName), sendMsg);
				
				for (NetSocket::SocketInfo c : serverSocket.GetRemoteSocketInfo())
				{
					if (c != client)
					{
						//Send "Add Client" message to every client except this one.
						serverSocket.SendToClientTCP(c.SocketFD, sendMsg.c_str(), NetUtil::GetStringSizeBytes(sendMsg));
						//Send "Add Client" message to this client, to update it's client list.
						serverSocket.SendToClientTCP(client.SocketFD, sendMsg.c_str(), NetUtil::GetStringSizeBytes(sendMsg));
					}
						
				}
			}
			else if ((receivedMsg.find(PREFIX_SENDER) != std::wstring::npos) && (receivedMsg.find(PREFIX_RECEIVER) != std::wstring::npos))
			{
				std::string receiverIP;
				std::wstring senderIP;

				std::vector<std::wstring> splitStrings;
				TokenizeReceivedString(receivedMsg, DELIM, splitStrings);

				senderIP	= splitStrings[0].substr(wcslen(PREFIX_SENDER), splitStrings[0].length() - wcslen(PREFIX_SENDER)).c_str();
				receiverIP	= CT2CA(splitStrings[1].substr(wcslen(PREFIX_RECEIVER), splitStrings[1].length() - wcslen(PREFIX_RECEIVER)).c_str());
				
				CString dispMsg;
				ConstructSenderReceiverDisplay(senderIP.c_str(), CA2CT(receiverIP.c_str()), splitStrings.back().c_str(), dispMsg);
				PostMessage(m_pMainWnd->m_hWnd, WM_APPEND_MESSAGE, (WPARAM)dispMsg.GetString(), NULL);

				for (auto& socket : serverSocket.GetRemoteSocketInfo())
				{
					if (strcmp(receiverIP.c_str(), socket.IPAddress) == 0)
					{
						//Relay the message to the intended recipient.
						serverSocket.SendToClientTCP(socket.SocketFD, receivedMsg.c_str(), NetUtil::GetStringSizeBytes(receivedMsg));
						break;
					}
				}
			}
		}
		else if (receivedBytes == SOCKET_ERROR)
		{
			if (isNonBlocking)
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		else
		{
			//Client has disconnected.
			CString ipAddress(CA2CT(client.IPAddress));
			PostMessage(m_pMainWnd->m_hWnd, WM_REMOVE_CLIENT, (WPARAM)ipAddress.GetString(), NULL);

			//Notify other clients that this one has disconnected.
			std::wstring sendMsg;
			for (auto& socket : serverSocket.GetRemoteSocketInfo())
			{
				if (client != socket)
				{
					sendMsg = PREFIX_REMOVE_CLIENT;
					sendMsg += CA2CT(client.IPAddress);
					serverSocket.SendToClientTCP(socket.SocketFD, sendMsg.c_str(), NetUtil::GetStringSizeBytes(sendMsg));
				}
			}

			clientMutex.lock();
			serverSocket.DropClientConnectionTCP(client.SocketFD);
			clientMutex.unlock();
			break;
		}
	}
}

inline void TCPServerApp::ConstructAddClientMessage(const wchar_t* name, const wchar_t* ipAddress, const wchar_t* hostName, std::wstring& outString)
{
	outString = PREFIX_ADD_CLIENT;
	outString += DELIM;
	outString += name;
	outString += DELIM;
	outString += ipAddress;
	outString += DELIM;
	outString += hostName;
}

inline void TCPServerApp::TokenizeReceivedString(const std::wstring& string, const wchar_t& delim, std::vector<std::wstring>& tokens)
{
	size_t start = 0, end = 0;

	while ((start = string.find_first_not_of(delim, end)) != std::wstring::npos)
	{
		end = string.find(DELIM, start);
		tokens.emplace_back(string.substr(start, end - start));
	}
}

inline void TCPServerApp::ConstructSenderReceiverDisplay(const wchar_t* senderIP, const wchar_t* receiverIP, const wchar_t* msg, CString& outString)
{
	outString = L"[";
	outString += senderIP;
	outString += L"] --> [";
	outString += receiverIP;
	outString += L"]: ";
	outString += msg;
}


