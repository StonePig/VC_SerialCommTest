
// SerialCommTestDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "SerialCommTest.h"
#include "SerialCommTestDlg.h"
#include "afxdialogex.h"
#include "CSeries.h"
#include "CTChart.h"
#include "CEnvironment.h"
#include "CAxes.h"
#include "CAxis.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define MODEID	0x16		// 模块ID，串口通讯部分
#define MAX_BUFFER_SIZE		2048

typedef struct DataFlag
{
	BOOL bIsCommOpen = false;	// 串口打开标志
	UINT uiBytesSent;		// 发送的总字节数
	UINT uiBytesReceived;	// 接收的总字节数
	CString strReceivedData;	// 接收的数据
};

OVERLAPPED osRead;
OVERLAPPED osShare;

BYTE byRxBuffer[MAX_BUFFER_SIZE];
DWORD RxLength = 0;
BYTE RxFlag = 0;

struct DataFlag st_DataFlag;
HANDLE    m_hComm;	// 串口句柄
UINT ReadComm(LPVOID pParam);

//UINT CheckCommand1(void);

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CSerialCommTestDlg 对话框



CSerialCommTestDlg::CSerialCommTestDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_SERIALCOMMTEST_DIALOG, pParent)
	, m_strWrite(_T(""))
	, m_strRead(_T(""))
	, m_pThreadCheckCommand(NULL)
	//, m_tchart(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CSerialCommTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_listRead);
	DDX_Control(pDX, IDC_BUTTON1, m_btnOpenComm);
	DDX_Control(pDX, IDC_EDIT2, m_editRead);
	DDX_Control(pDX, IDC_EDIT1, m_editWrite);
	DDX_Text(pDX, IDC_EDIT1, m_strWrite);
	DDX_Control(pDX, IDC_BUTTON3, m_btnSend);
	DDX_Text(pDX, IDC_EDIT2, m_strRead);
	DDX_Control(pDX, IDC_COMBO_BAUD, m_Combo_baud);
	DDX_Control(pDX, IDC_COMBO_COMM, m_Combo_comm);
	DDX_Control(pDX, IDC_COMBO_DATABITS, m_Combo_databits);
	DDX_Control(pDX, IDC_COMBO_PARITYBITS, m_Combo_paritybits);
	DDX_Control(pDX, IDC_COMBO_STOPBITS, m_Combo_stopbits);
	DDX_Control(pDX, IDC_CHECK_RECEIVEHEX, m_Chk_rhex);
	DDX_Control(pDX, IDC_CHECK_SENDHEX, m_Chk_thex);
	DDX_Control(pDX, IDC_TCHART2, m_tchart);
	DDX_Control(pDX, IDC_STATIC_RXLEN, m_stRxLen);
}

BEGIN_MESSAGE_MAP(CSerialCommTestDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CSerialCommTestDlg::OnBnClickedOpenComm)
	ON_BN_CLICKED(IDC_BUTTON2, &CSerialCommTestDlg::OnBnClickedClearRead)
	ON_BN_CLICKED(IDC_BUTTON3, &CSerialCommTestDlg::OnBnClickedSend)
	ON_MESSAGE(WM_SHOWRECEIVE, &CSerialCommTestDlg::OnShowreceive)
	ON_BN_CLICKED(IDC_CHECK_RECEIVEHEX, &CSerialCommTestDlg::OnBnClickedCheckReceivehex)
	ON_MESSAGE(WM_COMMANDPROCESS, &CSerialCommTestDlg::OnCommandprocess)
END_MESSAGE_MAP()


// CSerialCommTestDlg 消息处理程序

BOOL CSerialCommTestDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	// 设置窗体及控件的大小和位置
	this->SetWindowPos(NULL, 0, 0, 600, 700, SWP_NOMOVE);
	m_tchart.SetWindowPos(NULL, 40, 60, 500, 400, SWP_NOMOVE);
	// 禁止鼠标滚轮
	CEnvironment env = m_tchart.get_Environment();
	env.put_MouseWheelScroll(false);
	// 坐标轴初始化
	CAxes axes = m_tchart.get_Axis();
	CAxis leftAxis = (CAxis)axes.get_Left();
	CAxis bottomAxis = (CAxis)axes.get_Bottom();
	leftAxis.put_Visible(TRUE);
	leftAxis.put_Automatic(FALSE);
	leftAxis.put_Maximum(256);
	leftAxis.put_Minimum(0);
	leftAxis.put_Increment(10);
	bottomAxis.put_Automatic(FALSE);
	bottomAxis.put_Maximum(401);
	bottomAxis.put_Minimum(0);
	bottomAxis.put_Increment(1);


	InitDataFlag();	// 初始化变量
	FindComPort();	// 查找可用串口，添加到列表框
	InitComPortDialog();	// 初始化串口配置

	//// 查询串口接收数据
	//m_pThreadCheckCommand = AfxBeginThread(CheckCommand, this);// 开启下载线程
	//if (m_pThreadCheckCommand == NULL)
	//{
	//	// 线程启动失败
	//	//m_editStatus.SetWindowTextA("Fail to download!");
	//	AfxMessageBox("Fail to create check command thread!");
	//	//bIsDeviceReady = false;
	//	//m_btnOK.EnableWindow(true);
	//}

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CSerialCommTestDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CSerialCommTestDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CSerialCommTestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


