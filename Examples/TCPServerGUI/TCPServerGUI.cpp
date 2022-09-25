
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

AppUIControls* TCPServerApp::UIControls()
{
	return &uiCtrl;
}

NetSocket* TCPServerApp::GetSocket()
{
	return &serverSocket;
}

std::mutex* TCPServerApp::GetClientThreadMutex()
{
	return &clientThreadMutex;
}

void TCPServerApp::AddDialogClient(const NetSocket::SocketInfo& client, const wchar_t* displayName)
{
	std::wstring displayStr;
	displayStr += ((displayName == nullptr) || (displayName[0] == '\0')) ? L"(empty)" : displayName;
	displayStr += L"\t\t";
	displayStr += CA2CT(client.IPAddress);
	displayStr += L"\t";
	displayStr += CA2CT(client.HostName);

	dispMutex.lock();
	int index = uiCtrl.Clients->AddString(displayStr.c_str());
	
	clientsDisplay.insert({ client.SocketFD, {index, displayStr} });
	dispMutex.unlock();
}

void TCPServerApp::RemoveDialogClient(const NetSocket::SocketInfo& client)
{
	int currentIndex = clientsDisplay[client.SocketFD].first;

	dispMutex.lock();

	uiCtrl.Clients->DeleteString(currentIndex);
	clientsDisplay.erase(client.SocketFD);

	//Decrement the indexes of all entires after this socket by 1, since this socket will be removed from the list.
	for (auto& iter : clientsDisplay)
	{
		if (iter.second.first >= currentIndex)
			iter.second.first -= 1;
	}

	dispMutex.unlock();
}

void TCPServerApp::UpdateDialogClient(const NetSocket::SocketInfo& client, const wchar_t* newDisplayName)
{
	if (newDisplayName != nullptr)
	{
		int index = clientsDisplay[client.SocketFD].first;

		dispMutex.lock();
		uiCtrl.Clients->DeleteString(index);

		std::wstring displayStr;
		displayStr += newDisplayName;
		displayStr += L"\t\t";
		displayStr += CA2CT(client.IPAddress);
		displayStr += L"\t";
		displayStr += CA2CT(client.HostName);

		uiCtrl.Clients->InsertString(index, displayStr.c_str());
		dispMutex.unlock();
	}
}

void TCPServerApp::PrintDialogMessage(const wchar_t* msg)
{
	uiCtrl.Messages->AddString(msg);
	m_pMainWnd->UpdateWindow();
}

void TCPServerApp::LaunchAcceptingThread()
{
	if (!threadMasterFlag)
		threadMasterFlag = true;

	acceptThread = std::thread(&TCPServerApp::AcceptIncomingClients, this, this);
}

void TCPServerApp::CloseAllThreads() noexcept
{
	threadMasterFlag = false;

	if (acceptThread.joinable())
		acceptThread.join();

	for (auto& iter : clientThreads)
	{
		if (iter.second.joinable())
			iter.second.join();
	}

	clientThreads.clear();
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

inline void TCPServerApp::AcceptIncomingClients(TCPServerApp* appInst)
{
	while (threadMasterFlag)
	{
		if (serverSocket.AcceptIncomingClientTCP())
		{
			NetSocket::SocketInfo newClient = serverSocket.GetRemoteSocketInfo().back();
			bool isDuplicate = false;
			
			//Delete any thread objects that aren't running.
			for (const auto& iter : clientThreads)
			{
				if (!iter.second.joinable())
				{
					clientThreadMutex.lock();
					clientThreads.erase(iter.first);
					clientThreadMutex.unlock();
				}
			}

			//Drop client if the IP and HostName already exists
			for (auto& socket : serverSocket.GetRemoteSocketInfo())
			{
				if ((strcmp(newClient.IPAddress, socket.IPAddress) == 0) && 
					(strcmp(newClient.HostName, socket.HostName) == 0) &&
					(newClient.SocketFD != socket.SocketFD))
				{
					serverSocket.DropClientConnectionTCP(newClient.SocketFD);
					isDuplicate = true;
					break;
				}
			}

			if (!isDuplicate)
			{
				if (clientThreads.size() < MAX_NUM_CLIENTS)
				{
					AddDialogClient(newClient, nullptr);
					clientThreads.insert({ newClient.SocketFD, std::thread(&TCPServerApp::ProcessClientMessages, appInst, newClient) });
				}
				else
					serverSocket.DropClientConnectionTCP(newClient.SocketFD);
			}
		}

		if (serverSocket.GetThisSocketInfo().Blocking == 1)
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

inline void TCPServerApp::ProcessClientMessages(NetSocket::SocketInfo client)
{
	std::wstring receivedMsg;

	while (threadMasterFlag)
	{
		int receivedBytes = serverSocket.ReceiveFromClientTCP(client, receivedMsg);
		
		if (receivedBytes > 0)
		{
			PrintDialogMessage(receivedMsg.c_str());

			//Determine the type of message received
			if (receivedMsg.find(PREFIX_DISPNAME) != std::wstring::npos)
			{
				std::wstring dispName = receivedMsg.substr(wcslen(PREFIX_DISPNAME), receivedMsg.length() - wcslen(PREFIX_DISPNAME));
				UpdateDialogClient(client, dispName.c_str());
			}
			else if ((receivedMsg.find(PREFIX_SENDER) != std::wstring::npos) && (receivedMsg.find(PREFIX_RECEIVER) != std::wstring::npos))
			{
				std::string senderIP, receiverIP;
				std::wstring message;

				size_t start = 0, end = 0;
				std::vector<std::wstring> splitStrings;

				while ((start = receivedMsg.find_first_not_of(DELIM, end)) != std::wstring::npos)
				{
					end = receivedMsg.find(DELIM, start);
					splitStrings.emplace_back(receivedMsg.substr(start, end - start));
					PrintDialogMessage(splitStrings.back().c_str());
				}

				receiverIP = CT2CA(splitStrings[1].substr(wcslen(PREFIX_RECEIVER), splitStrings[1].length() - wcslen(PREFIX_RECEIVER)).c_str());

				for (auto& socket : serverSocket.GetRemoteSocketInfo())
				{
					if (strcmp(receiverIP.c_str(), socket.IPAddress) == 0)
					{
						//Relay the message to the intended recipient.
						serverSocket.SendToClientTCP(socket.SocketFD, receivedMsg.c_str(), (receivedMsg.length() + 1) * sizeof(receivedMsg[0]));
						break;
					}
				}
			}
		}
		else if (receivedBytes == 0)
			break;
		else
		{
			if (serverSocket.GetThisSocketInfo().Blocking == 1)
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	//Client has disconnected.
	clientThreadMutex.lock();
	RemoveDialogClient(client);
	serverSocket.DropClientConnectionTCP(client.SocketFD);
	clientThreadMutex.unlock();
}


