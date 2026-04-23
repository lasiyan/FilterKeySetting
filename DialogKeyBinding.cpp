// DialogKeyBinding.cpp: implementation file
//

// clang-format off
#include "pch.h"
#include "FilterKeySetting.h"
#include "DialogKeyBinding.h"
#include "UserKeyBinding.hpp"
#include "UserLanguage.hpp"
#include "afxdialogex.h"
// clang-format on

IMPLEMENT_DYNAMIC(DialogKeyBinding, CDialogEx)

DialogKeyBinding::DialogKeyBinding(DWORD current_hotkey, CWnd* pParent)
    : CDialogEx(IDD_KEY_BINDING, pParent), hotkey_value_(current_hotkey)
{
}

DialogKeyBinding::~DialogKeyBinding()
{
}

void DialogKeyBinding::DoDataExchange(CDataExchange* pDX)
{
  CDialogEx::DoDataExchange(pDX);
  DDX_Text(pDX, IDC_EDIT_KEY_BINDING, hotkey_text_);
}

BEGIN_MESSAGE_MAP(DialogKeyBinding, CDialogEx)
END_MESSAGE_MAP()

BOOL DialogKeyBinding::OnInitDialog()
{
  CDialogEx::OnInitDialog();

  hotkey_text_ = KeyBinding::HotkeyToString(hotkey_value_, true);
  UpdateData(FALSE);

  static constexpr Lang::ControlTextEntry kKeyBindDlgTexts[] = {
    { IDC_STATIC_KEY_BINDING_GUIDE, IDS_LBL_KEYBIND_GUIDE },
    { IDOK, IDS_BTN_APPLY },
    { IDCANCEL, IDS_BTN_CANCEL },
  };
  Lang::ApplyCaption(this, IDS_DLG_KEYBIND_TITLE);
  Lang::ApplyControlTexts(this, kKeyBindDlgTexts, _countof(kKeyBindDlgTexts));

  return TRUE;
}

BOOL DialogKeyBinding::PreTranslateMessage(MSG* pMsg)
{
  if (pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN)
  {
    const UINT vk = static_cast<UINT>(pMsg->wParam);

    if (vk == VK_RETURN)
    {
      CDialogEx::OnOK();
      return TRUE;
    }

    if (vk == VK_BACK)
    {
      ClearHotkey();
      return TRUE;
    }

    if (KeyBinding::IsModifierVirtualKey(vk))
    {
      return TRUE;
    }

    UINT modifiers = KeyBinding::CurrentModifiers();

    SetHotkeyValue(vk, modifiers);
    return TRUE;
  }

  return CDialogEx::PreTranslateMessage(pMsg);
}

void DialogKeyBinding::SetHotkeyValue(UINT vk, UINT modifiers)
{
  hotkey_value_ = (static_cast<DWORD>(modifiers) << 16) | static_cast<DWORD>(vk);
  hotkey_text_  = KeyBinding::HotkeyToString(hotkey_value_, true);
  UpdateData(FALSE);
}

void DialogKeyBinding::ClearHotkey()
{
  hotkey_value_ = 0;
  hotkey_text_  = KeyBinding::HotkeyToString(0, true);
  UpdateData(FALSE);
}