// "打开串口"按钮
void CSerialCommTestDlg::OnBnClickedOpenComm()
{
	// TODO: 在此添加控件通知处理程序代码
	CString str;

	if (st_DataFlag.bIsCommOpen == FALSE)
	{
		m_Combo_comm.GetWindowTextA(str);
		if (str == "")
		{
			AfxMessageBox("没有串口！");
			return;
		}
		CString str1;
		str1.Format("\\\\.\\%s", str);	// 串口号大于10时，无法打开，需要改串口名
		m_hComm = CreateFile(str1,  //串口号
			GENERIC_READ | GENERIC_WRITE, //指定可以对串口进行读写操作
			0, //表示串口为独占打开
			NULL,// 权限控制，表示返回的句柄不能被子进程继承。
			OPEN_EXISTING, //表示当指定串口不存在时，程序将返回失败
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, //表示文件属性
			/*当打开串口时，必须指定 FILE_FLAG_OVERLAPPED（重叠方式），它表示文件或设备不会维护访问指针，则在读写时，必须使用OVERLAPPED 结构指定访问的文件偏移量。
			*/
			NULL);//临时文件的句柄，不使用。

		if (m_hComm == INVALID_HANDLE_VALUE)
		{
			AfxMessageBox("串口建立失败！");
			return;
		}

		/********************输入缓冲区和输出缓冲区的大小***********/
		SetupComm(m_hComm, 4096, 4096);
		PurgeComm(m_hComm, PURGE_TXCLEAR | PURGE_RXCLEAR);// 清空输入和输出缓冲区
		SetCommMask(m_hComm, EV_RXCHAR);// 设置通知事件。EV_RXCHAR: 输入缓冲区有数据时，通过WaitCommEvent函数可以获得通知

		DCB dcb;

		////设置超时时间
		//COMMTIMEOUTS TimeOuts;
		//TimeOuts.ReadIntervalTimeout = 0;
		//TimeOuts.ReadTotalTimeoutMultiplier = 0;
		//TimeOuts.ReadTotalTimeoutConstant = 50;
		//TimeOuts.WriteTotalTimeoutMultiplier = 0;
		//TimeOuts.WriteTotalTimeoutConstant = 0;
		//SetCommTimeouts(m_hComm, &TimeOuts);

		GetCommState(m_hComm, &dcb);//获得参数  

		m_Combo_baud.GetWindowTextA(str);
		dcb.BaudRate = atoi(str);

		m_Combo_databits.GetWindowTextA(str);
		dcb.ByteSize = atoi(str);
		
		m_Combo_stopbits.GetWindowTextA(str);
		if(str == "1")
			dcb.StopBits = ONESTOPBIT;//一个停止位 
		else if (str == "1.5")
			dcb.StopBits = ONE5STOPBITS;
		else if (str == "2")
			dcb.StopBits = TWOSTOPBITS;

		m_Combo_paritybits.GetWindowTextA(str);
		if (str == "无")
		{
			dcb.Parity = NOPARITY; //校验位
		}
		else if (str == "奇")
		{
			dcb.Parity = ODDPARITY; //校验位
		}
		else if (str == "偶")
		{
			dcb.Parity = EVENPARITY; //校验位
		}
		else if (str == "1")
		{
			dcb.Parity = MARKPARITY; //校验位
		}
		else if (str == "0")
		{
			dcb.Parity = SPACEPARITY; //校验位
		}

		dcb.fBinary = TRUE;// 指定是否允许二进制模式
		dcb.fParity = TRUE;// 指定是否允许奇偶校验

		//根据设备控制块配置通信设备
		if (!SetCommState(m_hComm, &dcb))
		{
			AfxMessageBox("串口设置出错！");
			CloseHandle(m_hComm);
			return;
		}

		m_btnOpenComm.SetWindowTextA("关闭串口");
		st_DataFlag.bIsCommOpen = TRUE;

		// 创建线程读取串口数据
		AfxBeginThread(ReadComm, this); //开读串口线程
	}
	else
	{
		m_btnOpenComm.SetWindowTextA("打开串口");
		CloseHandle(m_hComm);
		m_hComm = INVALID_HANDLE_VALUE;
		st_DataFlag.bIsCommOpen = FALSE;
	}

}

