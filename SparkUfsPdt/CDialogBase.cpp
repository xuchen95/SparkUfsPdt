#include "pch.h"
#include "CDialogBase.h"

UFS_OPTION CDialogBase::s_sharedOption = {};

CDialogBase::CDialogBase(UINT nIDTemplate, CWnd* pParent /*= nullptr*/)
	: CDialogEx(nIDTemplate, pParent)
{
	m_pUfsOption = &s_sharedOption;
}

PUFS_OPTION CDialogBase::GetSharedUfsOption()
{
	return &s_sharedOption;
}

void CDialogBase::SetUfsOption(PUFS_OPTION pOption)
{
	if (pOption)
	{
		m_pUfsOption = pOption;
	}
}

PUFS_OPTION CDialogBase::GetUfsOption() const
{
	return m_pUfsOption;
}

void CDialogBase::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}
