
// TCPServerGUIDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "TCPServerGUI.h"
#include "TCPServerGUIDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CTCPServerGUIDlg dialog


CTCPServerGUIDlg::CTCPServerGUIDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_TCPSERVERGUI_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTCPServerGUIDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CTCPServerGUIDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CTCPServerGUIDlg::OnOK)
	ON_BN_CLICKED(BTN_STARTSERVER, &CTCPServerGUIDlg::OnBnClickedStartserver)
	ON_BN_CLICKED(BTN_STOPSERVER, &CTCPServerGUIDlg::OnBnClickedStopserver)
	ON_MESSAGE(WM_APPEND_MESSAGE, &CTCPServerGUIDlg::OnAppendMessage)
	ON_MESSAGE(WM_CLEAR_CLIENTS, &CTCPServerGUIDlg::OnClearClients)
	ON_MESSAGE(WM_ADD_CLIENT, &CTCPServerGUIDlg::OnAddClient)
	ON_MESSAGE(WM_REMOVE_CLIENT, &CTCPServerGUIDlg::OnRemoveClient)
	ON_MESSAGE(WM_UPDATE_CLIENT, &CTCPServerGUIDlg::OnUpdateClient)
END_MESSAGE_MAP()


// CTCPServerGUIDlg message handlers

BOOL CTCPServerGUIDlg::OnInitDialog()
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
		p_App = (TCPServerApp*)AfxGetApp();

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

void CTCPServerGUIDlg::OnPaint()
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
HCURSOR CTCPServerGUIDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CTCPServerGUIDlg::OnCancel()
{	
	if (p_App->GetSocket()->IsInitialized())
	{
		OnAppendMessage((WPARAM)L"Shutting down server...", NULL);

		p_App->CloseAllThreads();
		p_App->GetSocket()->CloseThisSocket();
		p_App->GetSocket()->DropAllClients();
		OnClearClients(NULL, NULL);
		
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	PostQuitMessage(0);
}

void CTCPServerGUIDlg::OnOK()
{
	//Do nothing
}

LRESULT CTCPServerGUIDlg::OnAppendMessage(WPARAM wParam, LPARAM lParam)
{
	if (ui.Messages != nullptr)
	{
		int index = ui.Messages->AddString((LPCWSTR)wParam);
		ui.Messages->SetTopIndex(index);	//To ensure the CListBox auto-scrolls to the bottom.
	}

	return 1;
}

LRESULT CTCPServerGUIDlg::OnClearClients(WPARAM wParam, LPARAM lParam)
{
	clients.clear();
	ui.Clients->ResetContent();

	return 1;
}

LRESULT CTCPServerGUIDlg::OnAddClient(WPARAM wParam, LPARAM lParam)
{
	if (ui.Clients != nullptr)
	{
		NetSocket::SocketInfo* client = (NetSocket::SocketInfo*)wParam;
		const wchar_t* name			= (LPCWSTR)lParam;
		const wchar_t* ipAddress	= CA2CT(client->IPAddress);
		const wchar_t* hostName		= CA2CT(client->HostName);

		std::wstring displayStr;
		const wchar_t* newName = ((name == nullptr) || (name[0] == '\0')) ? L"(empty)" : name;
		ConstructClientDisplayString(newName, ipAddress, hostName, displayStr);

		int index = ui.Clients->AddString(displayStr.c_str());
		ui.Messages->SetTopIndex(index);	//To ensure the CListBox auto-scrolls to the bottom.
		clients.insert({ ipAddress, index });
	}

	return 1;
}

LRESULT CTCPServerGUIDlg::OnRemoveClient(WPARAM wParam, LPARAM lParam)
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
	
	return 1;
}

LRESULT CTCPServerGUIDlg::OnUpdateClient(WPARAM wParam, LPARAM lParam)
{
	NetSocket::SocketInfo* client = (NetSocket::SocketInfo*)wParam;
	const wchar_t* name			= (LPCWSTR)lParam;
	const wchar_t* ipAddress	= CA2CT(client->IPAddress);
	const wchar_t* hostName		= CA2CT(client->HostName);

	std::wstring displayStr;
	const wchar_t* newName = ((name == nullptr) || (name[0] == '\0')) ? L"(empty)" : name;
	ConstructClientDisplayString(newName, ipAddress, hostName, displayStr);

	int index = clients[ipAddress];
	ui.Clients->DeleteString(index);
	ui.Clients->InsertString(index, displayStr.c_str());

	return 1;
}