/*unsigned int CComm::thread_read()
{
	BOOL bRet;
	DWORD dw;

	c_event_event_listener listener;

	const int kReadBufSize = 1 << 20;
	unsigned char* block_data = NULL;
	block_data = new unsigned char[kReadBufSize];

_wait_for_work:
	debug_out(("[读线程] 就绪\n"));
	dw = ::WaitForSingleObject(_thread_read.hEventToBegin, INFINITE);
	SMART_ASSERT(dw == WAIT_OBJECT_0)(dw).Fatal();

	debug_out(("[读线程] 开始工作...\n"));
	if (!is_opened()) {
		debug_out(("[读线程] 没有工作, 退出中...\n"));
		delete[] block_data;
		::SetEvent(_thread_read.hEventToExit);
		return 0;
	}

	c_overlapped overlap(false, false);

	_event_listener.add_listener(listener, EV_RXCHAR);


	HANDLE handles[2];
	handles[0] = _thread_read.hEventToExit;
	handles[1] = listener.hEvent;

_get_packet:
	switch (::WaitForMultipleObjects(_countof(handles), handles, FALSE, INFINITE))
	{
	case WAIT_FAILED:
		_notifier->msgerr("[读线程] Wait失败!\n");
		goto _restart;
	case WAIT_OBJECT_0 + 0:
		debug_out(("[读线程] 收到退出事件!\n"));
		goto _restart;
	case WAIT_OBJECT_0 + 1:
		break;
	}

	DWORD nBytesToRead, nRead, nTotalRead;
	DWORD	comerr;
	COMSTAT	comsta;
	// for some reasons, such as comport has been removed
	if (!::ClearCommError(_hComPort, &comerr, &comsta)) {
		_notifier->msgerr("ClearCommError()");
		goto _restart;
	}

	nBytesToRead = comsta.cbInQue;
	if (nBytesToRead == 0)
		nBytesToRead++; // would never happen

	if (nBytesToRead > kReadBufSize)
		nBytesToRead = kReadBufSize;

	for (nTotalRead = 0; nTotalRead < nBytesToRead;) {
		bRet = ::ReadFile(_hComPort, block_data + nTotalRead, nBytesToRead - nTotalRead, &nRead, &overlap);
		if (bRet != FALSE) {
			bRet = ::GetOverlappedResult(_hComPort, &overlap, &nRead, FALSE);
			if (bRet) {
				debug_out(("[读线程] 读取 %d 字节, bRet==TRUE, nBytesToRead: %d\n", nRead, nBytesToRead));
			}
			else {
				_notifier->msgerr("[写线程] GetOverlappedResult失败!\n");
				goto _restart;
			}
		}
		else {
			if (::GetLastError() == ERROR_IO_PENDING) {
				HANDLE handles[2];
				handles[0] = _thread_read.hEventToExit;
				handles[1] = overlap.hEvent;

				switch (::WaitForMultipleObjects(_countof(handles), &handles[0], FALSE, INFINITE))
				{
				case WAIT_FAILED:
					debug_out(("[读线程] 等待失败!\n"));
					goto _restart;
				case WAIT_OBJECT_0 + 0:
					debug_out(("[读线程] 收到退出事件!\n"));
					goto _restart;
				case WAIT_OBJECT_0 + 1:
					bRet = ::GetOverlappedResult(_hComPort, &overlap, &nRead, FALSE);
					if (bRet) {
						debug_out(("[读线程] 读取 %d 字节, bRet==FALSE\n", nRead));
					}
					else {
						_notifier->msgerr("[读线程] GetOverlappedResult失败!\n");
						goto _restart;
					}
					break;
				}
			}
			else {
				_notifier->msgerr("[读线程] ::GetLastError() != ERROR_IO_PENDING");
				goto _restart;
			}
		}

		if (nRead > 0) {
			nTotalRead += nRead;
			_data_counter.add_recv(nRead);
			_data_counter.call_updater();
		}
		else {
			nBytesToRead--;
		}
	}
	call_data_receivers(block_data, nBytesToRead);
	goto _get_packet;

_restart:
	if (!::CancelIo(_hComPort)) {

	}

	// Sometimes we got here not because of we've got a exit signal
	// Maybe something wrong
	// And if something wrong, the following handle is still non-signal.
	// The main thread notify this thread to exit by signaling the event and then wait
	// this thread Reset it, since the event is a Manual reset event handle.
	// So, let's wait whatever the current signal-state the event is, just before the
	// main thread  really want we do that.
	::WaitForSingleObject(_thread_read.hEventToExit, INFINITE);
	::ResetEvent(_thread_read.hEventToExit);

	goto _wait_for_work;
}
*/
//读串口线程
UINT ReadComm(LPVOID pParam)
{
	CSerialCommTestDlg* dlg = (CSerialCommTestDlg*)pParam;

	memset(&osRead, 0, sizeof(osRead));
	memset(&osShare, 0, sizeof(osShare));

	osRead.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (osRead.hEvent == NULL)
		AfxMessageBox("建立read事件失败！");

	osShare.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (osShare.hEvent == NULL)
		AfxMessageBox("建立osShare事件失败");

	PurgeComm(m_hComm, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);// 清除缓冲区

				DWORD dwLengthLast=0;
				CString strshow="";
	while (st_DataFlag.bIsCommOpen)
	{
		//DWORD nResult = WaitForSingleObject(osRead.hEvent,INFINITE);
		DWORD dwEvtMask = 0;
		WaitCommEvent(m_hComm, &dwEvtMask, &osShare);	// 等待串口事件

		INT nEvent = WaitForSingleObject(osShare.hEvent, 1000);// INFINITE);
		CString str1;
				CString strRec;
				DWORD dwLength;
		if (nEvent == WAIT_OBJECT_0)
		{
			if ((dwEvtMask & EV_RXCHAR) == EV_RXCHAR)
			{
				DWORD dErrInformation;
				COMSTAT comStat;//通信设备控制块  
				DWORD dRBufferSize = 0;
				DWORD i;

				if (m_hComm != NULL)
				{
					ClearCommError(m_hComm, &dErrInformation, &comStat);
				}
				dwLength = comStat.cbInQue;	// 缓冲区中的数据个数
				strRec = "";

				if (RxLength + dwLength > MAX_BUFFER_SIZE)
					RxLength = 0;
				if (dwLength > MAX_BUFFER_SIZE)
					dwLength = MAX_BUFFER_SIZE;

				if (dwLength > 0)
				{
					ReadFile(m_hComm, byRxBuffer + RxLength, dwLength, &dRBufferSize, &osRead);
				}

				if (dlg->m_Chk_rhex.GetCheck())
				{
					char tempstr[4];
					for (i = 0; i < dwLength; i++)
					{
						sprintf_s(tempstr, "%02X ", (BYTE)byRxBuffer[RxLength + i]);
						strRec += tempstr;
					}
				}
				else
				{
					for (i = 0; i < dwLength; i++)
					{
						strRec += byRxBuffer[RxLength + i];
					}
				}
				st_DataFlag.strReceivedData += strRec;
				st_DataFlag.uiBytesReceived += dwLength;	// 接收数据个数
				RxLength += dwLength;
				RxFlag = 1;
				strshow += strRec;
				//str1.Format("%d", RxLength);
				//dlg->m_stRxLen.SetWindowTextA(str1);
				PurgeComm(m_hComm, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);// 清除缓冲区

				//::SendMessage(dlg->GetSafeHwnd(), WM_SHOWRECEIVE, (WPARAM)(&strRec), (LPARAM)&dwLength);
				//dlg->CheckCommand();
			}
		}
		else if (nEvent == WAIT_TIMEOUT)
		{
			if (RxFlag)
			{
				RxFlag = 0;
				DWORD len;
				str1.Format("%d", RxLength);
				dlg->m_stRxLen.SetWindowTextA(str1);
				len = dwLengthLast - RxLength;

				::SendMessage(dlg->GetSafeHwnd(), WM_SHOWRECEIVE, (WPARAM)(&strshow), (LPARAM)&len);
				dwLengthLast = RxLength;
				strshow = "";
				//dlg->CheckCommand();

			}
		}
	}
	CloseHandle(osRead.hEvent);
	CloseHandle(osShare.hEvent);
	return TRUE;
}

