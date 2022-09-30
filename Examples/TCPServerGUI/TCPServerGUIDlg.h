
// TCPServerGUIDlg.h : header file
//

#pragma once

struct UIControls
{
	CIPAddressCtrl* IPAddress	= nullptr;
	CListBox*		Clients		= nullptr;
	CListBox*		Messages	= nullptr;
	CButton*		StartServer = nullptr;
	CButton*		StopServer	= nullptr;
};

// CTCPServerGUIDlg dialog
class CTCPServerGUIDlg : public CDialogEx
{
// Construction
public:
	CTCPServerGUIDlg(CWnd* pParent = nullptr);	// standard constructor


// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TCPSERVERGUI_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnCancel();
	afx_msg void OnOK();
	DECLARE_MESSAGE_MAP()

protected:
	//Custom message handlers
	afx_msg LRESULT OnAppendMessage(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnClearClients(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAddClient(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnRemoveClient(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnUpdateClient(WPARAM wParam, LPARAM lParam);

private:
	TCPServerApp* p_App = nullptr;
	UIControls ui;
	std::unordered_map<std::wstring ,int> clients;	//<IPAdress, index>

	inline void SetUIControls();
	inline void InitUIControls();
	inline void CStringToStdString(const CString cStr, std::string& stdStr);
	inline void StdStringToCString(const std::string& stdStr, CString& cStr);
	inline void ConstructClientDisplayString(const wchar_t* name, const wchar_t* ipAddress, const wchar_t* hostName, std::wstring& outString);

public:
	afx_msg void OnBnClickedStartserver();
	afx_msg void OnBnClickedStopserver();
};
