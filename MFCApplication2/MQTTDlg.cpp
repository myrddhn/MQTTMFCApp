
// MFCApplication2Dlg.cpp : implementation file
//

#include "framework.h"
#include "MQTTApp.h"
#include "MQTTDlg.h"
#include "afxdialogex.h"
#include <string>
#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <cctype>
#include <thread>
#include <chrono>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CMFCApplication2Dlg::CMFCApplication2Dlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MFCAPPLICATION2_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMFCApplication2Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDITSERVER, txtServer);
	DDX_Control(pDX, IDC_EDITEXT, txtTempExternal);
	DDX_Control(pDX, IDC_EDITINT, txtTempInternal);
	DDX_Control(pDX, IDC_EDITPRESSURE, txtPressure);
	DDX_Control(pDX, IDC_LIST1, lstDebug);
	DDX_Control(pDX, IDC_MYBUTTON, btnConnection);
	DDX_Control(pDX, IDC_TEMPINT, lblTempInt);
	DDX_Control(pDX, IDC_TEMPEXT, lblTempExt);
	DDX_Control(pDX, IDC_PRESSURE, lblPressure);
}

BEGIN_MESSAGE_MAP(CMFCApplication2Dlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CMFCApplication2Dlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_MYBUTTON, &CMFCApplication2Dlg::OnBnClickedMybutton)
	ON_BN_CLICKED(IDCANCEL, &CMFCApplication2Dlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_SAVERC, &CMFCApplication2Dlg::OnBnClickedSaverc)
	ON_BN_CLICKED(IDC_LOADRC, &CMFCApplication2Dlg::OnBnClickedLoadrc)
END_MESSAGE_MAP()

// CMFCApplication2Dlg message handlers

BOOL CMFCApplication2Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	//CString mqttserver;
	
	CFile rcfile(_T("settings.rc"), CFile::modeRead);
	CArchive archive(&rcfile, CArchive::load);
	archive >> Settings.mqttserver >> Settings.topicinternaltemp >> Settings.topicexternaltemp >> Settings.topicpressure;
	archive.Close();
	rcfile.Close();

	txtServer.SetWindowTextW(Settings.mqttserver);
	txtTempInternal.SetWindowTextW(Settings.topicinternaltemp);
	txtTempExternal.SetWindowTextW(Settings.topicexternaltemp);
	txtPressure.SetWindowTextW(Settings.topicpressure);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMFCApplication2Dlg::OnPaint()
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

HCURSOR CMFCApplication2Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CMFCApplication2Dlg::OnBnClickedOk()
{
	bDoingBackgroundProcessing = FALSE;
	CDialogEx::OnOK();
}

void CMFCApplication2Dlg::writelog(std::string str) {
	CString s(str.c_str());
	lstDebug.AddString(s);
	lstDebug.SetTopIndex(lstDebug.GetCount() - 1);
}

CMFCApplication2Dlg::~CMFCApplication2Dlg() {
	MessageBoxA(NULL, "Destructor called", "", MB_OK);
}

void CMFCApplication2Dlg::OnBnClickedMybutton()
{
	CString txt;
	CFont m_Font;
	m_Font.Detach();
	m_Font.CreateFont(-33, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, 0, 0, 0, 0, 0, _T("Tahoma"));

	lblTempInt.SetFont(&m_Font);
	lblTempExt.SetFont(&m_Font);
	lblPressure.SetFont(&m_Font);

	txtServer.GetWindowTextW(txt);

	std::string mqttserver = CT2A(Settings.mqttserver);
	mqtt::async_client cli(mqttserver, CLIENT_ID);

	mqtt::connect_options connOpts;
	connOpts.set_clean_session(false);

	callback cb(cli, connOpts);
	cb.setdlg(this);
	cli.set_callback(cb);

	try {
		writelog("Connecting to the MQTT server...");
		cli.connect(connOpts, nullptr, cb);
	}
	catch (const mqtt::exception& exc) {
		writelog("ERROR: Unable to connect to MQTT server ");
		return;
	}

	while (bDoingBackgroundProcessing)
	{
		MSG msg;
		while (::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (!AfxGetApp()->PumpMessage())
			{
				cli.disconnect()->wait();

				bDoingBackgroundProcessing = FALSE;
				::PostQuitMessage(0);

				break;
			}
		}
		LONG lIdle = 0;
		while (AfxGetApp()->OnIdle(lIdle++))
			;
	}
}


void CMFCApplication2Dlg::OnBnClickedCancel()
{
	bDoingBackgroundProcessing = FALSE;
	CDialogEx::OnCancel();
}


void CMFCApplication2Dlg::OnBnClickedSaverc()
{
	txtServer.GetWindowTextW(Settings.mqttserver);
	txtTempInternal.GetWindowTextW(Settings.topicinternaltemp);
	txtTempExternal.GetWindowTextW(Settings.topicexternaltemp);
	txtPressure.GetWindowTextW(Settings.topicpressure);

	CFile rcfile(_T("settings.rc"), CFile::modeWrite | CFile::modeCreate);
	CArchive archive(&rcfile, CArchive::store);
	archive << Settings.mqttserver << Settings.topicinternaltemp << Settings.topicexternaltemp << Settings.topicpressure;
	archive.Close();
	rcfile.Close();
}


void CMFCApplication2Dlg::OnBnClickedLoadrc()
{
	CFile rcfile(_T("settings.rc"), CFile::modeRead);
	CArchive archive(&rcfile, CArchive::load);
	archive >> Settings.mqttserver >> Settings.topicinternaltemp >> Settings.topicexternaltemp >> Settings.topicpressure;
	archive.Close();
	rcfile.Close();

	txtServer.SetWindowTextW(Settings.mqttserver);
	txtTempInternal.SetWindowTextW(Settings.topicinternaltemp);
	txtTempExternal.SetWindowTextW(Settings.topicexternaltemp);
	txtPressure.SetWindowTextW(Settings.topicpressure);
}