UINT ReadComm1(LPVOID pParam)
{
	CSerialCommTestDlg* dlg = (CSerialCommTestDlg*)pParam;
	OVERLAPPED os;//异状态步输入/输出的  
	OVERLAPPED m_osRead;
	COMSTAT comStat;//通信设备控制块  
	CString strRec;

	memset(&os, 0, sizeof(os));
	memset(&m_osRead, 0, sizeof(m_osRead));
	os.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_osRead.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	char czReceiveBuffer[1024];
	DWORD dErrInformation;
	memset(czReceiveBuffer, 0, sizeof(czReceiveBuffer));

	SetCommMask(m_hComm, EV_RXCHAR);// 设置通知事件。EV_RXCHAR: 输入缓冲区有数据时，通过WaitCommEvent函数可以获得通知
									/*if(os.hEvent=NULL)
									{
									AfxMessageBox("无法创建事件对象!");
									return (UINT)-1;
									}*/
									/****************** 在通信之前清除掉错误信息**************/
	if (m_hComm != NULL)
	{
		ClearCommError(m_hComm, &dErrInformation, &comStat);
	}

	BOOL bRunning = TRUE;
	while (bRunning && st_DataFlag.bIsCommOpen)
	{
		DWORD wEven;
		INT bResult = WaitCommEvent(m_hComm, &wEven, &os);
		//DWORD BytesRead;  
		if (!bResult)
		{
			DWORD dwError = GetLastError();
			switch (dwError)
			{
				case ERROR_IO_PENDING:
					break;
				case 87:
					break;
				default:
					//TRACE("XCOM ComThread WaitCommEvent:Error\n"); 
					AfxMessageBox("WaitCommEvent错误！");
					bRunning = FALSE;
					break;
			}
		}
		else
		{
			ClearCommError(m_hComm, &dErrInformation, &comStat);
			if (comStat.cbInQue == 0)// 缓冲区数据长度
			{
				continue;
			}
		}

		INT nEvent = WaitForSingleObject(os.hEvent, INFINITE);
		strRec = "";
		if (nEvent == WAIT_OBJECT_0)
		{
			DWORD CommEvent = 0;
			GetCommMask(m_hComm, &CommEvent);
					DWORD dRBufferSize = 100;
			if ((CommEvent & EV_RXCHAR) == EV_RXCHAR)//输入缓冲区接收到新字符  
			{
				
				BOOL bread = TRUE;
				BOOL bresult = TRUE;
				DWORD deError = 0;
				DWORD BytesRead = 0;
				/////////////////////  
				COMSTAT comstat;
				for (;;)
				{
					INT bResult = ClearCommError(m_hComm, &deError, &comstat);//成功返回值非0  
					if (comstat.cbInQue == 0)
					{
						// 接收数据完成
						//dlg->m_listRead.InsertString(dlg->m_listRead.GetCount(), strRec);
						//dlg->m_listRead.SetTopIndex(dlg->m_listRead.GetCount() - 1);

						RxFlag = 1;
						break;
					}
					if (bread)
					{
						//strRec = "";
						bresult = ReadFile(m_hComm, czReceiveBuffer, 100, &dRBufferSize, &m_osRead);
						if (!bresult)
						{
							switch (deError = GetLastError())
							{
								case ERROR_IO_PENDING:
								{
									//bread = FALSE;
									while (!GetOverlappedResult(m_hComm,
										&m_osRead,
										&BytesRead,
										TRUE
										))
									{
										deError = GetLastError();
										if (deError == ERROR_IO_INCOMPLETE) continue;
										else
											break;
									}
									break;
								}
								default:
								{
									break;
								}
							}
						}
						else
						{
							bread = TRUE;
						}
					}
					//if (!bread)
					//{
					//	bread = TRUE;
					//	bresult = GetOverlappedResult
					//		(
					//			m_hComm,
					//			&m_osRead,
					//			&BytesRead,
					//			TRUE
					//			);
					//	if (!bresult)
					//	{// Error in communications; report it.
					//	}
					//}
					PurgeComm(m_hComm, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
					st_DataFlag.uiBytesReceived += BytesRead;	// 接收数据个数

					if (dlg->m_Chk_rhex.GetCheck())
					{
						char tempstr[4];
						//CString str1;
						for (DWORD k = 0;k<BytesRead;k++)
						{
							sprintf_s(tempstr, "%02X ", (BYTE)czReceiveBuffer[k]);
							strRec += tempstr;
							byRxBuffer[RxLength++] = (BYTE)czReceiveBuffer[k];
							//str1.Format("%d", RxLength);
							//dlg->m_stRxLen.SetWindowTextA(str1);
						}
					}
					else
					{
						for (DWORD k = 0;k<BytesRead;k++)
						{
							strRec += czReceiveBuffer[k];
						}
					}
					st_DataFlag.strReceivedData += strRec;
					/*dlg->m_listRead.InsertString(dlg->m_listRead.GetCount(), strRec);
					dlg->m_listRead.SetTopIndex(dlg->m_listRead.GetCount() - 1);*/

					CString str1;
					str1.Format("%d", RxLength);
					dlg->m_stRxLen.SetWindowTextA(str1);
				}

				::SendMessage(dlg->GetSafeHwnd(), WM_SHOWRECEIVE, (WPARAM)&strRec, (LPARAM)&dRBufferSize);
				//CSerialCommTestDlg::CheckCommand1();

				//::SendMessage(dlg->GetSafeHwnd(), WM_COMMANDPROCESS, NULL, NULL);
			}
		}
		if (nEvent != WAIT_OBJECT_0)
		{
			GetLastError();
		}
	}
	return TRUE;
}

// 清除
void CSerialCommTestDlg::OnBnClickedClearRead()
{
	// TODO: 在此添加控件通知处理程序代码
	m_listRead.ResetContent();
	m_strRead = "";
	UpdateData(FALSE);
	st_DataFlag.strReceivedData = "";
	st_DataFlag.uiBytesReceived = 0;
	st_DataFlag.uiBytesSent = 0;
	RxLength = 0;
	m_stRxLen.SetWindowTextA("0");

	// 清除图表
	CSeries LineSeries1 = m_tchart.Series(0);// 直线序列
	CSeries LineSeries2 = m_tchart.Series(1);// 直线序列
	LineSeries1.Clear();	// 清空数据
	LineSeries2.Clear();	// 清空数据

}

void CSerialCommTestDlg::WriteComm(unsigned char *buff, unsigned int writebytes)
{
	if (st_DataFlag.bIsCommOpen == FALSE)
	{
		AfxMessageBox("串口未打开！");
		return;
	}

	OVERLAPPED m_osWriter;
	OVERLAPPED os;//异状态步输入/输出的  

	memset(&m_osWriter, 0, sizeof(OVERLAPPED));
	memset(&os, 0, sizeof(os));
	m_osWriter.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	UpdateData(TRUE);
	//  m_strSend += "/n";  
	COMSTAT ComStat;
	DWORD dwErrorFlags;
	BOOL  bWriterStat;
	DWORD dwBytesWrite = 0;
	DWORD dwWrite = 0;
	ClearCommError(m_hComm, &dwErrorFlags, &ComStat);
	bWriterStat = WriteFile(m_hComm, m_strWrite, m_strWrite.GetLength(), &dwBytesWrite, &m_osWriter);
	if (!bWriterStat)
	{
		if (GetLastError() == ERROR_IO_PENDING)
		{
			//WaitForSingleObject(m_osWriter.hEvent, 1000);
			while (!GetOverlappedResult(
				m_hComm,
				&m_osWriter,
				&dwWrite,
				TRUE
				))
			{
				if (GetLastError() == ERROR_IO_INCOMPLETE)
				{
					// 发送未完成
					st_DataFlag.uiBytesSent += dwWrite;
					continue;
				}
			}
		}
	}
}

// "发送"按钮
void CSerialCommTestDlg::OnBnClickedSend()
{
	// TODO:  在此添加控件通知处理程序代码
	CByteArray hexdata;

	if (st_DataFlag.bIsCommOpen == FALSE)
	{
		AfxMessageBox("串口未打开！");
		return;
	}

	OVERLAPPED m_osWriter;
	memset(&m_osWriter, 0, sizeof(OVERLAPPED));
	m_osWriter.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	UpdateData(TRUE);
	//  m_strSend += "/n";  
	COMSTAT ComStat;
	DWORD dwErrorFlags;
	BOOL  bWriterStat;
	DWORD dwBytesWrite = 0;
	DWORD dwWrite = 0;
	DWORD dwNumberOfBytesToWrite = 0;

	ClearCommError(m_hComm, &dwErrorFlags, &ComStat);

	//while (1)
	{
		if (m_Chk_thex.GetCheck())	// HEX发送
		{
			dwNumberOfBytesToWrite = String2Hex(m_strWrite, &hexdata);
			bWriterStat = WriteFile(m_hComm, &hexdata[0], dwNumberOfBytesToWrite, &dwBytesWrite, &m_osWriter);
		}
		else
		{
			dwNumberOfBytesToWrite = m_strWrite.GetLength();
			bWriterStat = WriteFile(m_hComm, m_strWrite, dwNumberOfBytesToWrite, &dwBytesWrite, &m_osWriter);
		}

		if (!bWriterStat)
		{
			DWORD lstError = GetLastError();
			if (lstError == ERROR_IO_PENDING)
			{
				WaitForSingleObject(m_osWriter.hEvent, INFINITE);
				st_DataFlag.uiBytesSent += dwBytesWrite;
				//INT nEvent = WaitForSingleObject(m_osWriter.hEvent, INFINITE);
				//if (nEvent == WAIT_OBJECT_0)
				//{
				//	break;
				//}
				//else if (nEvent == WAIT_OBJECT_0 + 1)
				//{
				//	if (!GetOverlappedResult(m_hComm, &m_osWriter, &dwWrite, TRUE))
				//	{
				//		lstError = GetLastError();
				//		//if (lstError == ERROR_IO_INCOMPLETE)
				//		//{
				//		//	// 发送未完成
				//		//	st_DataFlag.uiBytesSent += dwWrite;
				//		//	continue;
				//		//}
				//		if (lstError == ERROR_INVALID_HANDLE)
				//			break;

				//		break;
				//	}

				//}
				//else if (nEvent == WAIT_FAILED)
				//{
				//	break;
				//}
				//while (!GetOverlappedResult(
				//	m_hComm,
				//	&m_osWriter,
				//	&dwWrite,
				//	TRUE
				//	))
				//{
				//	if (GetLastError() == ERROR_IO_INCOMPLETE)
				//	{
				//		// 发送未完成
				//		st_DataFlag.uiBytesSent += dwWrite;
				//		continue;
				//	}
				//}

			}
			/*else if (lstError == ERROR_INVALID_HANDLE)
				break;
			else
				break;*/
		}
		//else
		//	break;
		//dwNumberOfBytesToWrite -= dwWrite;
		//if (dwNumberOfBytesToWrite <= 0)
		//	break;
	}
}



afx_msg LRESULT CSerialCommTestDlg::OnShowreceive(WPARAM wParam, LPARAM lParam)
{
	CString str = *(CString*)wParam;
	m_strRead += str;
	UpdateData(FALSE);
	m_editRead.LineScroll(m_editRead.GetLineCount());
	return 0;
}

// 查找串口
void CSerialCommTestDlg::FindComPort()
{
	HKEY   hKey;

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Hardware\\DeviceMap\\SerialComm"), NULL, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		TCHAR       szPortName[256], szComName[256];
		DWORD       dwLong, dwSize;
		int         nCount = 0;
		CComboBox*  pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_COMM);

		pCombo->ResetContent();
		while (true)
		{
			dwLong = dwSize = 256;
			if (RegEnumValue(hKey, nCount, szPortName, &dwLong, NULL, NULL, (PUCHAR)szComName, &dwSize) == ERROR_NO_MORE_ITEMS)
				break;

			pCombo->InsertString(nCount, szComName);
			nCount++;
		}
		RegCloseKey(hKey);
		pCombo->SetCurSel(0);
	}

}

