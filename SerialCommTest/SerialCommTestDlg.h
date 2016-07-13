
// SerialCommTestDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include "tchart2.h"
// CSerialCommTestDlg 对话框
class CSerialCommTestDlg : public CDialogEx
{
// 构造
public:
	CSerialCommTestDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SERIALCOMMTEST_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOpenComm();
	CListBox m_listRead;
	CButton m_btnOpenComm;
	afx_msg void OnBnClickedClearRead();
	CEdit m_editRead;

	void WriteComm(unsigned char *buff, unsigned int writebytes);
	CEdit m_editWrite;
	CString m_strWrite;
	CButton m_btnSend;
	afx_msg void OnBnClickedSend();
protected:
	afx_msg LRESULT OnShowreceive(WPARAM wParam, LPARAM lParam);
public:
	CString m_strRead;
	void FindComPort();
	void InitComPortDialog();
	CComboBox m_Combo_baud;
	CComboBox m_Combo_comm;
	CListBox m_List_databits;
	CComboBox m_Combo_databits;
	CComboBox m_Combo_paritybits;
	CComboBox m_Combo_stopbits;
	CButton m_Chk_rhex;
	CButton m_Chk_thex;
	int String2Hex(CString srcstr, CByteArray *hexdata);
	char Hex2Char(char chh);
	afx_msg void OnBnClickedCheckReceivehex();
	void InitDataFlag();
	CTchart2 m_tchart;
	//UINT CheckCommand(LPVOID pParam);
	UINT CheckCommand(void);
		CStatic m_stRxLen;

private:
	CWinThread *m_pThreadCheckCommand;
public:
	int CommandProcess();
protected:
	afx_msg LRESULT OnCommandprocess(WPARAM wParam, LPARAM lParam);
};

#define WM_SHOWRECEIVE	(WM_USER + 100)
#define WM_COMMANDPROCESS	(WM_USER + 101)

