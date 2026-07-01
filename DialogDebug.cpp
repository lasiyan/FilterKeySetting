// clang-format off
#include "pch.h"
#include "DialogDebug.h"
#include "FilterKeySetting.h"
#include "UserAdminGuard.hpp"
#include "UserFilterKey.hpp"
#include "UserLanguage.hpp"
#include "UserPresetOSD.hpp"
#include "afxdialogex.h"
#include "FilterKeySettingDlg.h"
#include <shellapi.h>
// clang-format on

IMPLEMENT_DYNAMIC(DialogDebug, CDialogEx)

DialogDebug::DialogDebug(CWnd* pParent) : CDialogEx(IDD_DEBUG_CONSOLE, pParent)
{
}

DialogDebug::~DialogDebug()
{
}

void DialogDebug::DoDataExchange(CDataExchange* pDX)
{
  CDialogEx::DoDataExchange(pDX);
  DDX_Text(pDX, IDC_EDIT_DEBUG_LOG, log_text_);
}

BEGIN_MESSAGE_MAP(DialogDebug, CDialogEx)
ON_WM_CLOSE()
ON_WM_DESTROY()
ON_WM_HSCROLL()
ON_BN_CLICKED(IDC_CHECK_DBG_MOVE_TRACKER, &DialogDebug::OnBnClickedCheckDbgMoveTracker)
ON_BN_CLICKED(IDC_CHECK_SYNC_FILTERKEY, &DialogDebug::OnBnClickedCheckSyncFilterkey)
ON_BN_CLICKED(IDC_CHECK_MUTE_SOUND, &DialogDebug::OnBnClickedCheckMuteSound)
ON_BN_CLICKED(IDC_CHECK_OFF_USE_WINDOWS_DEFAULT, &DialogDebug::OnBnClickedCheckOffUseWindowsDefault)
ON_BN_CLICKED(IDC_CHECK_ENABLE_PRESET_OSD, &DialogDebug::OnBnClickedCheckEnablePresetOsd)
ON_CBN_SELCHANGE(IDC_COMBO_DBG_PRESET_COUNT, &DialogDebug::OnCbnSelchangeComboDbgPresetCount)
ON_CBN_SELCHANGE(IDC_COMBO_PRESET_OSD_CORNER, &DialogDebug::OnCbnSelchangeComboPresetOsdCorner)
ON_CBN_SELCHANGE(IDC_COMBO_PRESET_OSD_SIZE, &DialogDebug::OnCbnSelchangeComboPresetOsdSize)
ON_BN_CLICKED(IDC_RADIO_PRESET_OSD_KEEP, &DialogDebug::OnBnClickedRadioPresetOsdKeep)
ON_BN_CLICKED(IDC_RADIO_PRESET_OSD_3SEC, &DialogDebug::OnBnClickedRadioPresetOsd3sec)
ON_CBN_SELCHANGE(IDC_COMBO_MOVE_SENSITIVITY, &DialogDebug::OnCbnSelchangeComboMouseSensitivity)
ON_BN_CLICKED(IDC_CHECK_AUTO_START, &DialogDebug::OnBnClickedCheckAutoStart)
ON_BN_CLICKED(IDC_CHECK_PRESET_OFF_PROCESS, &DialogDebug::OnBnClickedCheckPresetOffProcess)
ON_BN_CLICKED(IDC_CHECK_PRESET_OFF_PROCESS_RESTORE, &DialogDebug::OnBnClickedCheckPresetOffProcessRestore)
ON_BN_CLICKED(IDC_CHECK_IF_FULL_SCREEN_GAME, &DialogDebug::OnBnClickedCheckIfFullScreenGame)
ON_BN_CLICKED(IDC_CHECK_DISABLE_HOTKEY, &DialogDebug::OnBnClickedCheckDisableHotkey)
ON_EN_KILLFOCUS(IDC_EDIT_PRESET_OFF_PROCESS, &DialogDebug::OnEnKillFocusEditPresetOffProcess)
ON_CBN_SELCHANGE(IDC_COMBO_LANG, &DialogDebug::OnCbnSelchangeComboLang)
ON_BN_CLICKED(IDC_BTN_CLEAR_APP_DATA, &DialogDebug::OnBnClickedBtnClearAppData)
END_MESSAGE_MAP()