// 初始化串口参数
void CSerialCommTestDlg::InitComPortDialog()
{
	// 波特率
	m_Combo_baud.AddString("4800");
	m_Combo_baud.AddString("9600");
	m_Combo_baud.AddString("19200");
	m_Combo_baud.AddString("115200");
	m_Combo_baud.SetCurSel(1);

	// 数据位数
	m_Combo_databits.AddString("5");
	m_Combo_databits.AddString("6");
	m_Combo_databits.AddString("7");
	m_Combo_databits.AddString("8");
	m_Combo_databits.SetCurSel(3);

	// 停止位
	m_Combo_stopbits.AddString("1");
	m_Combo_stopbits.AddString("1.5");
	m_Combo_stopbits.AddString("2");
	m_Combo_stopbits.SetCurSel(0);

	// 校验位
	m_Combo_paritybits.AddString("无");
	m_Combo_paritybits.AddString("奇");
	m_Combo_paritybits.AddString("偶");
	m_Combo_paritybits.AddString("0");
	m_Combo_paritybits.AddString("1");
	m_Combo_paritybits.SetCurSel(0);
}

// 把字符串转成数组，返回字节数
int CSerialCommTestDlg::String2Hex(CString srcstr, CByteArray *hexdata)
{
	int data;
	int hexdatalen = 0;
	int len = srcstr.GetLength();
	char cstr[3];
	CString strtemp;
	int i;
	
	i = srcstr.Remove(' ');	// 移除空格
	len -= i;
	(*hexdata).SetSize(len / 2);
	for (i = 0; i < len; i += 2)
	{
		strtemp = srcstr.Mid(i, 2);
		sprintf_s(cstr, "%s", strtemp);
		sscanf_s(cstr, "%x", &data);	// 把十六进制字符串转成数值
		(*hexdata).SetAt(hexdatalen, data);
		//(*hexdata).InsertAt(hexdatalen, data);
		hexdatalen++;
	}
	return hexdatalen;
}



