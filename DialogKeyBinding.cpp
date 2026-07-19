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

namespace {
// 게임에서 고정으로 사용되는 단독(수정키 없는) 키: 핫키로 등록하면 RegisterHotKey가
// 전역에서 가로채 게임 내 원래 기능이 동작하지 않으므로 적용 전 경고 대상으로 판단한다.
bool IsGameReservedStandaloneKey(DWORD hotkey_value)
{
  if (KeyBinding::HotkeyModifiers(hotkey_value) != 0)
    return false;

  switch (KeyBinding::HotkeyVK(hotkey_value))
  {
    case VK_ESCAPE:
    case VK_SCROLL:    // ScrLk
    case VK_RETURN:    // Enter
    case VK_TAB:
    case VK_SNAPSHOT:  // PrtSc
    case VK_PAUSE:     // Pause/Break
      return true;
    default:
      return false;
  }
}
}  // namespace

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

void DialogKeyBinding::OnOK()
{
  if (IsGameReservedStandaloneKey(hotkey_value_))
  {
    if (AfxMessageBox(Lang::T(IDS_MSG_HOTKEY_GAME_RESERVED), MB_YESNO | MB_ICONWARNING) != IDYES)
      return;
  }

  CDialogEx::OnOK();
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