BOOL DialogDebug::OnInitDialog()
{
  CDialogEx::OnInitDialog();

  InitializeOptions();
  DevLog::AttachSink(this);
  DevLog::Write(_T("Developer debug window opened"));

  // Disable resizing
  SetWindowLong(GetSafeHwnd(), GWL_STYLE, GetWindowLong(GetSafeHwnd(), GWL_STYLE) & ~WS_SIZEBOX);

  InitializeOsdOptions();
  InitializeMouseSensitivity();
  InitializeAutoStart();
  InitializeProcessOffOptions();

  // Tooltip
  tooltip_.Initialize(this);

  // Apply localized text for non-Korean languages
  static constexpr Lang::ControlTextEntry kDebugDlgTexts[] = {
    { IDC_CHECK_DBG_MOVE_TRACKER, IDS_CHK_DBG_MOVE_TRACKER },
    { IDC_STATIC_PRESET_COUNT_LABEL, IDS_LBL_PRESET_COUNT },
    { IDC_STATIC_EXPERIMENTAL_GROUP, IDS_GRP_EXPERIMENTAL },
    { IDC_CHECK_SYNC_FILTERKEY, IDS_CHK_SYNC_FILTERKEY },
    { IDC_CHECK_MUTE_SOUND, IDS_CHK_MUTE_SOUND },
    { IDC_CHECK_OFF_USE_WINDOWS_DEFAULT, IDS_CHK_OFF_USE_WINDOWS_DEFAULT },
    { IDC_CHECK_IF_FULL_SCREEN_GAME, IDS_CHK_IF_FULL_SCREEN_GAME },
    { IDC_CHECK_DISABLE_HOTKEY, IDS_CHK_DISABLE_HOTKEY },
    { IDC_STATIC_MOVE_SENSITIVITY_LABEL, IDS_LBL_MOVE_SENSITIVITY },
    { IDC_CHECK_AUTO_START, IDS_CHK_AUTO_START },
    { IDC_CHECK_ENABLE_PRESET_OSD, IDS_CHK_ENABLE_PRESET_OSD },
    { IDC_STATIC_PRESET_OSD_ALPHA, IDS_LBL_OSD_ALPHA_TRANSPARENT },
    { IDC_STATIC_PRESET_OSD_DURATION, IDS_LBL_OSD_DURATION },
    { IDC_RADIO_PRESET_OSD_KEEP, IDS_RDO_OSD_KEEP },
    { IDC_RADIO_PRESET_OSD_3SEC, IDS_RDO_OSD_3SEC },
    { IDC_STATIC_PRESET_OSD_DURATION2, IDS_LBL_OSD_CORNER },
    { IDC_STATIC_PRESET_OSD_DURATION3, IDS_LBL_OSD_SIZE },
    { IDC_CHECK_PRESET_OFF_PROCESS, IDS_CHK_PRESET_OFF_PROCESS },
    { IDC_STATIC_PROCESS_LABEL, IDS_LBL_PROCESS },
    { IDC_CHECK_PRESET_OFF_PROCESS_RESTORE, IDS_CHK_PRESET_OFF_PROCESS_RESTORE },
    { IDC_BTN_CLEAR_APP_DATA, IDS_BTN_CLEAR_APP_DATA },
  };
  Lang::ApplyCaption(this, IDS_DLG_DEBUG_TITLE);
  Lang::ApplyControlTexts(this, kDebugDlgTexts, _countof(kDebugDlgTexts));

  return TRUE;
}

BOOL DialogDebug::PreTranslateMessage(MSG* pMsg)
{
  tooltip_.RelayEvent(pMsg);

  if ((pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN) &&
      (pMsg->wParam == 'A' || pMsg->wParam == 'a'))
  {
    const bool ctrl_pressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    if (ctrl_pressed)
    {
      CWnd* focus = GetFocus();
      if (focus && focus->GetDlgCtrlID() == IDC_EDIT_DEBUG_LOG)
      {
        auto* edit = static_cast<CEdit*>(GetDlgItem(IDC_EDIT_DEBUG_LOG));
        if (edit)
        {
          edit->SetSel(0, -1);
          return TRUE;
        }
      }
    }
  }

  if (pMsg->message == WM_LBUTTONDOWN || pMsg->message == WM_LBUTTONDBLCLK)
  {
    CWnd* target = CWnd::FromHandle(pMsg->hwnd);
    if (target && target->GetDlgCtrlID() == IDC_EDIT_PRESET_OFF_PROCESS)
    {
      if (GetDlgItem(IDC_EDIT_PRESET_OFF_PROCESS)->IsWindowEnabled())
        BrowseProcessFile();
      return TRUE;
    }
  }

  if (pMsg->message == WM_RBUTTONDOWN)
  {
    CWnd* target = CWnd::FromHandle(pMsg->hwnd);
    if (target && target->GetDlgCtrlID() == IDC_EDIT_PRESET_OFF_PROCESS)
    {
      if (GetDlgItem(IDC_EDIT_PRESET_OFF_PROCESS)->IsWindowEnabled())
        ClearProcessFile();
      return TRUE;
    }
  }

  return CDialogEx::PreTranslateMessage(pMsg);
}

void DialogDebug::OnCancel()
{
  DestroyWindow();
}

void DialogDebug::OnClose()
{
  DestroyWindow();
}

void DialogDebug::OnDestroy()
{
  DevLog::DetachSink(this);

  CWnd* parent = GetParent();
  if (parent && ::IsWindow(parent->GetSafeHwnd()))
    parent->SendMessage(WM_DEV_DEBUG_CLOSED, 0, 0);

  CDialogEx::OnDestroy();
}

void DialogDebug::PostNcDestroy()
{
  CDialogEx::PostNcDestroy();
  delete this;
}