// 接收区域十六进制显示
void CSerialCommTestDlg::OnBnClickedCheckReceivehex()
{
	// TODO: 在此添加控件通知处理程序代码
	CString readstr = "";// = m_strRead;


	DWORD len = st_DataFlag.strReceivedData.GetLength();
	char tempstr[4];
	CByteArray hexdata;

	if (m_Chk_rhex.GetCheck())	// HEX
	{
		for (DWORD k = 0; k<len; k++)
		{
			sprintf_s(tempstr, "%02X ", (BYTE)st_DataFlag.strReceivedData[k]);
			readstr += tempstr;
		}
		m_strRead = readstr;
		st_DataFlag.strReceivedData = readstr;
	}
	else
	{
		int hexlen = String2Hex(st_DataFlag.strReceivedData, &hexdata);
		//m_strRead.Format("%c", 0x30);//(char*)&hexdata);
		for (int i = 0; i < hexlen; i++)
		{
			st_DataFlag.strReceivedData.Format("%c", hexdata[i]);
			readstr += st_DataFlag.strReceivedData;
		}
		m_strRead = readstr;
		st_DataFlag.strReceivedData = readstr;
	}
	UpdateData(FALSE);
}

// 初始化参数
void CSerialCommTestDlg::InitDataFlag()
{
	st_DataFlag.bIsCommOpen = FALSE;
	st_DataFlag.strReceivedData = "";
	st_DataFlag.uiBytesReceived = 0;
	st_DataFlag.uiBytesSent = 0;
}