inline void CTCPServerGUIDlg::SetUIControls()
{
	ui.IPAddress	= (CIPAddressCtrl*)GetDlgItem(EC_IPADDRESS);
	ui.Clients		= (CListBox*)GetDlgItem(LB_CLIENTS);
	ui.Messages		= (CListBox*)GetDlgItem(LB_MESSAGES);
	ui.StartServer	= (CButton*)GetDlgItem(BTN_STARTSERVER);
	ui.StopServer	= (CButton*)GetDlgItem(BTN_STOPSERVER);
}

inline void CTCPServerGUIDlg::InitUIControls()
{
	ui.IPAddress->SetWindowText(L"0.0.0.0");
	ui.StopServer->EnableWindow(false);
	
}

inline void CTCPServerGUIDlg::CStringToStdString(const CString cStr, std::string& stdStr)
{
	CT2CA conv(cStr);
	stdStr = conv;
}

inline void CTCPServerGUIDlg::StdStringToCString(const std::string& stdStr, CString& cStr)
{
	CA2CT conv(stdStr.c_str());
	cStr = conv;
}

inline void CTCPServerGUIDlg::ConstructClientDisplayString(const wchar_t* name, const wchar_t* ipAddress, const wchar_t* hostName, std::wstring& outString)
{
	outString = name;
	outString += L"\t\t";
	outString += ipAddress;
	outString += L'\t';
	outString += hostName;
}


void CTCPServerGUIDlg::OnBnClickedStartserver()
{
	NetSocket* socket = p_App->GetSocket();

	if (socket->IsInitialized())
		socket->CloseThisSocket();

	//Get IP Address and port from UI controls
	CString ipAddrCStr, portCStr;
	std::string ipAddrStr;

	ui.IPAddress->GetWindowText(ipAddrCStr);
	CStringToStdString(ipAddrCStr, ipAddrStr);

	OnAppendMessage((WPARAM)L"Initializing server socket...", NULL);

	WinsockError initErr = socket->Init(ipAddrStr.c_str(), SERVER_PORT, SocketType::TCP_SERVER, true);

	if (initErr == WinsockError::OK)
	{
		OnAppendMessage((WPARAM)L"Server socket initialized successfully.", NULL);
		OnAppendMessage((WPARAM)L"----------------------------------------------------------------", NULL);
		socket->StartListeningTCP(MAX_PENDING_CLIENTS);
		p_App->LaunchAcceptingThread();

		OnAppendMessage((WPARAM)L"Listening for connections...", NULL);
		ui.StartServer->EnableWindow(false);
		ui.StopServer->EnableWindow(true);
	}
	else
		OnAppendMessage((WPARAM)(L"Socket initialization failed with error code " + std::to_wstring(GetSocketError)).c_str(), NULL);
}


void CTCPServerGUIDlg::OnBnClickedStopserver()
{
	NetSocket* socket = p_App->GetSocket();

	OnAppendMessage((WPARAM)L"Shutting down server...", NULL);

	if (socket->IsInitialized())
	{	
		//Send disconnect message to all clients
		std::wstring disConnStr(PREFIX_SERVER_DOWN);
		for (auto& client : socket->GetRemoteSocketInfo())
			socket->SendToClientTCP(client.SocketFD, PREFIX_SERVER_DOWN, NetUtil::GetStringSizeBytes(disConnStr));

		p_App->CloseAllThreads();

		if (socket->CloseThisSocket())
		{		
			socket->DropAllClients();
			OnClearClients(NULL, NULL);

			OnAppendMessage((WPARAM)L"Server shut down successfully.", NULL);
			OnAppendMessage((WPARAM)L"----------------------------------------------------------------", NULL);
			
			socket->CloseThisSocket();
			ui.StopServer->EnableWindow(false);
			ui.StartServer->EnableWindow(true);
		}
		else
			OnAppendMessage((WPARAM)L"Error: Unable to close server socket.", NULL);
	}
}