void DialogDebug::AppendDevLog(const CString& line)
{
  if (!::IsWindow(GetSafeHwnd()))
    return;

  CEdit* edit = static_cast<CEdit*>(GetDlgItem(IDC_EDIT_DEBUG_LOG));
  if (!edit)
    return;

  CString current;
  edit->GetWindowText(current);

  if (!current.IsEmpty())
    current += _T("\r\n");
  current += line;

  if (current.GetLength() > kLogMaxChars)
    current = current.Right(kTrimKeepChars);

  edit->SetWindowText(current);
  edit->SetSel(current.GetLength(), current.GetLength());
}

void DialogDebug::InitializeOptions()
{
  if (auto* move = static_cast<CButton*>(GetDlgItem(IDC_CHECK_DBG_MOVE_TRACKER)); move)
  {
    const bool checked = (GLOBAL_OPTION.getInteger(KEY_ENABLE_MOUSE_MOVE_TRACKER) != 0);
    move->SetCheck(checked ? BST_CHECKED : BST_UNCHECKED);
  }

  if (auto* combo = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_DBG_PRESET_COUNT)); combo)
  {
    preset_count_combo_updating_ = true;

    combo->ResetContent();
    for (int value = PRESET_MIN_COUNT; value <= PRESET_MAX_COUNT; ++value)
    {
      CString item;
      item.Format(_T("%d"), value);
      combo->AddString(item);
    }

    int current_count = static_cast<int>(GLOBAL_OPTION.getInteger(KEY_PRESET_COUNT));
    if (current_count < PRESET_MIN_COUNT)
      current_count = PRESET_MIN_COUNT;
    if (current_count > PRESET_MAX_COUNT)
      current_count = PRESET_MAX_COUNT;

    combo->SetCurSel(current_count - PRESET_MIN_COUNT);

    preset_count_combo_updating_ = false;
  }

  if (auto* sync = static_cast<CButton*>(GetDlgItem(IDC_CHECK_SYNC_FILTERKEY)); sync)
  {
    const bool checked = (GLOBAL_OPTION.getInteger(KEY_SYNC_FILTERKEY, 0) != 0);
    sync->SetCheck(checked ? BST_CHECKED : BST_UNCHECKED);
  }

  if (auto* mute = static_cast<CButton*>(GetDlgItem(IDC_CHECK_MUTE_SOUND)); mute)
  {
    const bool checked = (GLOBAL_OPTION.getInteger(KEY_MUTE_SOUND, 0) != 0);
    mute->SetCheck(checked ? BST_CHECKED : BST_UNCHECKED);
  }

  if (auto* off_behavior = static_cast<CButton*>(GetDlgItem(IDC_CHECK_OFF_USE_WINDOWS_DEFAULT)); off_behavior)
    if (off_behavior)
    {
      const bool checked = (GLOBAL_OPTION.getInteger(KEY_OFF_USE_WINDOWS_DEFAULT, 1) != 0);
      off_behavior->SetCheck(checked ? BST_CHECKED : BST_UNCHECKED);
    }

  if (auto* full_screen = static_cast<CButton*>(GetDlgItem(IDC_CHECK_IF_FULL_SCREEN_GAME)); full_screen)
  {
    const bool checked = (GLOBAL_OPTION.getInteger(KEY_IF_FULL_SCREEN_GAME, 0) != 0);
    full_screen->SetCheck(checked ? BST_CHECKED : BST_UNCHECKED);
  }

  if (auto* disable_hotkey = static_cast<CButton*>(GetDlgItem(IDC_CHECK_DISABLE_HOTKEY)); disable_hotkey)
  {
    const bool checked = (GLOBAL_OPTION.getInteger(KEY_DISABLE_HOTKEY, 0) != 0);
    disable_hotkey->SetCheck(checked ? BST_CHECKED : BST_UNCHECKED);
  }

  if (auto* combo = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_LANG)); combo)
  {
    combo->ResetContent();
    combo->AddString(Lang::NativeName(AppLanguage::Korean));
    combo->AddString(Lang::NativeName(AppLanguage::English));
    combo->AddString(Lang::NativeName(AppLanguage::Japanese));
    combo->SetCurSel(static_cast<int>(Lang::GetCurrent()));
  }
}

void DialogDebug::NotifyOptionsChanged()
{
  CWnd* parent = GetParent();
  if (parent && ::IsWindow(parent->GetSafeHwnd()))
    parent->PostMessage(WM_DEV_DEBUG_OPTIONS_CHANGED, 0, 0);
}