// 处理接收到的指令
//UINT CheckCommand(LPVOID pParam)
UINT CSerialCommTestDlg::CheckCommand(void)
{
	USHORT i = 0, j = 0, point = 0;
	BYTE cmd, crc = 0;
	USHORT len;
	CSeries LineSeries1 = m_tchart.Series(0);// 直线序列
	CSeries LineSeries2 = m_tchart.Series(1);// 直线序列
	CByteArray y1, y2;

	if (RxFlag)
	{
		RxFlag = 0;

		if (RxLength < 7)
		{
			return 0;
		}

		point = 0;
		if (byRxBuffer[point] != 0xAA)
		{
			RxLength = 0;
			return 0;
		}
		point++;
		if (byRxBuffer[point] != 0x99)
		{
			RxLength = 0;
			return 0;
		}
		point++;
		if (byRxBuffer[point] != MODEID)
		{
			RxLength = 0;
			return 0;
		}
		point++;
		cmd = byRxBuffer[point];
		point++;
		len = byRxBuffer[point];
		point++;
		len = (len << 8) + byRxBuffer[point];
		if (RxLength - 7 < len)	// 数据未接收完整
			return 0;

		crc = 0;
		for (i = 0; i < len + 6; i++)
		{
			crc = crc ^ byRxBuffer[i];
		}

		if (crc != byRxBuffer[i]) // 校验失败
		{
			RxLength = 0;
			AfxMessageBox("校验失败！");
			return 0;
		}
		//AfxMessageBox("校验成功！");

		y1.SetSize(len / 2);
		y2.SetSize(len / 2);
		j = 6;
		for (i = 0; i < len / 2; i++)
		{
			y1[i] = byRxBuffer[j++];
			y2[i] = byRxBuffer[j++];
		}

		switch (cmd)
		{
		case 0xF1: // 启动测量, 
		{
			LineSeries1.Clear();	// 清空数据
			LineSeries2.Clear();	// 清空数据
			for (i = 0; i < len / 2; i++)
			{
				LineSeries1.AddXY(i, y1[i], 0, RGB(255, 0, 0));
				LineSeries2.AddXY(i, y2[i], 0, RGB(0, 255, 0));
			}
		}
		break;

		}
		i = RxLength;
		RxLength = 0;
	}
	return i;
	////CSerialCommTestDlg *pDlg = (CSerialCommTestDlg*)pParam;
	//USHORT i = 0;
	//BYTE cmd, crc=0;
	//USHORT len;
	//CSeries LineSeries1 = m_tchart.Series(0);// 直线序列
	//CSeries LineSeries2 = m_tchart.Series(1);// 直线序列
	//
	//if (RxFlag)
	//{
	//	RxFlag = 0;

	//	if (RxLength < 7)
	//	{
	//		return 0;
	//	}

	//	if (byRxBuffer[i] != 0xAA)
	//	{
	//		RxLength = 0;
	//		return 0;
	//	}
	//	i++;
	//	if (byRxBuffer[i] != 0x99)
	//	{
	//		RxLength = 0;
	//		return 0;
	//	}
	//	i++;
	//	if (byRxBuffer[i] != MODEID)
	//	{
	//		RxLength = 0;
	//		return 0;
	//	}
	//	i++;
	//	cmd = byRxBuffer[i];
	//	i++;
	//	len = byRxBuffer[i];
	//	i++;
	//	len = (len << 8) + byRxBuffer[i];
	//	i++;
	//	
	//	if (RxLength - 7 < len)	// 数据未接收完整
	//		return 0;

	//	crc = 0;
	//	for (i = 0; i < len + 6; i++)
	//	{
	//		crc = crc ^ byRxBuffer[i];
	//	}

	//	if (crc != byRxBuffer[i]) // 校验失败
	//	{
	//		RxLength = 0;
	//		AfxMessageBox("校验失败！");
	//		return 0;
	//	}
	//	AfxMessageBox("校验成功！");

	//	switch (cmd)
	//	{
	//	case 0xF1: // 启动测量, 
	//	{

	//	}
	//		break;

	//	}
	//	i = RxLength;
	//	RxLength = 0;
	//}
	//return i;
}


