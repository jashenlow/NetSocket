
// TCPClientGUIDlg.h : header file
//

#pragma once

struct UIControls
{
	CIPAddressCtrl* LocalIP		= nullptr;
	CIPAddressCtrl* ServerIP	= nullptr;
	CEdit*			DisplayName = nullptr;
	CListBox*		Clients		= nullptr;
	CListBox*		Messages	= nullptr;
	CEdit*			SendMsg		= nullptr;
	CButton*		Connect		= nullptr;
	CButton*		Disconnect	= nullptr;
	CButton*		Send		= nullptr;
};

// CTCPClientGUIDlg dialog
class CTCPClientGUIDlg : public CDialogEx
{
// Construction
public:
	CTCPClientGUIDlg(CWnd* pParent = nullptr);	// standard constructor


// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TCPClientGUI_DIALOG };
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
	DECLARE_MESSAGE_MAP()

protected:
	//Custom message handlers
	afx_msg LRESULT OnAppendMessage(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnClearClients(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAddClient(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnRemoveClient(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnServerDisconnect(WPARAM wParam, LPARAM lParam);

private:
	TCPClientApp* p_App = nullptr;
	UIControls ui;
	std::unordered_map<std::wstring, int> clients;	//<IPAdress, index>

	inline void SetUIControls();
	inline void InitUIControls();
	inline void CStringToStdString(const CString cStr, std::string& stdStr);
	inline void StdStringToCString(const std::string& stdStr, CString& cStr);
	inline void ConstructClientDisplayString(const wchar_t* name, const wchar_t* ipAddress, const wchar_t* hostName, std::wstring& outString);
	inline void ConstructSenderReceiverString(const wchar_t* senderIP, const wchar_t* receiverIP, const wchar_t* msg, std::wstring& outString);
	inline void TokenizeReceivedString(const std::wstring& string, const wchar_t& delim, std::vector<std::wstring>& tokens);
	void ConstructSenderReceiverDisplay(LPCWSTR sender, LPCWSTR receiver, LPCWSTR msg, CString& outString);

public:
	afx_msg void OnBnClickedConnect();
	afx_msg void OnBnClickedDisconnect();
	afx_msg void OnBnClickedSend();
};