void DialogDebug::OnBnClickedCheckDbgMoveTracker()
{
  auto*      button   = static_cast<CButton*>(GetDlgItem(IDC_CHECK_DBG_MOVE_TRACKER));
  const bool checked  = (button && button->GetCheck() == BST_CHECKED);
  const bool previous = (GLOBAL_OPTION.getInteger(KEY_ENABLE_MOUSE_MOVE_TRACKER, 0) != 0);

  if (button && checked && !previous)
  {
    const CString option_key = KEY_ENABLE_MOUSE_MOVE_TRACKER;
    const auto    prompt     = AdminGuard::PromptAdminRestartIfNeeded(this, &option_key);
    if (prompt != AdminGuard::PromptResult::Proceed)
    {
      button->SetCheck(BST_UNCHECKED);
      return;
    }
  }

  GLOBAL_OPTION.set(KEY_ENABLE_MOUSE_MOVE_TRACKER, static_cast<DWORD>(checked));

  if (auto* combo = GetDlgItem(IDC_COMBO_MOVE_SENSITIVITY); combo)
    combo->EnableWindow(checked);

  DevLog::Writef(_T("Option changed: move tracker = %d"), checked ? 1 : 0);
  NotifyOptionsChanged();
}

void DialogDebug::InitializeMouseSensitivity()
{
  auto* combo = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_MOVE_SENSITIVITY));
  if (!combo)
    return;

  combo->ResetContent();
  combo->AddString(Lang::T(IDS_LBL_VERY_LOW));
  combo->AddString(Lang::T(IDS_LBL_LOW));
  combo->AddString(Lang::T(IDS_LBL_NORMAL));
  combo->AddString(Lang::T(IDS_LBL_HIGH));
  combo->AddString(Lang::T(IDS_LBL_VERY_HIGH));

  const auto saved = static_cast<int>(GLOBAL_OPTION.getInteger(KEY_MOUSE_MOVE_SENSITIVITY, 2));
  combo->SetCurSel((saved >= 0 && saved <= 4) ? saved : 2);

  const bool move_enabled = (GLOBAL_OPTION.getInteger(KEY_ENABLE_MOUSE_MOVE_TRACKER) != 0);
  combo->EnableWindow(move_enabled);
}

void DialogDebug::OnCbnSelchangeComboMouseSensitivity()
{
  auto* combo = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_MOVE_SENSITIVITY));
  if (!combo)
    return;

  const int sel = combo->GetCurSel();
  if (sel == CB_ERR)
    return;

  GLOBAL_OPTION.set(KEY_MOUSE_MOVE_SENSITIVITY, static_cast<DWORD>(sel));

  static constexpr LPCTSTR kNames[] = { _T("very_low"), _T("low"), _T("normal"), _T("high"), _T("very_high") };
  DevLog::Writef(_T("Option changed: mouse sensitivity = %s"), kNames[sel]);
  NotifyOptionsChanged();
}

void DialogDebug::OnBnClickedCheckSyncFilterkey()
{
  auto*      button  = static_cast<CButton*>(GetDlgItem(IDC_CHECK_SYNC_FILTERKEY));
  const bool checked = (button && button->GetCheck() == BST_CHECKED);

  GLOBAL_OPTION.set(KEY_SYNC_FILTERKEY, static_cast<DWORD>(checked));
  DevLog::Writef(_T("Option changed: sync filterkey = %d"), checked ? 1 : 0);
  NotifyOptionsChanged();
}

void DialogDebug::OnBnClickedCheckMuteSound()
{
  auto*      button  = static_cast<CButton*>(GetDlgItem(IDC_CHECK_MUTE_SOUND));
  const bool checked = (button && button->GetCheck() == BST_CHECKED);

  GLOBAL_OPTION.set(KEY_MUTE_SOUND, static_cast<DWORD>(checked));
  DevLog::Writef(_T("Option changed: mute sound = %d"), checked ? 1 : 0);
  NotifyOptionsChanged();
}

