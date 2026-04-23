// DialogRename.cpp: 구현 파일
//

// clang-format off
#include "pch.h"
#include "FilterKeySetting.h"
#include "DialogRename.h"
#include "UserLanguage.hpp"
#include "afxdialogex.h"
// clang-format on

// DialogRename 대화 상자

IMPLEMENT_DYNAMIC(DialogRename, CDialogEx)

DialogRename::DialogRename(CString origin_name, CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_RENAME_PRESET, pParent)
{
  new_name_   = origin_name;
  reset_once_ = TRUE;
}

DialogRename::~DialogRename() {}

void DialogRename::DoDataExchange(CDataExchange* pDX)
{
  CDialogEx::DoDataExchange(pDX);
  DDX_Text(pDX, IDC_EDIT_CHANGE_NAME, new_name_);
}

BEGIN_MESSAGE_MAP(DialogRename, CDialogEx)
ON_EN_SETFOCUS(IDC_EDIT_CHANGE_NAME, &DialogRename::OnEnSetfocusEditChangeName)
END_MESSAGE_MAP()

// DialogRename 메시지 처리기

BOOL DialogRename::OnInitDialog()
{
  CDialogEx::OnInitDialog();

  static constexpr Lang::ControlTextEntry kRenameDlgTexts[] = {
    { IDOK, IDS_BTN_APPLY },
    { IDCANCEL, IDS_BTN_CANCEL },
  };
  Lang::ApplyCaption(this, IDS_DLG_RENAME_TITLE);
  Lang::ApplyControlTexts(this, kRenameDlgTexts, _countof(kRenameDlgTexts));

  return TRUE;  // return TRUE unless you set the focus to a control
  // 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}

void DialogRename::OnEnSetfocusEditChangeName()
{
  if (reset_once_)
  {
    new_name_.Empty();
    reset_once_ = FALSE;
    UpdateData(FALSE);
  }
}

void DialogRename::OnOK()
{
  UpdateData(TRUE);

  if (new_name_.IsEmpty())
  {
    AfxMessageBox(Lang::T(IDS_MSG_ENTER_NAME));
    return;
  }

  CDialogEx::OnOK();
}
