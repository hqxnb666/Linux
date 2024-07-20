#pragma once


// Clogin 对话框

class Clogin : public CDialogEx
{
	DECLARE_DYNAMIC(Clogin)

public:
	Clogin(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~Clogin();
	CString username;
	CString userpassword;
// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD =  };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnClose();
};
