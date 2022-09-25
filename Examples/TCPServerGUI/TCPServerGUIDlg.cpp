
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

inline void CTCPServerGUIDlg::PrintMessage(const wchar_t* msg)
{
	p_App->UIControls()->Messages->AddString(msg);
	UpdateWindow();
}

void CTCPServerGUIDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CTCPServerGUIDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(BTN_STARTSERVER, &CTCPServerGUIDlg::OnBnClickedStartserver)
	ON_BN_CLICKED(BTN_STOPSERVER, &CTCPServerGUIDlg::OnBnClickedStopserver)
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
		PrintMessage(L"Shutting down server...");

		p_App->GetSocket()->CloseThisSocket();
		p_App->GetSocket()->DropAllClients();
		p_App->CloseAllThreads();

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	PostQuitMessage(0);
}

inline void CTCPServerGUIDlg::SetUIControls()
{
	p_App->UIControls()->IPAddress		= (CIPAddressCtrl*)GetDlgItem(EC_IPADDRESS);
	p_App->UIControls()->Clients		= (CListBox*)GetDlgItem(LB_CLIENTS);
	p_App->UIControls()->Messages		= (CListBox*)GetDlgItem(LB_MESSAGES);
	p_App->UIControls()->StartServer	= (CButton*)GetDlgItem(BTN_STARTSERVER);
	p_App->UIControls()->StopServer		= (CButton*)GetDlgItem(BTN_STOPSERVER);
}

inline void CTCPServerGUIDlg::InitUIControls()
{
	p_App->UIControls()->IPAddress->SetWindowText(L"0.0.0.0");
	p_App->UIControls()->StopServer->EnableWindow(false);
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



void CTCPServerGUIDlg::OnBnClickedStartserver()
{
	NetSocket* socket = p_App->GetSocket();

	if (socket->IsInitialized())
		socket->CloseThisSocket();

	//Get IP Address and port from UI controls
	CString ipAddrCStr, portCStr;
	std::string ipAddrStr;

	p_App->UIControls()->IPAddress->GetWindowText(ipAddrCStr);
	CStringToStdString(ipAddrCStr, ipAddrStr);

	PrintMessage(L"Initializing server socket...");

	WinsockError initErr = socket->Init(ipAddrStr.c_str(), SERVER_PORT, SocketType::TCP_SERVER/*, true*/);

	if (initErr == WinsockError::OK)
	{
		PrintMessage(L"Server socket initialized successfully.");
		PrintMessage(L"----------------------------------------------------------------");
		socket->StartListeningTCP(MAX_PENDING_CLIENTS);
		p_App->LaunchAcceptingThread();

		PrintMessage(L"Listening for connections...");
		p_App->UIControls()->StartServer->EnableWindow(false);
		p_App->UIControls()->StopServer->EnableWindow(true);
	}
	else
		PrintMessage((L"Socket initialization failed with error code " + std::to_wstring(GetSocketError)).c_str());
}


void CTCPServerGUIDlg::OnBnClickedStopserver()
{
	NetSocket* socket = p_App->GetSocket();

	PrintMessage(L"Shutting down server...");

	if (socket->IsInitialized())
	{	
		if (socket->CloseThisSocket())
		{
			socket->DropAllClients();
			p_App->CloseAllThreads();
			
			PrintMessage(L"Server shut down successfully.");
			PrintMessage(L"----------------------------------------------------------------");

			p_App->UIControls()->StopServer->EnableWindow(false);
			p_App->UIControls()->StartServer->EnableWindow(true);
		}
		else
			PrintMessage(L"Error: Unable to close server socket.");
	}
}