void DialogDebug::InitializeAutoStart()
{
  auto* check = static_cast<CButton*>(GetDlgItem(IDC_CHECK_AUTO_START));
  if (!check)
    return;

  static constexpr LPCTSTR kRunPath = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run");

  HKEY hKey = nullptr;
  if (::RegOpenKeyEx(HKEY_CURRENT_USER, kRunPath, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
  {
    check->SetCheck(BST_UNCHECKED);
    DevLog::Write(_T("[AutoStart] Run registry open failed. checkbox=OFF"));
    return;
  }

  TCHAR reg_path[MAX_PATH] = {};
  DWORD size               = sizeof(reg_path);
  DWORD type               = 0;
  LONG  result             = ::RegQueryValueEx(hKey, GetAppName(), nullptr, &type, reinterpret_cast<LPBYTE>(reg_path), &size);
  ::RegCloseKey(hKey);

  TCHAR exe_path[MAX_PATH] = {};
  ::GetModuleFileName(nullptr, exe_path, MAX_PATH);

  if (result != ERROR_SUCCESS || type != REG_SZ)
  {
    check->SetCheck(BST_UNCHECKED);
    DevLog::Writef(_T("[AutoStart] Run entry not found. checkbox=OFF, exe=%s"), exe_path);
    return;
  }

  const bool match = (_tcsicmp(reg_path, exe_path) == 0);
  check->SetCheck(match ? BST_CHECKED : BST_UNCHECKED);
  DevLog::Writef(_T("[AutoStart] checkbox=%s, match=%s\r\n  registered=%s\r\n  current=%s"),
                 match ? _T("ON") : _T("OFF"),
                 match ? _T("YES") : _T("NO (exe moved?)"),
                 reg_path, exe_path);
}

void DialogDebug::OnBnClickedCheckAutoStart()
{
  auto*      check   = static_cast<CButton*>(GetDlgItem(IDC_CHECK_AUTO_START));
  const bool enabled = (check && check->GetCheck() == BST_CHECKED);

  static constexpr LPCTSTR kRunPath = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run");

  HKEY hKey = nullptr;
  if (::RegOpenKeyEx(HKEY_CURRENT_USER, kRunPath, 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS)
  {
    DevLog::Writef(_T("[AutoStart] Run registry open failed (KEY_SET_VALUE). error=%lu"), ::GetLastError());
    return;
  }

  if (enabled)
  {
    TCHAR exe_path[MAX_PATH] = {};
    ::GetModuleFileName(nullptr, exe_path, MAX_PATH);
    LONG res = ::RegSetValueEx(hKey, GetAppName(), 0, REG_SZ,
                               reinterpret_cast<const BYTE*>(exe_path),
                               static_cast<DWORD>((_tcslen(exe_path) + 1) * sizeof(TCHAR)));
    DevLog::Writef(_T("[AutoStart] Registered: result=%ld, path=%s"), res, exe_path);
  }
  else
  {
    LONG res = ::RegDeleteValue(hKey, GetAppName());
    DevLog::Writef(_T("[AutoStart] Unregistered: result=%ld"), res);
  }

  ::RegCloseKey(hKey);
}

void DialogDebug::OnBnClickedBtnClearAppData()
{
  const bool ctrl_down = ((::GetKeyState(VK_CONTROL) & 0x8000) != 0) ||
                         ((::GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0);
  if (!ctrl_down)
    return;

  const int ret = MessageBox(Lang::T(IDS_MSG_CLEAR_APP_DATA_CONFIRM),
                             Lang::T(IDS_MSG_CLEAR_APP_DATA_TITLE),
                             MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
  if (ret != IDYES)
    return;

  DevLog::Write(_T("[ClearAppData] Requested. Delegating to main window for filter-key restore + exit."));

  // 필터키 복원·데이터 삭제·종료는 메인 다이얼로그가 소유한다 (X 버튼 종료와 동일 시퀀스).
  if (CWnd* parent = GetParent(); parent && ::IsWindow(parent->GetSafeHwnd()))
    parent->PostMessage(CFilterKeySettingDlg::WM_CLEAR_APP_DATA_AND_EXIT);
}

void DialogDebug::InitializeProcessOffOptions()
{
  const bool enabled = (GLOBAL_OPTION.getInteger(KEY_PROCESS_OFF_ENABLED) != 0);
  const bool restore = (GLOBAL_OPTION.getInteger(KEY_PROCESS_OFF_RESTORE) != 0);
  CString    name    = GLOBAL_OPTION.getString(KEY_PROCESS_OFF_NAME);

  if (auto* chk = static_cast<CButton*>(GetDlgItem(IDC_CHECK_PRESET_OFF_PROCESS)); chk)
    chk->SetCheck(enabled ? BST_CHECKED : BST_UNCHECKED);

  if (auto* edit = static_cast<CEdit*>(GetDlgItem(IDC_EDIT_PRESET_OFF_PROCESS)); edit)
  {
    edit->SetWindowText(name);
    edit->SetReadOnly(TRUE);
  }

  if (auto* chk = static_cast<CButton*>(GetDlgItem(IDC_CHECK_PRESET_OFF_PROCESS_RESTORE)); chk)
    chk->SetCheck(restore ? BST_CHECKED : BST_UNCHECKED);

  UpdateProcessOffControlStates();
}

void DialogDebug::UpdateProcessOffControlStates()
{
  const bool enabled = (GLOBAL_OPTION.getInteger(KEY_PROCESS_OFF_ENABLED) != 0);

  if (auto* edit = GetDlgItem(IDC_EDIT_PRESET_OFF_PROCESS); edit)
    edit->EnableWindow(enabled);
  if (auto* chk = GetDlgItem(IDC_CHECK_PRESET_OFF_PROCESS_RESTORE); chk)
    chk->EnableWindow(enabled);
}

void DialogDebug::OnBnClickedCheckPresetOffProcess()
{
  auto*      check    = static_cast<CButton*>(GetDlgItem(IDC_CHECK_PRESET_OFF_PROCESS));
  const bool checked  = (check && check->GetCheck() == BST_CHECKED);
  const bool previous = (GLOBAL_OPTION.getInteger(KEY_PROCESS_OFF_ENABLED, 0) != 0);

  if (check && checked && !previous)
  {
    const CString option_key = KEY_PROCESS_OFF_ENABLED;
    const auto    prompt     = AdminGuard::PromptAdminRestartIfNeeded(this, &option_key);
    if (prompt != AdminGuard::PromptResult::Proceed)
    {
      check->SetCheck(BST_UNCHECKED);
      return;
    }
  }

  GLOBAL_OPTION.set(KEY_PROCESS_OFF_ENABLED, static_cast<DWORD>(checked));
  UpdateProcessOffControlStates();
  DevLog::Writef(_T("Option changed: process off = %d"), checked ? 1 : 0);
  NotifyOptionsChanged();
}

void DialogDebug::OnBnClickedCheckPresetOffProcessRestore()
{
  auto*      check   = static_cast<CButton*>(GetDlgItem(IDC_CHECK_PRESET_OFF_PROCESS_RESTORE));
  const bool checked = (check && check->GetCheck() == BST_CHECKED);

  GLOBAL_OPTION.set(KEY_PROCESS_OFF_RESTORE, static_cast<DWORD>(checked));
  DevLog::Writef(_T("Option changed: process off restore = %d"), checked ? 1 : 0);
  NotifyOptionsChanged();
}

void DialogDebug::OnBnClickedCheckIfFullScreenGame()
{
  auto* check = static_cast<CButton*>(GetDlgItem(IDC_CHECK_IF_FULL_SCREEN_GAME));
  if (!check)
    return;

  const bool request_enable = (check->GetCheck() == BST_CHECKED);
  const bool previous       = (GLOBAL_OPTION.getInteger(KEY_IF_FULL_SCREEN_GAME, 0) != 0);

  if (request_enable && !previous)
  {
    const CString option_key = KEY_IF_FULL_SCREEN_GAME;
    const auto    prompt     = AdminGuard::PromptAdminRestartIfNeeded(this, &option_key);
    if (prompt != AdminGuard::PromptResult::Proceed)
    {
      check->SetCheck(BST_UNCHECKED);
      return;
    }
  }

  if (request_enable)
  {
    const int answer = AfxMessageBox(Lang::T(IDS_MSG_FULLSCREEN_GAME_WARNING),
                                     MB_OKCANCEL | MB_ICONWARNING);
    if (answer != IDOK)
    {
      check->SetCheck(BST_UNCHECKED);
      DevLog::Write(_T("User cancelled enabling full screen game mode"));
      return;
    }
  }

  GLOBAL_OPTION.set(KEY_IF_FULL_SCREEN_GAME, static_cast<DWORD>(request_enable));
  const bool is_elevated = FilterKey::IsProcessElevatedNow();
  DevLog::Writef(_T("Option changed: fullscreen game raw input mode = %d (admin=%d)"),
                 request_enable ? 1 : 0, is_elevated ? 1 : 0);
  NotifyOptionsChanged();
}

void DialogDebug::OnBnClickedCheckDisableHotkey()
{
  auto* check = static_cast<CButton*>(GetDlgItem(IDC_CHECK_DISABLE_HOTKEY));
  if (!check)
    return;

  const bool checked = (check->GetCheck() == BST_CHECKED);

  GLOBAL_OPTION.set(KEY_DISABLE_HOTKEY, static_cast<DWORD>(checked));

  PresetOption option(PRESET_OFF);
  DWORD        value = checked ? WINDOW_FILTER_FLAG & ~(FKF_HOTKEYACTIVE | FKF_CONFIRMHOTKEY | FKF_HOTKEYSOUND)
                               : WINDOW_FILTER_FLAG;
  option.set(KEY_FILTER_FLAG, value);

  DevLog::Writef(_T("Option changed: disable hotkey = %d"), checked ? 1 : 0);
  NotifyOptionsChanged();
}

void DialogDebug::OnEnKillFocusEditPresetOffProcess()
{
  auto* edit = GetDlgItem(IDC_EDIT_PRESET_OFF_PROCESS);
  if (!edit)
    return;

  CString text;
  edit->GetWindowText(text);
  text.Trim();

  GLOBAL_OPTION.set(KEY_PROCESS_OFF_NAME, text);
  DevLog::Writef(_T("Option changed: process off name = %s"), static_cast<LPCTSTR>(text));
  NotifyOptionsChanged();
}

void DialogDebug::BrowseProcessFile()
{
  CFileDialog dlg(TRUE, _T("exe"), nullptr, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
                  Lang::T(IDS_FLT_EXE_FILES), this);
  if (dlg.DoModal() != IDOK)
    return;

  CString name = dlg.GetFileName();
  if (auto* edit = GetDlgItem(IDC_EDIT_PRESET_OFF_PROCESS); edit)
    edit->SetWindowText(name);

  GLOBAL_OPTION.set(KEY_PROCESS_OFF_NAME, name);
  DevLog::Writef(_T("Process off target set: %s (path: %s)"),
                 static_cast<LPCTSTR>(name), static_cast<LPCTSTR>(dlg.GetPathName()));
  NotifyOptionsChanged();
}

void DialogDebug::ClearProcessFile()
{
  CString current = GLOBAL_OPTION.getString(KEY_PROCESS_OFF_NAME);
  if (current.Trim().IsEmpty())
    return;

  if (AfxMessageBox(Lang::T(IDS_MSG_RESET_PROCESS_CONFIRM), MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;

  if (auto* edit = GetDlgItem(IDC_EDIT_PRESET_OFF_PROCESS); edit)
    edit->SetWindowText(_T(""));

  GLOBAL_OPTION.set(KEY_PROCESS_OFF_NAME, CString());
  DevLog::Write(_T("Process off target cleared"));
  NotifyOptionsChanged();
}

void DialogDebug::OnBnClickedCheckOffUseWindowsDefault()
{
  auto*      button  = static_cast<CButton*>(GetDlgItem(IDC_CHECK_OFF_USE_WINDOWS_DEFAULT));
  const bool checked = (button && button->GetCheck() == BST_CHECKED);

  GLOBAL_OPTION.set(KEY_OFF_USE_WINDOWS_DEFAULT, static_cast<DWORD>(checked));
  DevLog::Writef(_T("Option changed: off use windows default = %d"), checked ? 1 : 0);
  NotifyOptionsChanged();
}

void DialogDebug::OnBnClickedCheckEnablePresetOsd()
{
  SaveOsdOptionsFromUI();
  DevLog::Writef(_T("Option changed: preset osd = %d"),
                 GLOBAL_OPTION.getInteger(KEY_ENABLE_PRESET_OSD, 0) ? 1 : 0);
  NotifyOptionsChanged();
}

void DialogDebug::OnCbnSelchangeComboPresetOsdCorner()
{
  SaveOsdOptionsFromUI();
  NotifyOptionsChanged();
}

void DialogDebug::OnBnClickedRadioPresetOsdKeep()
{
  SaveOsdOptionsFromUI();
  NotifyOptionsChanged();
}

void DialogDebug::OnBnClickedRadioPresetOsd3sec()
{
  SaveOsdOptionsFromUI();
  NotifyOptionsChanged();
}

void DialogDebug::OnCbnSelchangeComboPresetOsdSize()
{
  SaveOsdOptionsFromUI();
  NotifyOptionsChanged();
}

void DialogDebug::OnCbnSelchangeComboDbgPresetCount()
{
  if (preset_count_combo_updating_)
    return;

  auto* combo = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_DBG_PRESET_COUNT));
  if (!combo)
    return;

  const int selected = combo->GetCurSel();
  if (selected == CB_ERR)
    return;

  int current_count = static_cast<int>(GLOBAL_OPTION.getInteger(KEY_PRESET_COUNT, PRESET_MIN_COUNT));
  if (current_count < PRESET_MIN_COUNT)
    current_count = PRESET_MIN_COUNT;
  if (current_count > PRESET_MAX_COUNT)
    current_count = PRESET_MAX_COUNT;

  const int selected_count = PRESET_MIN_COUNT + selected;
  if (selected_count == current_count)
    return;

  CString message;
  message.Format(Lang::T(IDS_FMT_PRESET_COUNT_RESTART), selected_count);

  const int answer = AfxMessageBox(message, MB_ICONQUESTION | MB_YESNO);
  if (answer != IDYES)
  {
    preset_count_combo_updating_ = true;
    combo->SetCurSel(current_count - PRESET_MIN_COUNT);
    preset_count_combo_updating_ = false;
    return;
  }

  GLOBAL_OPTION.set(KEY_PRESET_COUNT, static_cast<DWORD>(selected_count));
  DevLog::Writef(_T("Option changed: preset_count = %d (restart accepted)"), selected_count);

  if (!FilterKey::RestartProgram(this))
  {
    AfxMessageBox(Lang::T(IDS_MSG_RESTART_FAILED));
    return;
  }
}

void DialogDebug::OnCbnSelchangeComboLang()
{
  auto* combo = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_LANG));
  if (!combo)
    return;

  const int sel = combo->GetCurSel();
  if (sel == CB_ERR)
    return;

  const int current = static_cast<int>(Lang::GetCurrent());
  if (sel == current)
    return;

  const auto    target_lang = static_cast<AppLanguage>(sel);
  CString       msg         = Lang::T(IDS_MSG_LANGUAGE_RESTART_CONFIRM);
  const CString target_msg  = Lang::TFor(target_lang, IDS_MSG_LANGUAGE_RESTART_CONFIRM);
  if (!target_msg.IsEmpty() && target_msg != msg)
    msg += _T("\r\n\r\n") + target_msg;

  const int answer = AfxMessageBox(msg, MB_YESNO | MB_ICONQUESTION);
  if (answer != IDYES)
  {
    combo->SetCurSel(current);
    return;
  }

  GLOBAL_OPTION.set(KEY_LANGUAGE, static_cast<DWORD>(sel));
  DevLog::Writef(_T("Option changed: language = %d (restart accepted)"), sel);

  if (!FilterKey::RestartProgram(this))
  {
    AfxMessageBox(Lang::T(IDS_MSG_RESTART_FAILED));
    return;
  }
}

void DialogDebug::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
  CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);

  if (pScrollBar && pScrollBar->GetDlgCtrlID() == IDC_SLIDER_PRESET_OSD_ALPHA)
  {
    SaveOsdOptionsFromUI();
    NotifyOptionsChanged();
  }
}

void DialogDebug::InitializeOsdOptions()
{
  if (auto* check = static_cast<CButton*>(GetDlgItem(IDC_CHECK_ENABLE_PRESET_OSD)); check)
    check->SetCheck(GLOBAL_OPTION.getInteger(KEY_ENABLE_PRESET_OSD, 0) ? BST_CHECKED : BST_UNCHECKED);

  if (auto* combo = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_PRESET_OSD_CORNER)); combo)
  {
    combo->ResetContent();
    combo->AddString(Lang::T(IDS_LBL_CORNER_TL));
    combo->AddString(Lang::T(IDS_LBL_CORNER_TR));
    combo->AddString(Lang::T(IDS_LBL_CORNER_BL));
    combo->AddString(Lang::T(IDS_LBL_CORNER_BR));

    const int idx = UserPresetOSD::ClampPosition(static_cast<int>(GLOBAL_OPTION.getInteger(
        KEY_PRESET_OSD_CORNER, static_cast<DWORD>(UserPresetOSD::Corner::BottomRight))));
    combo->SetCurSel(idx);
  }

  if (auto* slider = static_cast<CSliderCtrl*>(GetDlgItem(IDC_SLIDER_PRESET_OSD_ALPHA)); slider)
  {
    slider->SetRange(0, 100);
    slider->SetTicFreq(10);
    const int stored = UserPresetOSD::ClampAlpha(static_cast<int>(GLOBAL_OPTION.getInteger(KEY_PRESET_OSD_ALPHA, 220)));
    slider->SetPos((stored * 100) / 255);
  }

  if (auto* combo = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_PRESET_OSD_SIZE)); combo)
  {
    combo->ResetContent();
    for (int value = 1; value <= 10; ++value)
    {
      CString item;
      item.Format(_T("%d"), value);
      combo->AddString(item);
    }

    const int size_percent = UserPresetOSD::ClampSizePercent(
        static_cast<int>(GLOBAL_OPTION.getInteger(KEY_PRESET_OSD_SIZE, 5)));
    combo->SetCurSel(size_percent - 1);
  }

  const bool keep = (GLOBAL_OPTION.getInteger(KEY_PRESET_OSD_KEEP_VISIBLE, 0) != 0);
  if (auto* keep_radio = static_cast<CButton*>(GetDlgItem(IDC_RADIO_PRESET_OSD_KEEP)); keep_radio)
    keep_radio->SetCheck(keep ? BST_CHECKED : BST_UNCHECKED);
  if (auto* auto_radio = static_cast<CButton*>(GetDlgItem(IDC_RADIO_PRESET_OSD_3SEC)); auto_radio)
    auto_radio->SetCheck(keep ? BST_UNCHECKED : BST_CHECKED);
}

