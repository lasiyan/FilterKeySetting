// clang-format off
#include "pch.h"
#include "DialogDebug.h"
#include "FilterKeySetting.h"
#include "UserAdminGuard.hpp"
#include "UserFilterKey.hpp"
#include "UserPresetOSD.hpp"
#include "afxdialogex.h"
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
ON_EN_KILLFOCUS(IDC_EDIT_PRESET_OFF_PROCESS, &DialogDebug::OnEnKillFocusEditPresetOffProcess)
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
  static constexpr LPCTSTR kLabels[] = { _T("매우 낮음"), _T("낮음"), _T("보통"), _T("높음"), _T("매우 높음") };
  for (auto& label : kLabels)
    combo->AddString(label);

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

  static constexpr LPCTSTR kNames[] = { _T("매우 낮음"), _T("낮음"), _T("보통"), _T("높음"), _T("매우 높음") };
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
    const int answer = AfxMessageBox(_T("적용 전 주의 사항\r\n"
                                        "- 전체화면이 아닌 창모드 사용 시 해당 기능은 필요하지 않습니다.\r\n"
                                        "- HotKey 방식보다 보안 탐지에 민감하게 받아들여질 수 있습니다.\r\n"
                                        "- 해당 기능 사용 대신 가능한 창모드를 권장드립니다.\r\n"
                                        "- 추후 안전성이 확인되면 해당 메세지는 제거될 예정입니다.\r\n"
                                        "\r\n기능을 활성화 하시겠습니까?"),
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
                  _T("실행 파일 (*.exe)|*.exe|모든 파일 (*.*)|*.*||"), this);
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

  if (AfxMessageBox(_T("등록된 프로세스를 초기화하시겠습니까?"), MB_YESNO | MB_ICONQUESTION) != IDYES)
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
  message.Format(_T("프리셋 개수를 %d개로 변경하려면 프로그램 재시작이 필요합니다.\r\n"
                    "지금 다시 시작하시겠습니까?"),
                 selected_count);

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
    AfxMessageBox(_T("프로그램 재시작에 실패했습니다."));
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
    combo->AddString(_T("좌측 상단"));
    combo->AddString(_T("우측 상단"));
    combo->AddString(_T("좌측 하단"));
    combo->AddString(_T("우측 하단"));

    const int idx = UserPresetOSD::ClampPosition(static_cast<int>(GLOBAL_OPTION.getInteger(
        KEY_PRESET_OSD_CORNER, static_cast<DWORD>(UserPresetOSD::Corner::BottomRight))));
    combo->SetCurSel(idx);
  }

  if (auto* slider = static_cast<CSliderCtrl*>(GetDlgItem(IDC_SLIDER_PRESET_OSD_ALPHA)); slider)
  {
    slider->SetRange(0, 255);
    slider->SetTicFreq(15);
    slider->SetPos(
        UserPresetOSD::ClampAlpha(static_cast<int>(GLOBAL_OPTION.getInteger(KEY_PRESET_OSD_ALPHA, 220))));
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
    alpha = UserPresetOSD::ClampAlpha(slider->GetPos());

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
