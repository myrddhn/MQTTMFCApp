
// MFCApplication2Dlg.h : header file
//

#pragma once

#include <string>

//const std::string SERVER_ADDRESS("tcp://darwinistic.com:1883");
const std::string CLIENT_ID("andi_async_subscribe");
//const std::string TOPIC("/home/temp/int/raw");
//const std::string TOPIC2("/home/pressure/raw");
//const std::string TOPIC3("/home/temp/ext/raw");

const int	QOS1 = 1;
const int	N_RETRY_ATTEMPTS = 5;


struct s_Settings {
	CString mqttserver;
	CString topicinternaltemp;
	CString topicexternaltemp;
	CString topicpressure;
};

// CMFCApplication2Dlg dialog
class CMFCApplication2Dlg : public CDialogEx
{


// Construction
public:
	struct s_Settings Settings;

	CMFCApplication2Dlg(CWnd* pParent = nullptr);	// standard constructor
	~CMFCApplication2Dlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MFCAPPLICATION2_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;
	BOOL bDoingBackgroundProcessing = TRUE;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedMybutton();
	CEdit txtServer;
	CEdit txtTempExternal;
	CEdit txtTempInternal;
	CEdit txtPressure;
	CListBox lstDebug;
	void writelog(std::string str);
	afx_msg void OnBnClickedCancel();
	CMFCButton btnConnection;
	CStatic lblTempInt;
	CStatic lblTempExt;
	CStatic lblPressure;
	CStatic lblVCC;
	afx_msg void OnBnClickedSaverc();
	afx_msg void OnBnClickedLoadrc();
};

class action_listener : public virtual mqtt::iaction_listener
{
	std::string name_;

	void on_failure(const mqtt::token& tok) override {
	}

	void on_success(const mqtt::token& tok) override {
	}

public:
	action_listener(const std::string& name) : name_(name) {}
};

class callback : public virtual mqtt::callback,
	public virtual mqtt::iaction_listener
{
	int nretry_;
	mqtt::async_client& cli_;
	mqtt::connect_options& connOpts_;
	action_listener subListener_;
	CMFCApplication2Dlg* dlg_ = NULL;
	
public:
	void setdlg(CMFCApplication2Dlg *o) {
		this->dlg_ = o;
	}

private:
	void reconnect() {
		std::this_thread::sleep_for(std::chrono::milliseconds(2500));
		try {
			cli_.connect(connOpts_, nullptr, *this);
		}
		catch (const mqtt::exception& exc) {
			exit(1);
		}
	}

	void on_failure(const mqtt::token& tok) override {
		if (++nretry_ > N_RETRY_ATTEMPTS)
			exit(1);
		reconnect();
	}

	void on_success(const mqtt::token& tok) override {}

	void connected(const std::string& cause) override {
		std::string server = CT2A(dlg_->Settings.mqttserver);
		std::string inttemp = CT2A(dlg_->Settings.topicinternaltemp);
		std::string exttemp = CT2A(dlg_->Settings.topicexternaltemp);
		std::string pressure = CT2A(dlg_->Settings.topicpressure);

		dlg_->writelog("Connected to " + server);
		cli_.subscribe(inttemp, QOS1, nullptr, subListener_);
		cli_.subscribe(exttemp, QOS1, nullptr, subListener_);
		cli_.subscribe(pressure, QOS1, nullptr, subListener_);
	}

	void connection_lost(const std::string& cause) override {
		if (!cause.empty())
			dlg_->writelog("Connection lost (" + cause + ")");

		nretry_ = 0;
		reconnect();
	}

	void message_arrived(mqtt::const_message_ptr msg) override {
		dlg_->writelog("Message arrived: " + msg->get_topic() + " " + msg->to_string());
		CString topic(msg->get_topic().c_str());
		CString s(msg->get_payload_str().c_str());

		if (topic == dlg_->Settings.topicinternaltemp) {
			s.Insert(0, _T("Internal Temperature: "));
			s.Append(_T("°C"));
			dlg_->lblTempInt.SetWindowText(s);
		}

		if (topic == dlg_->Settings.topicexternaltemp) {
			s.Insert(0, _T("External Temperature: "));
			s.Append(_T("°C"));
			dlg_->lblTempExt.SetWindowText(s);
		}

		if (topic == dlg_->Settings.topicpressure) {
			s.Insert(0, _T("Pressure "));
			s.Append(_T("hPa"));
			dlg_->lblPressure.SetWindowText(s);
		}
	}

	void delivery_complete(mqtt::delivery_token_ptr token) override {}

public:
	callback(mqtt::async_client& cli, mqtt::connect_options& connOpts)
		: nretry_(0), cli_(cli), connOpts_(connOpts), subListener_("Subscription") {}
};