void DialogDebug::SaveOsdOptionsFromUI()
{
  bool enabled = false;
  if (auto* check = static_cast<CButton*>(GetDlgItem(IDC_CHECK_ENABLE_PRESET_OSD)); check)
    enabled = (check->GetCheck() == BST_CHECKED);

  int corner = static_cast<int>(UserPresetOSD::Corner::BottomRight);
  if (auto* combo = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_PRESET_OSD_CORNER)); combo)
  {
    const int sel = combo->GetCurSel();
    if (sel != CB_ERR)
      corner = UserPresetOSD::ClampPosition(sel);
  }

  int alpha = 220;
  if (auto* slider = static_cast<CSliderCtrl*>(GetDlgItem(IDC_SLIDER_PRESET_OSD_ALPHA)); slider)
    alpha = UserPresetOSD::ClampAlpha((slider->GetPos() * 255) / 100);

  bool keep_visible = false;
  if (auto* keep_radio = static_cast<CButton*>(GetDlgItem(IDC_RADIO_PRESET_OSD_KEEP)); keep_radio)
    keep_visible = (keep_radio->GetCheck() == BST_CHECKED);

  int size_percent = 5;
  if (auto* combo = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_PRESET_OSD_SIZE)); combo)
  {
    const int sel = combo->GetCurSel();
    if (sel != CB_ERR)
      size_percent = sel + 1;
  }
  size_percent = UserPresetOSD::ClampSizePercent(size_percent);

  GLOBAL_OPTION.set(KEY_ENABLE_PRESET_OSD, static_cast<DWORD>(enabled));
  GLOBAL_OPTION.set(KEY_PRESET_OSD_CORNER, static_cast<DWORD>(corner));
  GLOBAL_OPTION.set(KEY_PRESET_OSD_ALPHA, static_cast<DWORD>(alpha));
  GLOBAL_OPTION.set(KEY_PRESET_OSD_KEEP_VISIBLE, static_cast<DWORD>(keep_visible));
  GLOBAL_OPTION.set(KEY_PRESET_OSD_SIZE, static_cast<DWORD>(size_percent));

  UserPresetOSD::RefreshDisplay();
}
