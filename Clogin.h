#pragma once


// Clogin �Ի���

class Clogin : public CDialogEx
{
	DECLARE_DYNAMIC(Clogin)

public:
	Clogin(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~Clogin();
	CString username;
	CString userpassword;
// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD =  };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnClose();
};
