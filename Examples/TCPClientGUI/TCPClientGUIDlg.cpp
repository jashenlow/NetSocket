
// TCPClientGUIDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "TCPClientGUI.h"
#include "TCPClientGUIDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CTCPClientGUIDlg dialog


CTCPClientGUIDlg::CTCPClientGUIDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_TCPCLIENTGUI_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTCPClientGUIDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CTCPClientGUIDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(BTN_CONNECT, &CTCPClientGUIDlg::OnBnClickedConnect)
	ON_BN_CLICKED(BTN_DISCONNECT, &CTCPClientGUIDlg::OnBnClickedDisconnect)
	ON_BN_CLICKED(BTN_SEND, &CTCPClientGUIDlg::OnBnClickedSend)
	ON_MESSAGE(WM_APPEND_MESSAGE, &CTCPClientGUIDlg::OnAppendMessage)
	ON_MESSAGE(WM_CLEAR_CLIENTS, &CTCPClientGUIDlg::OnClearClients)
	ON_MESSAGE(WM_ADD_CLIENT, &CTCPClientGUIDlg::OnAddClient)
	ON_MESSAGE(WM_REMOVE_CLIENT, &CTCPClientGUIDlg::OnRemoveClient)
	ON_MESSAGE(WM_SERVER_DISCONNECT, &CTCPClientGUIDlg::OnServerDisconnect)
END_MESSAGE_MAP()


// CTCPClientGUIDlg message handlers

