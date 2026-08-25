// CDialogPortMapping.cpp: 实现文件
//

#include "pch.h"
#include "SparkUfsPdt.h"
#include "afxdialogex.h"
#include "CDialogPortMapping.h"


// CDialogPortMapping 对话框

IMPLEMENT_DYNAMIC(CDialogPortMapping, CDialogEx)

CDialogPortMapping::CDialogPortMapping(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG_PORT_MAPPING, pParent)
{

}

CDialogPortMapping::~CDialogPortMapping()
{
}

void CDialogPortMapping::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDialogPortMapping, CDialogEx)
	ON_BN_CLICKED(IDC_PM_BTN_SCAN_DEVICE, &CDialogPortMapping::OnBnClickedPmBtnScanDevice)
	ON_BN_CLICKED(IDC_PM_BTN_WRITE_TESTER_ID, &CDialogPortMapping::OnBnClickedPmBtnWriteTesterId)
END_MESSAGE_MAP()


// CDialogPortMapping 消息处理程序

void CDialogPortMapping::OnBnClickedPmBtnScanDevice()
{
	// TODO: 在此添加控件通知处理程序代码
}

void CDialogPortMapping::OnBnClickedPmBtnWriteTesterId()
{
	// TODO: 在此添加控件通知处理程序代码
}