afx_msg LRESULT CSerialCommTestDlg::OnCommandprocess(WPARAM wParam, LPARAM lParam)
{
	USHORT i = 0, j = 0, point = 0;
	BYTE cmd, crc = 0;
	USHORT len;
	CSeries LineSeries1 = m_tchart.Series(0);// 直线序列
	CSeries LineSeries2 = m_tchart.Series(1);// 直线序列
	CByteArray y1, y2;
	
	if (RxFlag)
	{
		RxFlag = 0;

		if (RxLength < 7)
		{
			return 0;
		}

		point = 0;
		if (byRxBuffer[point] != 0xAA)
		{
			RxLength = 0;
			return 0;
		}
		point++;
		if (byRxBuffer[point] != 0x99)
		{
			RxLength = 0;
			return 0;
		}
		point++;
		if (byRxBuffer[point] != MODEID)
		{
			RxLength = 0;
			return 0;
		}
		point++;
		cmd = byRxBuffer[point];
		point++;
		len = byRxBuffer[point];
		point++;
		len = (len << 8) + byRxBuffer[point];
		if (RxLength - 7 < len)	// 数据未接收完整
			return 0;
		
		crc = 0;
		for (i = 0; i < len + 6; i++)
		{
			crc = crc ^ byRxBuffer[i];
		}

		if (crc != byRxBuffer[i]) // 校验失败
		{
			RxLength = 0;
			AfxMessageBox("校验失败！");
			return 0;
		}
		//AfxMessageBox("校验成功！");

		y1.SetSize(len/2);
		y2.SetSize(len/2);
		j = 6;
		for (i = 0; i < len / 2; i++)
		{
			y1[i] = byRxBuffer[j++];
			y2[i] = byRxBuffer[j++];
		}

		switch (cmd)
		{
		case 0xF1: // 启动测量, 
		{
			LineSeries1.Clear();	// 清空数据
			LineSeries2.Clear();	// 清空数据
			for (i = 0; i < len / 2; i++)
			{
				LineSeries1.AddXY(i, y1[i], 0, RGB(255, 0, 0));
				LineSeries2.AddXY(i, y2[i], 0, RGB(0, 255, 0));
			}
		}
		break;

		}
		i = RxLength;
		RxLength = 0;
	}
	return i;
}