BOOL CTCPClientGUIDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	ModifyStyle(0, WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX, TRUE);
	SetDefID(IDIGNORE);	//This prevents dialog from closing when user presses Enter.
	
	if (p_App == nullptr)
		p_App = (TCPClientApp*)AfxGetApp();

	if (p_App != nullptr)
	{
		SetUIControls();
		InitUIControls();
	}

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CTCPClientGUIDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CTCPClientGUIDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CTCPClientGUIDlg::OnCancel()
{	
	if (p_App->GetSocket()->IsInitialized())
	{
		OnAppendMessage((WPARAM)L"Shutting down client...", NULL);

		p_App->GetSocket()->CloseThisSocket();
		p_App->SetConnected(false);

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	PostQuitMessage(0);
}

LRESULT CTCPClientGUIDlg::OnAppendMessage(WPARAM wParam, LPARAM lParam)
{
	if (ui.Messages != nullptr)
	{
		int index = ui.Messages->AddString((LPCWSTR)wParam);
		ui.Messages->SetTopIndex(index);	//To ensure the CListBox auto-scrolls to the bottom.
	}

	return 1;
}

LRESULT CTCPClientGUIDlg::OnClearClients(WPARAM wParam, LPARAM lParam)
{
	clients.clear();
	ui.Clients->ResetContent();

	return 1;
}

LRESULT CTCPClientGUIDlg::OnAddClient(WPARAM wParam, LPARAM lParam)
{
	if (ui.Clients != nullptr)
	{
		const wchar_t* clientString = (LPCWSTR)wParam;

		std::vector<std::wstring> splitStrings;
		TokenizeReceivedString(clientString, DELIM, splitStrings);
		
		std::wstring displayStr;
		ConstructClientDisplayString(splitStrings[1].c_str(), splitStrings[2].c_str(), splitStrings[3].c_str(), displayStr);

		int index = ui.Clients->AddString(displayStr.c_str());
		ui.Messages->SetTopIndex(index);	//To ensure the CListBox auto-scrolls to the bottom.
		clients.insert({ splitStrings[2], index });
	}

	return 1;
}

LRESULT CTCPClientGUIDlg::OnRemoveClient(WPARAM wParam, LPARAM lParam)
{
	if (ui.Clients != nullptr)
	{
		LPCWSTR ipAddress = (LPCWSTR)wParam;

		int index = -1;

		for (auto& c : clients)
		{
			if (wcscmp(c.first.c_str(), ipAddress) == 0)
				index = c.second;
		}

		if (index != -1)
		{
			ui.Clients->DeleteString(index);
			clients.erase(ipAddress);

			//Decrement the indexes of all entires after this socket by 1, since this socket was removed from the list.
			for (auto& iter : clients)
			{
				if (iter.second >= index)
					iter.second -= 1;
			}
		}
	}

	return 1;
}

LRESULT CTCPClientGUIDlg::OnServerDisconnect(WPARAM wParam, LPARAM lParam)
{
	OnBnClickedDisconnect();

	return 1;
}

inline void CTCPClientGUIDlg::SetUIControls()
{
	ui.LocalIP		= (CIPAddressCtrl*)GetDlgItem(EC_LOCALIPADDRESS);
	ui.ServerIP		= (CIPAddressCtrl*)GetDlgItem(EC_SERVERIPADDRESS);
	ui.DisplayName	= (CEdit*)GetDlgItem(EC_DISPLAYNAME);
	ui.Connect		= (CButton*)GetDlgItem(BTN_CONNECT);
	ui.Disconnect	= (CButton*)GetDlgItem(BTN_DISCONNECT);
	ui.Clients		= (CListBox*)GetDlgItem(LB_CLIENTS);
	ui.Messages		= (CListBox*)GetDlgItem(LB_MESSAGES);
	ui.SendMsg		= (CEdit*)GetDlgItem(EC_SENDMESSAGE);
	ui.Send			= (CButton*)GetDlgItem(BTN_SEND);
}

inline void CTCPClientGUIDlg::InitUIControls()
{
	ui.LocalIP->SetWindowText(L"0.0.0.0");
	ui.ServerIP->SetWindowText(L"0.0.0.0");
	ui.Connect->EnableWindow(true);
	ui.Disconnect->EnableWindow(false);
}

inline void CTCPClientGUIDlg::CStringToStdString(const CString cStr, std::string& stdStr)
{
	CT2CA conv(cStr);
	stdStr = conv;
}

inline void CTCPClientGUIDlg::StdStringToCString(const std::string& stdStr, CString& cStr)
{
	CA2CT conv(stdStr.c_str());
	cStr = conv;
}

inline void CTCPClientGUIDlg::ConstructClientDisplayString(const wchar_t* name, const wchar_t* ipAddress, const wchar_t* hostName, std::wstring& outString)
{
	outString = name;
	outString += L"\t\t";
	outString += ipAddress;
	outString += L'\t';
	outString += hostName;
}

inline void CTCPClientGUIDlg::ConstructSenderReceiverString(const wchar_t* senderIP, const wchar_t* receiverIP, const wchar_t* msg, std::wstring& outString)
{
	outString = PREFIX_SENDER;
	outString += senderIP;
	outString += DELIM;
	outString += PREFIX_RECEIVER;
	outString += receiverIP;
	outString += DELIM;
	outString += msg;
}

inline void CTCPClientGUIDlg::TokenizeReceivedString(const std::wstring& string, const wchar_t& delim, std::vector<std::wstring>& tokens)
{
	size_t start = 0, end = 0;

	while ((start = string.find_first_not_of(delim, end)) != std::wstring::npos)
	{
		end = string.find(DELIM, start);
		tokens.emplace_back(string.substr(start, end - start));
	}
}

void CTCPClientGUIDlg::ConstructSenderReceiverDisplay(LPCWSTR sender, LPCWSTR receiver, LPCWSTR msg, CString& outString)
{
	outString = L"[";
	outString += sender;
	outString += L"] --> [";
	outString += receiver;
	outString += L"]: ";
	outString += msg;
}

void CTCPClientGUIDlg::OnBnClickedConnect()
{
	NetSocket* socket = p_App->GetSocket();

	if (socket->IsInitialized())
		socket->CloseThisSocket();

	//Get IP Addresses from UI controls
	CString localIPCStr, serverIPCStr, dispNameCStr;
	std::string localIPStr, serverIPStr;

	ui.LocalIP->GetWindowText(localIPCStr);
	ui.ServerIP->GetWindowText(serverIPCStr);
	ui.DisplayName->GetWindowText(dispNameCStr);
	CStringToStdString(localIPCStr, localIPStr);
	CStringToStdString(serverIPCStr, serverIPStr);

	OnAppendMessage((WPARAM)L"Initializing socket...", NULL);

	WinsockError initErr = socket->Init(localIPStr.c_str(), SERVER_PORT, SocketType::TCP_CLIENT, true);

	if (initErr == WinsockError::OK)
	{
		int errConnect = socket->ConnectToServerTCP(serverIPStr.c_str(), SERVER_PORT);

		if (errConnect == NonBlockIncomplete)	//For non-blocking call
		{
			int errWaitConnect = socket->CheckForConnectNonBlockTCP(NONBLOCK_TIMEOUT);

			switch (errWaitConnect)
			{
				case 1:
					OnAppendMessage((WPARAM)L"Connected to server!", NULL);
					p_App->SetConnected(true);
					break;
				case 0:
					OnAppendMessage((WPARAM)L"Connection to server timeout expired.", NULL);
					break;
				case SOCKET_ERROR:
					OnAppendMessage((WPARAM)L"Failed to connect to server.", NULL);
					break;
			}
		}
		else if (errConnect == 0)	//For blocking call
		{
			p_App->SetConnected(true);
			OnAppendMessage((WPARAM)L"Connected to server!", NULL);
		}
		else
			OnAppendMessage((WPARAM)L"Failed to connect to server.", NULL);

		OnAppendMessage((WPARAM)L"----------------------------------------------------------------", NULL);

		if (p_App->IsConnected())
		{
			ui.Connect->EnableWindow(false);
			ui.Disconnect->EnableWindow(true);

			std::wstring setDispNameCmd = PREFIX_DISPNAME;
			setDispNameCmd += dispNameCStr.GetString();
			socket->SendToServerTCP(setDispNameCmd.c_str(), NetUtil::GetStringSizeBytes(setDispNameCmd));
			
			p_App->StartNetworkThread();
		}
	}
	else
		OnAppendMessage((WPARAM)(L"Socket initialization failed with error code " + std::to_wstring(GetSocketError)).c_str(), NULL);
}


void CTCPClientGUIDlg::OnBnClickedDisconnect()
{
	NetSocket* socket = p_App->GetSocket();

	if (socket->IsInitialized())
	{
		p_App->CloseNetworkThread();
		p_App->SetConnected(false);
		socket->CloseThisSocket();

		ui.Connect->EnableWindow(true);
		ui.Disconnect->EnableWindow(false);
		OnAppendMessage((WPARAM)L"Disonnected from server!", NULL);
		OnAppendMessage((WPARAM)L"----------------------------------------------------------------", NULL);
	}
}


void CTCPClientGUIDlg::OnBnClickedSend()
{
	CString msgTextCStr;
	NetSocket* socket = p_App->GetSocket();
	
	//Get receiver's IP
	int selectedIndex = ui.Clients->GetCurSel();
	std::wstring receiverIP;

	for (auto& c : clients)
	{
		if (c.second == selectedIndex)
			receiverIP = c.first;
	}

	if (!receiverIP.empty())
	{
		ui.SendMsg->GetWindowText(msgTextCStr);
		std::wstring msgText(msgTextCStr.GetString());
		std::wstring senderIP = CA2CT(socket->GetThisSocketInfo().IPAddress);

		std::wstring sendMsg;
		ConstructSenderReceiverString(senderIP.c_str(), receiverIP.c_str(), msgText.c_str(), sendMsg);

		if (socket->SendToServerTCP(sendMsg.c_str(), NetUtil::GetStringSizeBytes(sendMsg)))
		{
			CString myDispNameCStr;
			ui.DisplayName->GetWindowText(myDispNameCStr);
			
			CString receiverDispCStr;
			ui.Clients->GetText(selectedIndex, receiverDispCStr);
			std::wstring receiverDisp(receiverDispCStr.GetString());
			std::wstring receiverName = receiverDisp.substr(0, receiverDisp.find(L'\t'));
			receiverDispCStr = receiverName.c_str();

			CString dispString;
			ConstructSenderReceiverDisplay(myDispNameCStr.GetString(), receiverDispCStr.GetString(), msgTextCStr.GetString(), dispString);
			OnAppendMessage((WPARAM)dispString.GetString(), NULL);
		}
	}
}
