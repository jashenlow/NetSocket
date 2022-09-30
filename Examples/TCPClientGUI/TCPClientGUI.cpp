
// TCPClientGUI.cpp : Defines the class behaviors for the application.
//

#include "pch.h"
#include "framework.h"
#include "TCPClientGUI.h"
#include "TCPClientGUIDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// TCPClientApp

BEGIN_MESSAGE_MAP(TCPClientApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// TCPClientApp construction

TCPClientApp::TCPClientApp()
{
	// support Restart Manager
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

NetSocket* TCPClientApp::GetSocket()
{
	return &clientSocket;
}

void TCPClientApp::StartNetworkThread()
{
	threadRunFlag = true;
	networkThread = std::thread(&TCPClientApp::ProcessNetworkMessages, this, std::ref(threadRunFlag));
}

void TCPClientApp::CloseNetworkThread()
{
	if (threadRunFlag)
	{
		threadRunFlag = false;
		networkThread.join();
	}
}

void TCPClientApp::SetConnected(const bool& flag)
{
	connectedToServer = flag;
}

bool TCPClientApp::IsConnected()
{
	return connectedToServer;
}

void TCPClientApp::ConstructSenderReceiverDisplay(const wchar_t* sender, const wchar_t* receiver, const wchar_t* msg, CString& outString)
{
	outString = L"[";
	outString += sender;
	outString += L"] --> [";
	outString += receiver;
	outString += L"]: ";
	outString += msg;
}


// The one and only TCPClientApp object

TCPClientApp theApp;


// TCPClientApp initialization

BOOL TCPClientApp::InitInstance()
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

	CTCPClientGUIDlg dlg;
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

inline void TCPClientApp::ProcessNetworkMessages(std::atomic<bool>& run)
{
	std::wstring receivedMsg;
	CTCPClientGUIDlg* p_Dialog = (CTCPClientGUIDlg*)m_pMainWnd;
	bool isNonBlocking = (clientSocket.GetThisSocketInfo().NonBlocking == 1);

	while (run)
	{
		int receivedBytes = clientSocket.ReceiveFromServerTCP(receivedMsg);

		if (receivedBytes > 0)
		{
			if (receivedMsg.find(PREFIX_SERVER_DOWN) != std::wstring::npos)
			{
				//Server has disconnected.
				PostMessage(m_pMainWnd->m_hWnd, WM_SERVER_DISCONNECT, NULL, NULL);
				break;
			}
			else if (receivedMsg.find(PREFIX_ADD_CLIENT) != std::wstring::npos)
			{
				//New client has been added
				CString newClient(receivedMsg.c_str());
				PostMessage(m_pMainWnd->m_hWnd, WM_ADD_CLIENT, (WPARAM)newClient.GetString(), NULL);
			}
			else if (receivedMsg.find(PREFIX_REMOVE_CLIENT) != std::wstring::npos)
			{
				//A client has been disconnected
				size_t prefixLen = wcslen(PREFIX_REMOVE_CLIENT);

				CString remClient(receivedMsg.substr(prefixLen, receivedMsg.length() - prefixLen).c_str());
				PostMessage(m_pMainWnd->m_hWnd, WM_REMOVE_CLIENT, (WPARAM)remClient.GetString(), NULL);
			}
			else if ((receivedMsg.find(PREFIX_SENDER) != std::wstring::npos) && (receivedMsg.find(PREFIX_RECEIVER) != std::wstring::npos))
			{
				PostMessage(m_pMainWnd->m_hWnd, WM_MSG_FROM_CLIENT, (WPARAM)receivedMsg.c_str(), NULL);
			}
			
		}
		else if (receivedBytes == SOCKET_ERROR)
		{
			if (isNonBlocking)
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		else
		{
			PostMessage(m_pMainWnd->m_hWnd, WM_SERVER_DISCONNECT, NULL, NULL);
			break;
		}
	}
}



