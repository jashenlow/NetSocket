
// TCPServerGUIDlg.h : header file
//

#pragma once


// CTCPServerGUIDlg dialog
class CTCPServerGUIDlg : public CDialogEx
{
// Construction
public:
	CTCPServerGUIDlg(CWnd* pParent = nullptr);	// standard constructor

	inline void PrintMessage(const wchar_t* msg);

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
	DECLARE_MESSAGE_MAP()
private:
	TCPServerApp* p_App = nullptr;

	inline void SetUIControls();
	inline void InitUIControls();
	inline void CStringToStdString(const CString cStr, std::string& stdStr);
	inline void StdStringToCString(const std::string& stdStr, CString& cStr);

public:
	afx_msg void OnBnClickedStartserver();
	afx_msg void OnBnClickedStopserver();
};
