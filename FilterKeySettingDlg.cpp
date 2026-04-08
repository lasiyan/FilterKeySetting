
// FilterKeySettingDlg.cpp : implementation file
//

// clang-format off
#include "pch.h"
#include "framework.h"
#include "FilterKeySetting.h"
#include "FilterKeySettingDlg.h"
#include "afxdialogex.h"

#include "DialogKeyBinding.h"
#include "DialogDebug.h"
#include "DialogRename.h"
#include "UserDevLog.hpp"
#include "UserFilterKey.hpp"
#include "UserKeyBinding.hpp"
#include "UserPlaySound.hpp"
#include "UserAdminGuard.hpp"
#include "UserPresetOSD.hpp"
#include "UserPresetService.hpp"
#include <algorithm>
// clang-format on

namespace {
UINT ModifierMaskForVirtualKey(UINT vk)
{
  switch (vk)
  {
    case VK_SHIFT:
    case VK_LSHIFT:
    case VK_RSHIFT:
      return MOD_SHIFT;
    case VK_CONTROL:
    case VK_LCONTROL:
    case VK_RCONTROL:
      return MOD_CONTROL;
    case VK_MENU:
    case VK_LMENU:
    case VK_RMENU:
      return MOD_ALT;
    case VK_LWIN:
    case VK_RWIN:
      return MOD_WIN;
    default:
      return 0;
  }
}

bool ReadDwordFromEdit(CDialogEx* dlg, int control_id, DWORD* out)
{
  if (!dlg || !out)
    return false;

  BOOL ok = FALSE;
  UINT u  = dlg->GetDlgItemInt(control_id, &ok, FALSE);
  if (!ok)
  {
    AfxMessageBox(_T("숫자 형식이 올바르지 않습니다."));
    if (auto* ctrl = dlg->GetDlgItem(control_id); ctrl)
      ctrl->SetFocus();
    return false;
  }

  *out = static_cast<DWORD>(u);
  return true;
}

bool ReadPresetValuesFromEdits(CDialogEx* dlg, PresetValues* out_values)
{
  if (!out_values)
    return false;

  PresetValues values = {};
  if (!ReadDwordFromEdit(dlg, IDC_EDIT_ACCEPT_DELAY, &values.accept_delay))
    return false;
  if (!ReadDwordFromEdit(dlg, IDC_EDIT_REPEAT_DELAY, &values.repeat_delay))
    return false;
  if (!ReadDwordFromEdit(dlg, IDC_EDIT_REPEAT_RATE, &values.repeat_rate))
    return false;

  *out_values = values;
  return true;
}

void AllowSingleInstanceActivateMessageIfElevated(HWND hwnd)
{
  if (!::IsWindow(hwnd))
    return;

  if (!FilterKey::IsProcessElevatedNow())
    return;

  if (::ChangeWindowMessageFilterEx(hwnd,
                                    CFilterKeySettingDlg::WM_ACTIVATE_EXISTING_INST,
                                    MSGFLT_ALLOW,
                                    nullptr) == FALSE)
  {
    const DWORD err = ::GetLastError();
    DevLog::Writef(_T("[SingleInstance] ChangeWindowMessageFilterEx failed: err=%lu"), err);
  }
}
}  // namespace

#ifdef _DEBUG
  #define new DEBUG_NEW
#endif

// CFilterKeySettingDlg dialog

CFilterKeySettingDlg::CFilterKeySettingDlg(CWnd* pParent /*=nullptr*/) : CDialogEx(IDD_FILTERKEYSETTING_DIALOG, pParent)
{
  m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CFilterKeySettingDlg::~CFilterKeySettingDlg()
{
}

void CFilterKeySettingDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CFilterKeySettingDlg, CDialogEx)
ON_WM_PAINT()
ON_WM_QUERYDRAGICON()
ON_WM_SIZE()
ON_WM_TIMER()
ON_WM_CTLCOLOR()
ON_WM_DESTROY()
ON_WM_HOTKEY()
ON_MESSAGE(WM_INPUT, &CFilterKeySettingDlg::OnRawInput)
ON_MESSAGE(WM_TRAYICON_MSG, &CFilterKeySettingDlg::OnTrayIcon)
ON_MESSAGE(WM_MOUSE_TRACKER_TRIGGERED, &CFilterKeySettingDlg::OnMouseTrackerTriggered)
ON_MESSAGE(WM_DEV_DEBUG_CLOSED, &CFilterKeySettingDlg::OnDebugDialogClosed)
ON_MESSAGE(WM_DEV_DEBUG_OPTIONS_CHANGED, &CFilterKeySettingDlg::OnDebugOptionsChanged)
ON_MESSAGE(CFilterKeySettingDlg::WM_ACTIVATE_EXISTING_INST, &CFilterKeySettingDlg::OnActivateExisting)
ON_COMMAND_RANGE(CFilterKeySettingDlg::dynamic_preset_button_base_,
                 CFilterKeySettingDlg::dynamic_preset_button_base_ + PRESET_MAX_COUNT - 1,
                 &CFilterKeySettingDlg::OnCommandPresetButton)
ON_EN_SETFOCUS(IDC_EDIT_TESTING, &CFilterKeySettingDlg::OnEnSetFocusTesting)
ON_EN_KILLFOCUS(IDC_EDIT_TESTING, &CFilterKeySettingDlg::OnEnKillFocusTesting)
ON_EN_SETFOCUS(IDC_EDIT_TOGGLE_KEYBIND, &CFilterKeySettingDlg::OnEnSetFocusToggleKeybind)
ON_BN_CLICKED(IDC_CHECK_EDIT_MODE, &CFilterKeySettingDlg::OnBnClickedCheckEditMode)
ON_BN_CLICKED(IDC_CHECK_RESTORE_SETTING, &CFilterKeySettingDlg::OnBnClickedCheckRestoreSetting)
ON_BN_CLICKED(IDC_CHECK_DISABLE_HOTKEY, &CFilterKeySettingDlg::OnBnClickedCheckDisableHotkey)
ON_BN_CLICKED(IDC_CHECK_MOVE_TO_TRAY, &CFilterKeySettingDlg::OnBnClickedCheckMoveToTray)
ON_BN_CLICKED(IDC_CHECK_ENABLE_KEYBIND, &CFilterKeySettingDlg::OnBnClickedCheckEnableKeybind)
ON_BN_CLICKED(IDC_CHECK_ENABLE_TOGGLE_KEYBIND, &CFilterKeySettingDlg::OnBnClickedCheckEnableToggleKeybind)
ON_BN_CLICKED(IDC_CHECK_SET_MOUSE_DBLCLICK_TRACKER, &CFilterKeySettingDlg::OnBnClickedCheckSetMouseDblclickTracker)
ON_BN_CLICKED(IDC_CHECK_DISABLE_WITH_ESC, &CFilterKeySettingDlg::OnBnClickedCheckDisableWithEsc)
END_MESSAGE_MAP()

// CFilterKeySettingDlg message handlers

BOOL CFilterKeySettingDlg::OnInitDialog()
{
  CDialogEx::OnInitDialog();

  // Set the icon for this dialog.  The framework does this automatically
  //  when the application's main window is not a dialog
  SetIcon(m_hIcon, TRUE);   // Set big icon
  SetIcon(m_hIcon, FALSE);  // Set small icon

  AllowSingleInstanceActivateMessageIfElevated(GetSafeHwnd());

  if (!EnsureAdminGuardOnStartup())
    return FALSE;

  FilterKey::BackupCurrentFilterKeysToOption();

  InitializePresetCount();
  BuildPresetButtons();
  LayoutDynamicControls();

  // Get last activated preset (with clamping), then sync UI only.
  auto last_set = static_cast<int>(GLOBAL_OPTION.getInteger(KEY_LAST_PRESET));
  if (PRESET_IS_VALID(last_set) == false)
    last_set = PRESET_OFF;
  ActivatePreset(last_set, FALSE, FALSE, _T("Init (last preset)"));

  // Update Interface
  UpdateOption(FALSE);
  ValidateActiveHotkeysAndAlert();

  // Tooltip
  tooltip_.Initialize(this);
  for (int preset = PRESET_OFF; preset < preset_count_; ++preset)
    tooltip_.RegisterPreset(GetDlgItem(GetPresetButtonControlId(preset)));

  // Kill Focus Edit control
  OnEnKillFocusTesting();
  EnsureEditModeHintLabel();
  UpdateEditModeCaption();

  // Set initial focus to the OFF preset button if available
  if (auto* off_button = GetDlgItem(GetPresetButtonControlId(PRESET_OFF));
      off_button && ::IsWindow(off_button->GetSafeHwnd()) &&
      off_button->IsWindowEnabled())
  {
    off_button->SetFocus();
    return FALSE;
  }

  return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CFilterKeySettingDlg::OnPaint()
{
  if (IsIconic())
  {
    CPaintDC dc(this);  // device context for painting

    SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

    // Center icon in client rectangle
    int   cxIcon = GetSystemMetrics(SM_CXICON);
    int   cyIcon = GetSystemMetrics(SM_CYICON);
    CRect rect;
    GetClientRect(&rect);
    int x = (rect.Width() - cxIcon + 1) / 2;
    int y = (rect.Height() - cyIcon + 1) / 2;

    // Draw the icon
    dc.DrawIcon(x, y, m_hIcon);
  }
  else
  {
    CDialogEx::OnPaint();
  }
}

// The system calls this function to obtain the cursor to display while the user
// drags
//  the minimized window.
HCURSOR CFilterKeySettingDlg::OnQueryDragIcon()
{
  return static_cast<HCURSOR>(m_hIcon);
}

void CFilterKeySettingDlg::OnDestroy()
{
  if (bg_esc_watch_active_)
  {
    KillTimer(TIMER_BG_ESC_WATCH);
    bg_esc_watch_active_ = false;
    bg_esc_prev_down_    = false;
  }

  if (process_watch_active_)
  {
    KillTimer(TIMER_PROCESS_WATCHER);
    process_watch_active_ = false;
  }

  if (debug_dialog_ && ::IsWindow(debug_dialog_->GetSafeHwnd()))
  {
    debug_dialog_->DestroyWindow();
    debug_dialog_ = nullptr;
  }

  mouse_tracker_.Release();

  if (GLOBAL_OPTION.getInteger(KEY_RESTORE_SETTING))
    ActivatePreset(PRESET_OFF, FALSE, FALSE, _T("Destroy (restore off)"));

  UnregisterPresetHotkeys();
  RemoveTrayIcon();

  CDialogEx::OnDestroy();
}

void CFilterKeySettingDlg::OnHotKey(UINT nHotKeyId, UINT nKey1, UINT nKey2)
{
  if (!HandleResolvedHotkeyId(nHotKeyId, false))
    CDialogEx::OnHotKey(nHotKeyId, nKey1, nKey2);
}

LRESULT CFilterKeySettingDlg::OnRawInput(WPARAM wParam, LPARAM lParam)
{
  UNREFERENCED_PARAMETER(wParam);

  if (!raw_input_registered_)
    return 0;

  UINT size = 0;
  if (::GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER)) != 0 || size == 0)
    return 0;

  std::vector<BYTE> buffer(size);
  if (::GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, buffer.data(), &size, sizeof(RAWINPUTHEADER)) != size)
    return 0;

  const auto* raw = reinterpret_cast<const RAWINPUT*>(buffer.data());
  if (raw->header.dwType != RIM_TYPEKEYBOARD)
    return 0;

  const RAWKEYBOARD& keyboard = raw->data.keyboard;
  UINT               vk       = keyboard.VKey;
  if (vk == 0 || vk == 255)
    return 0;

  if (vk == VK_SHIFT)
    vk = ::MapVirtualKey(keyboard.MakeCode, MAPVK_VSC_TO_VK_EX);
  else if (vk == VK_CONTROL)
    vk = (keyboard.Flags & RI_KEY_E0) ? VK_RCONTROL : VK_LCONTROL;
  else if (vk == VK_MENU)
    vk = (keyboard.Flags & RI_KEY_E0) ? VK_RMENU : VK_LMENU;

  if (vk >= raw_input_key_down_.size())
    return 0;

  const bool key_down = ((keyboard.Flags & RI_KEY_BREAK) == 0);
  UpdateRawInputModifierState(vk, key_down);

  if (!key_down)
  {
    raw_input_key_down_[vk] = false;
    return 0;
  }

  if (raw_input_key_down_[vk])
    return 0;
  raw_input_key_down_[vk] = true;

  if (KeyBinding::IsModifierVirtualKey(vk))
    return 0;

  const int hotkey_id = ResolveRawInputHotkeyId(vk, raw_input_modifiers_down_);
  if (hotkey_id >= 0)
    HandleResolvedHotkeyId(static_cast<UINT>(hotkey_id), true);

  return 0;
}

void CFilterKeySettingDlg::OnTimer(UINT_PTR nIDEvent)
{
  if (nIDEvent == TIMER_BG_ESC_WATCH)
  {
    const bool esc_down  = ((::GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0);
    const bool edge_down = (esc_down && !bg_esc_prev_down_);
    bg_esc_prev_down_    = esc_down;

    if (!edge_down)
      return;

    if (IsDialogForeground())
      return;

    if (const UINT modifiers = KeyBinding::CurrentModifiers(); modifiers != 0)
      return;

    if (KeyBinding::IsActiveHotkeyAssigned(VK_ESCAPE, 0, preset_count_))
      return;

    if (GLOBAL_OPTION.getInteger(KEY_LAST_PRESET) == static_cast<DWORD>(PRESET_OFF))
      return;

    ResetEditMode();
    ActivatePreset(PRESET_OFF, FALSE, TRUE, _T("Background ESC"));
    return;
  }

  if (nIDEvent == TIMER_PROCESS_WATCHER)
  {
    TickProcessWatcher();
    return;
  }

  CDialogEx::OnTimer(nIDEvent);
}

void CFilterKeySettingDlg::OnSize(UINT nType, int cx, int cy)
{
  CDialogEx::OnSize(nType, cx, cy);

  if (nType == SIZE_MINIMIZED)
  {
    if ((GLOBAL_OPTION.getInteger(KEY_MOVE_TO_TRAY) != 0) && AddTrayIcon())
      ShowWindow(SW_HIDE);
  }
}

HBRUSH CFilterKeySettingDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
  HBRUSH brush = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

  if (!pWnd || !edit_mode_hint_label_ready_)
    return brush;

  if (!::IsWindow(edit_mode_hint_label_.GetSafeHwnd()))
    return brush;

  if (pWnd->GetSafeHwnd() != edit_mode_hint_label_.GetSafeHwnd())
    return brush;

  if (nCtlColor == CTLCOLOR_STATIC)
  {
    pDC->SetBkMode(TRANSPARENT);
    pDC->SetTextColor(edit_mode_hint_text_color_);
  }

  return brush;
}

LRESULT CFilterKeySettingDlg::OnTrayIcon(WPARAM wParam, LPARAM lParam)
{
  if (wParam != TRAY_ICON_ID)
    return 0;

  if (lParam == WM_LBUTTONDBLCLK || lParam == WM_LBUTTONUP)
  {
    RestoreFromTray();
  }
  else if (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU)
  {
    CMenu popup;
    if (popup.CreatePopupMenu())
    {
      popup.AppendMenu(MF_STRING, IDM_TRAY_OPEN, _T("열기"));
      popup.AppendMenu(MF_SEPARATOR);

      const UINT active_preset = GLOBAL_OPTION.getInteger(KEY_LAST_PRESET);
      for (int preset = PRESET_OFF; preset < preset_count_; ++preset)
      {
        const UINT menu_id = IDM_TRAY_PRESET_BASE + static_cast<UINT>(preset);
        const UINT flags   = MF_STRING | (active_preset == static_cast<UINT>(preset) ? MF_CHECKED : 0);
        popup.AppendMenu(flags, menu_id, PresetOption::TAG(preset));
      }

      popup.AppendMenu(MF_SEPARATOR);
      popup.AppendMenu(MF_STRING, IDM_TRAY_EXIT, _T("종료"));

      POINT cursor;
      GetCursorPos(&cursor);
      SetForegroundWindow();

      const UINT cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_NONOTIFY, cursor.x, cursor.y, this);
      if (cmd == IDM_TRAY_OPEN)
      {
        RestoreFromTray();
      }
      else if (cmd == IDM_TRAY_EXIT)
      {
        RemoveTrayIcon();
        PostMessage(WM_CLOSE);
      }
      else if (cmd >= IDM_TRAY_PRESET_BASE && cmd < IDM_TRAY_PRESET_BASE + static_cast<UINT>(preset_count_))
      {
        const int selected_preset = static_cast<int>(cmd - IDM_TRAY_PRESET_BASE);
        ResetEditMode();
        last_selected_ = selected_preset;
        ActivatePreset(selected_preset, FALSE, TRUE, _T("Tray menu"));
      }

      PostMessage(WM_NULL);
    }
  }

  return 0;
}

LRESULT CFilterKeySettingDlg::OnMouseTrackerTriggered(WPARAM wParam, LPARAM lParam)
{
  UNREFERENCED_PARAMETER(lParam);

  const auto trigger = static_cast<MouseTrackerTrigger>(wParam);

  if (trigger == MouseTrackerTrigger::MoveDistance)
  {
    if (GLOBAL_OPTION.getInteger(KEY_ENABLE_MOUSE_MOVE_TRACKER) == 0)
      return 0;
  }
  else if (trigger == MouseTrackerTrigger::DoubleClick)
  {
    if (GLOBAL_OPTION.getInteger(KEY_ENABLE_MOUSE_DBLCLICK_TRACKER) == 0)
      return 0;
  }
  else
    return 0;

  if (IsDialogForeground())
    return 0;

  if (GLOBAL_OPTION.getInteger(KEY_LAST_PRESET) == static_cast<DWORD>(PRESET_OFF))
    return 0;

  ResetEditMode();
  ActivatePreset(PRESET_OFF, FALSE, TRUE,
                 trigger == MouseTrackerTrigger::MoveDistance ? _T("Mouse Move tracker") : _T("Mouse D-Click tracker"));
  return 0;
}

LRESULT CFilterKeySettingDlg::OnActivateExisting(WPARAM wParam, LPARAM lParam)
{
  UNREFERENCED_PARAMETER(wParam);
  UNREFERENCED_PARAMETER(lParam);

  if (tray_icon_added_)
  {
    RestoreFromTray();
  }
  else
  {
    BringDialogToForeground();
  }

  return 0;
}

LRESULT CFilterKeySettingDlg::OnDebugDialogClosed(WPARAM wParam, LPARAM lParam)
{
  UNREFERENCED_PARAMETER(wParam);
  UNREFERENCED_PARAMETER(lParam);

  debug_dialog_ = nullptr;
  return 0;
}

LRESULT CFilterKeySettingDlg::OnDebugOptionsChanged(WPARAM wParam, LPARAM lParam)
{
  UNREFERENCED_PARAMETER(wParam);
  UNREFERENCED_PARAMETER(lParam);

  UpdateOption(FALSE);
  return 0;
}

void CFilterKeySettingDlg::OpenDeveloperDebugDialog()
{
  if (debug_dialog_ && ::IsWindow(debug_dialog_->GetSafeHwnd()))
  {
    debug_dialog_->ShowWindow(SW_SHOW);
    debug_dialog_->SetForegroundWindow();
    return;
  }

  auto dialog = new DialogDebug(this);
  if (!dialog->Create(IDD_DEBUG_CONSOLE, this))
  {
    delete dialog;
    return;
  }

  debug_dialog_ = dialog;
  debug_dialog_->ShowWindow(SW_SHOW);
  debug_dialog_->SetForegroundWindow();
}

bool CFilterKeySettingDlg::AddTrayIcon()
{
  if (tray_icon_added_)
    return true;

  ZeroMemory(&tray_icon_data_, sizeof(tray_icon_data_));
  tray_icon_data_.cbSize           = sizeof(tray_icon_data_);
  tray_icon_data_.hWnd             = GetSafeHwnd();
  tray_icon_data_.uID              = TRAY_ICON_ID;
  tray_icon_data_.uFlags           = NIF_MESSAGE | NIF_ICON | NIF_TIP;
  tray_icon_data_.uCallbackMessage = WM_TRAYICON_MSG;
  tray_icon_data_.hIcon            = m_hIcon;
  _tcscpy_s(tray_icon_data_.szTip, static_cast<LPCTSTR>(GetAppName()));

  tray_icon_added_ = (Shell_NotifyIcon(NIM_ADD, &tray_icon_data_) == TRUE);
  return tray_icon_added_;
}

void CFilterKeySettingDlg::RemoveTrayIcon()
{
  if (!tray_icon_added_)
    return;

  Shell_NotifyIcon(NIM_DELETE, &tray_icon_data_);
  tray_icon_added_ = false;
}

void CFilterKeySettingDlg::RestoreFromTray()
{
  RemoveTrayIcon();
  BringDialogToForeground();
}

void CFilterKeySettingDlg::BringDialogToForeground()
{
  const HWND hwnd = GetSafeHwnd();
  if (!::IsWindow(hwnd))
    return;

  if (::IsIconic(hwnd))
    ShowWindow(SW_RESTORE);
  else
    ShowWindow(SW_SHOW);

  const HWND  fg_hwnd   = ::GetForegroundWindow();
  const DWORD fg_thread = fg_hwnd ? ::GetWindowThreadProcessId(fg_hwnd, nullptr) : 0;
  const DWORD ui_thread = ::GetCurrentThreadId();

  bool attached_input = false;
  if (fg_thread != 0 && fg_thread != ui_thread)
    attached_input = (::AttachThreadInput(ui_thread, fg_thread, TRUE) == TRUE);

  ::BringWindowToTop(hwnd);
  ::SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
  ::SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
  ::SetForegroundWindow(hwnd);
  ::SetActiveWindow(hwnd);

  if (attached_input)
    ::AttachThreadInput(ui_thread, fg_thread, FALSE);

  if (::GetForegroundWindow() != hwnd)
    ::FlashWindow(hwnd, TRUE);
}

void CFilterKeySettingDlg::RegisterPresetHotkeys()
{
  const bool fullscreen_mode = (GLOBAL_OPTION.getInteger(KEY_IF_FULL_SCREEN_GAME, 0) != 0);
  const bool any_enabled     = KeyBinding::IsEnabled() || KeyBinding::IsToggleEnabled();

  if (!any_enabled)
  {
    if (using_raw_input_mode_)
      DevLog::Write(_T("[Hotkey] mode changed: disabled"));
    using_raw_input_mode_ = false;

    UnregisterRawInputHotkeys();
    KeyBinding::UnregisterConfiguredHotkeys(GetSafeHwnd(), preset_count_, hotkey_registered_,
                                            &toggle_hotkey_registered_,
                                            KeyBinding::HOTKEY_ID_BASE,
                                            KeyBinding::TOGGLE_HOTKEY_ID);
    return;
  }

  if (fullscreen_mode)
  {
    if (!using_raw_input_mode_)
      DevLog::Write(_T("[Hotkey] mode changed: RawInput"));
    using_raw_input_mode_ = true;

    KeyBinding::UnregisterConfiguredHotkeys(GetSafeHwnd(), preset_count_, hotkey_registered_,
                                            &toggle_hotkey_registered_,
                                            KeyBinding::HOTKEY_ID_BASE,
                                            KeyBinding::TOGGLE_HOTKEY_ID);
    RegisterRawInputHotkeys();
    return;
  }

  if (using_raw_input_mode_)
    DevLog::Write(_T("[Hotkey] mode changed: RegisterHotKey"));
  using_raw_input_mode_ = false;

  UnregisterRawInputHotkeys();
  KeyBinding::RegisterConfiguredHotkeys(GetSafeHwnd(), preset_count_, hotkey_registered_,
                                        &toggle_hotkey_registered_,
                                        KeyBinding::HOTKEY_ID_BASE,
                                        KeyBinding::TOGGLE_HOTKEY_ID);
}

void CFilterKeySettingDlg::UnregisterPresetHotkeys()
{
  UnregisterRawInputHotkeys();
  KeyBinding::UnregisterConfiguredHotkeys(GetSafeHwnd(), preset_count_, hotkey_registered_,
                                          &toggle_hotkey_registered_,
                                          KeyBinding::HOTKEY_ID_BASE,
                                          KeyBinding::TOGGLE_HOTKEY_ID);
}

bool CFilterKeySettingDlg::RegisterRawInputHotkeys()
{
  if (raw_input_registered_)
    return true;

  RAWINPUTDEVICE device = {};
  device.usUsagePage    = 0x01;  // Generic desktop controls
  device.usUsage        = 0x06;  // Keyboard
  device.dwFlags        = RIDEV_INPUTSINK;
  device.hwndTarget     = GetSafeHwnd();

  if (!::RegisterRawInputDevices(&device, 1, sizeof(device)))
    return false;

  raw_input_registered_     = true;
  raw_input_modifiers_down_ = 0;
  raw_input_key_down_.fill(false);
  return true;
}

void CFilterKeySettingDlg::UnregisterRawInputHotkeys()
{
  if (!raw_input_registered_)
    return;

  RAWINPUTDEVICE device = {};
  device.usUsagePage    = 0x01;
  device.usUsage        = 0x06;
  device.dwFlags        = RIDEV_REMOVE;
  device.hwndTarget     = nullptr;
  ::RegisterRawInputDevices(&device, 1, sizeof(device));

  raw_input_registered_     = false;
  raw_input_modifiers_down_ = 0;
  raw_input_key_down_.fill(false);
}

void CFilterKeySettingDlg::UpdateRawInputModifierState(UINT vk, bool key_down)
{
  const UINT mask = ModifierMaskForVirtualKey(vk);
  if (mask == 0)
    return;

  if (key_down)
    raw_input_modifiers_down_ |= mask;
  else
    raw_input_modifiers_down_ &= ~mask;
}

int CFilterKeySettingDlg::ResolveRawInputHotkeyId(UINT vk, UINT modifiers) const
{
  if (KeyBinding::IsToggleEnabled())
  {
    const DWORD toggle_hotkey = KeyBinding::GetToggleHotkey();
    if (vk == KeyBinding::HotkeyVK(toggle_hotkey) &&
        modifiers == KeyBinding::HotkeyModifiers(toggle_hotkey))
      return KeyBinding::TOGGLE_HOTKEY_ID;
  }

  if (!KeyBinding::IsEnabled())
    return -1;

  for (int preset = PRESET_OFF; preset < preset_count_; ++preset)
  {
    PresetOption option(preset);
    const DWORD  hotkey = option.getInteger(KEY_PRESET_HOTKEY, 0);
    if (vk == KeyBinding::HotkeyVK(hotkey) &&
        modifiers == KeyBinding::HotkeyModifiers(hotkey))
      return KeyBinding::HOTKEY_ID_BASE + preset;
  }

  return -1;
}

bool CFilterKeySettingDlg::HandleResolvedHotkeyId(UINT hotkey_id, bool from_raw_input)
{
  const LPCTSTR source = from_raw_input ? _T("RawInput") : _T("RegisterHotKey");

  if (hotkey_id == static_cast<UINT>(KeyBinding::TOGGLE_HOTKEY_ID))
  {
    const int current_preset = static_cast<int>(GLOBAL_OPTION.getInteger(KEY_LAST_PRESET));
    const int target_preset  = KeyBinding::ResolveToggleTargetPreset(current_preset, last_on_preset_, preset_count_);

    if (!PRESET_IS_VALID(target_preset))
      return true;

    DevLog::Writef(_T("[Hotkey] source=%s id=Toggle -> preset=%d"), source, target_preset);
    ResetEditMode();
    ActivatePreset(target_preset, FALSE, TRUE, from_raw_input ? _T("Toggle hotkey (RawInput)") : _T("Toggle hotkey"));
    return true;
  }

  if (!KeyBinding::IsEnabled())
    return false;

  if (hotkey_id < KeyBinding::HOTKEY_ID_BASE ||
      hotkey_id >= KeyBinding::HOTKEY_ID_BASE + static_cast<UINT>(preset_count_))
    return false;

  const int target_preset = static_cast<int>(hotkey_id - KeyBinding::HOTKEY_ID_BASE);
  if (!PRESET_IS_VALID(target_preset))
    return true;

  DevLog::Writef(_T("[Hotkey] source=%s id=Preset(%u) -> preset=%d"), source, hotkey_id, target_preset);
  ResetEditMode();
  ActivatePreset(target_preset, FALSE, TRUE, from_raw_input ? _T("Preset hotkey (RawInput)") : _T("Preset hotkey"));
  return true;
}

bool CFilterKeySettingDlg::EnsureAdminGuardForOptionEnable(const CString& option_key,
                                                           bool           request_enable,
                                                           bool           previous_enabled)
{
  if (!request_enable || previous_enabled)
    return true;

  const auto result = AdminGuard::PromptAdminRestartIfNeeded(this, &option_key);
  if (result == AdminGuard::PromptResult::Proceed)
    return true;

  return false;
}

bool CFilterKeySettingDlg::EnsureAdminGuardOnStartup()
{
  const auto result = AdminGuard::PromptAdminRestartIfNeeded(this);
  return (result != AdminGuard::PromptResult::RestartRequested);
}

void CFilterKeySettingDlg::RefreshPresetButtonCaption(const int preset)
{
  if (PRESET_IS_VALID(preset) == false)
    return;

  PresetOption option(preset);
  CString      title   = option.getString(KEY_PRESET_TITLE);
  CString      caption = title;

  if (KeyBinding::IsEnabled() && alt_hotkey_view_)
  {
    caption = KeyBinding::HotkeyToString(option.getInteger(KEY_PRESET_HOTKEY, 0), true);
  }

  if (auto button = GetDlgItem(GetPresetButtonControlId(preset)); button)
    button->SetWindowText(caption);
}

void CFilterKeySettingDlg::RefreshAllPresetButtonCaptions()
{
  for (int preset = PRESET_OFF; preset < preset_count_; ++preset)
    RefreshPresetButtonCaption(preset);
}

BOOL CFilterKeySettingDlg::PreTranslateMessage(MSG* pMsg)
{
  tooltip_.RelayEvent(pMsg);

  const auto IsCtrlAltF12 = [](const MSG* msg) -> bool {
    if (!msg)
      return false;

    if ((msg->message != WM_KEYDOWN) && (msg->message != WM_SYSKEYDOWN))
      return false;

    if (msg->wParam != VK_F12)
      return false;

    const bool ctrl_pressed    = ((GetKeyState(VK_CONTROL) & 0x8000) != 0) || ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0);
    const bool alt_from_syskey = (msg->message == WM_SYSKEYDOWN) && ((msg->lParam & (1UL << 29)) != 0);
    const bool alt_pressed     = alt_from_syskey || ((GetKeyState(VK_MENU) & 0x8000) != 0) || ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0);

    return ctrl_pressed && alt_pressed;
  };

  if (IsCtrlAltF12(pMsg))
  {
    OpenDeveloperDebugDialog();
    return TRUE;
  }

  if (pMsg->message == WM_SYSCHAR)
  {
    return TRUE;
  }

  if (pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN)
  {
    if (pMsg->wParam == VK_MENU || pMsg->wParam == VK_LMENU || pMsg->wParam == VK_RMENU)
    {
      if (alt_hotkey_view_ == false)
      {
        alt_hotkey_view_ = true;
        RefreshAllPresetButtonCaptions();
      }

      return TRUE;
    }

    // ESC, Enter 키 이벤트 제거
    if (pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE)
    {
      return TRUE;
    }
  }
  else if (pMsg->message == WM_KEYUP || pMsg->message == WM_SYSKEYUP)
  {
    if (pMsg->wParam == VK_MENU || pMsg->wParam == VK_LMENU || pMsg->wParam == VK_RMENU)
    {
      if (alt_hotkey_view_)
      {
        alt_hotkey_view_ = false;
        RefreshAllPresetButtonCaptions();
      }
    }
  }

  return CDialogEx::PreTranslateMessage(pMsg);
}

void CFilterKeySettingDlg::OnBnClickedCheckEditMode()
{
  if (PRESET_IS_ON(last_selected_))
  {
    // UnChecked -> Checked된 상태인가?
    const bool edit_on = IsEditModeChecked();
    TRACE(_T("EditMode : %s : Target Preset(%d)\n"), edit_on ? "Checked" : "UnChecked", last_selected_);

    if (edit_on)
    {
      const int edit_target_preset = last_selected_;

      // 필터키 일시적으로 끄기
      preset_before_edit_ = GLOBAL_OPTION.getInteger(KEY_LAST_PRESET);
      ActivatePreset(PRESET_OFF, FALSE, FALSE, _T("Edit mode enter"));

      // 편집 대상은 유지한다. (적용 시 OFF로 저장/적용되는 문제 방지)
      last_selected_ = edit_target_preset;
      UpdateInterface(last_selected_);
    }
    else
    {
      const int target_preset_for_apply = PRESET_IS_ON(preset_before_edit_) ? preset_before_edit_ : last_selected_;
      bool      values_changed          = false;
      if (!SaveCurrentEditingValues(target_preset_for_apply, &values_changed))
      {
        if (auto edit_mode_button = static_cast<CButton*>(GetDlgItem(IDC_CHECK_EDIT_MODE)); edit_mode_button)
          edit_mode_button->SetCheck(BST_CHECKED);
        UpdateEditModeCaption();
        return;
      }

      ActivatePreset(target_preset_for_apply, values_changed ? TRUE : FALSE, FALSE, _T("Edit mode apply"));
      preset_before_edit_ = PRESET_OFF;
    }

    UpdateOption();
    UpdateEditModeCaption();
  }
}

void CFilterKeySettingDlg::OnBnClickedCheckRestoreSetting()
{
  UpdateOption();  //
}

void CFilterKeySettingDlg::OnBnClickedCheckDisableHotkey()
{
  UpdateOption();  //
}

void CFilterKeySettingDlg::OnBnClickedCheckMoveToTray()
{
  UpdateOption();
}

void CFilterKeySettingDlg::OnBnClickedCheckEnableKeybind()
{
  if (auto btn = static_cast<CButton*>(GetDlgItem(IDC_CHECK_ENABLE_KEYBIND)); btn)
  {
    const bool checked  = (btn->GetCheck() == BST_CHECKED);
    const bool previous = KeyBinding::IsEnabled();
    if (!EnsureAdminGuardForOptionEnable(KEY_ENABLE_KEYBIND, checked, previous))
    {
      btn->SetCheck(BST_UNCHECKED);
      return;
    }

    if (checked && !previous)
    {
      GLOBAL_OPTION.set(KEY_ENABLE_KEYBIND, true);
      if (!ValidateActiveHotkeysAndAlert())
      {
        GLOBAL_OPTION.set(KEY_ENABLE_KEYBIND, false);
        btn->SetCheck(BST_UNCHECKED);
      }
    }
  }

  UpdateOption();
}

void CFilterKeySettingDlg::OnBnClickedCheckEnableToggleKeybind()
{
  if (auto btn = static_cast<CButton*>(GetDlgItem(IDC_CHECK_ENABLE_TOGGLE_KEYBIND)); btn)
  {
    const bool checked  = (btn->GetCheck() == BST_CHECKED);
    const bool previous = KeyBinding::IsToggleEnabled();
    if (!EnsureAdminGuardForOptionEnable(KEY_ENABLE_TOGGLE_KEYBIND, checked, previous))
    {
      btn->SetCheck(BST_UNCHECKED);
      return;
    }

    if (checked && !previous)
    {
      GLOBAL_OPTION.set(KEY_ENABLE_TOGGLE_KEYBIND, true);
      if (!ValidateActiveHotkeysAndAlert())
      {
        GLOBAL_OPTION.set(KEY_ENABLE_TOGGLE_KEYBIND, false);
        btn->SetCheck(BST_UNCHECKED);
      }
    }
  }

  UpdateOption();
}

void CFilterKeySettingDlg::OnBnClickedCheckSetMouseDblclickTracker()
{
  if (auto btn = static_cast<CButton*>(GetDlgItem(IDC_CHECK_SET_MOUSE_DBLCLICK_TRACKER)); btn)
  {
    const bool checked  = (btn->GetCheck() == BST_CHECKED);
    const bool previous = (GLOBAL_OPTION.getInteger(KEY_ENABLE_MOUSE_DBLCLICK_TRACKER, 0) != 0);
    if (!EnsureAdminGuardForOptionEnable(KEY_ENABLE_MOUSE_DBLCLICK_TRACKER, checked, previous))
    {
      btn->SetCheck(BST_UNCHECKED);
      return;
    }
  }

  UpdateOption();
}

void CFilterKeySettingDlg::OnBnClickedCheckDisableWithEsc()
{
  if (auto btn = static_cast<CButton*>(GetDlgItem(IDC_CHECK_DISABLE_WITH_ESC)); btn)
  {
    const bool checked  = (btn->GetCheck() == BST_CHECKED);
    const bool previous = (GLOBAL_OPTION.getInteger(KEY_DISABLE_WITH_ESC, 0) != 0);

    if (!EnsureAdminGuardForOptionEnable(KEY_DISABLE_WITH_ESC, checked, previous))
    {
      btn->SetCheck(BST_UNCHECKED);
      return;
    }

    if (checked && !previous)
    {
      if (KeyBinding::IsActiveHotkeyAssigned(VK_ESCAPE, 0, preset_count_))
      {
        AfxMessageBox(_T("안내: 현재 활성 단축키에 ESC가 포함되어 있습니다.\r\n"
                         "이 경우 '백그라운드에서 ESC로 프리셋 끄기' 동작은 실행되지 않습니다."));
      }
    }
  }

  UpdateOption();
}

void CFilterKeySettingDlg::OnEnSetFocusTesting()
{
  if (auto* ctrl = GetDlgItem(IDC_EDIT_TESTING); ctrl)
    ctrl->SetWindowText(_T(""));
}

void CFilterKeySettingDlg::OnEnKillFocusTesting()
{
  if (auto* ctrl = GetDlgItem(IDC_EDIT_TESTING); ctrl)
    ctrl->SetWindowText(_T("설정 값 테스트 해보기"));
}

void CFilterKeySettingDlg::OnEnSetFocusToggleKeybind()
{
  if (opening_toggle_hotkey_dialog_)
    return;

  if (!KeyBinding::IsToggleEnabled())
    return;

  PopupToggleKeyBindingDialog();

  if (auto fallback = GetDlgItem(IDC_EDIT_TESTING); fallback)
    fallback->SetFocus();
}

////////////////////////////////////////////////////////////////////
void CFilterKeySettingDlg::BnClickPreset(const int target_preset)
{
  if (PRESET_IS_VALID(target_preset) == false)
    return;

  const bool press_ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
  const bool press_alt  = (GetKeyState(VK_MENU) & 0x8000) != 0;
  const bool keybind_on = KeyBinding::IsEnabled();

  if (press_alt && keybind_on)
  {
    ResetEditMode();
    last_selected_ = target_preset;
    PopupKeyBindingDialog(target_preset);
  }
  else if (press_ctrl)
  {
    ResetEditMode();
    last_selected_ = target_preset;
    if (PRESET_IS_ON(target_preset))
      PopupRenameDialog(target_preset);
  }
  else
  {
    if (!ConfirmSaveIfEditingAndDirty(target_preset))
      return;

    ResetEditMode();
    last_selected_ = target_preset;
    ActivatePreset(target_preset, FALSE, TRUE, _T("Preset button"));
  }
}

void CFilterKeySettingDlg::PopupRenameDialog(const int target_preset)
{
  DialogRename dlg(PresetOption::TAG(target_preset), this);

  if (dlg.DoModal() == IDOK)
  {
    // Update registry & caption

    const UINT uid = GetPresetButtonControlId(target_preset);

    PresetOption option(target_preset);
    option.set(KEY_PRESET_TITLE, dlg.new_name_);
    if (auto preset_button = GetDlgItem(uid); preset_button)
      preset_button->SetWindowText(dlg.new_name_);
  }
  RefreshPresetButtonCaption(target_preset);
}

void CFilterKeySettingDlg::PopupKeyBindingDialog(const int target_preset)
{
  if (KeyBinding::IsEnabled() == false)
    return;

  PresetOption option(target_preset);
  DWORD        current_hotkey = option.getInteger(KEY_PRESET_HOTKEY, 0);

  UnregisterPresetHotkeys();

  const auto FinishBindingFlow = [&]() {
    alt_hotkey_view_ = false;
    RegisterPresetHotkeys();
    RefreshAllPresetButtonCaptions();
  };

  while (true)
  {
    DialogKeyBinding dlg(current_hotkey, this);
    if (dlg.DoModal() != IDOK)
    {
      FinishBindingFlow();
      return;
    }

    const DWORD hotkey_value = dlg.hotkey_value_;
    auto        validation   = KeyBinding::ValidateHotkeyCandidate(
        preset_count_, hotkey_value, target_preset, false);

    if (!validation.ok)
    {
      AfxMessageBox(validation.error_message);
      current_hotkey = hotkey_value;
      continue;
    }

    option.set(KEY_PRESET_HOTKEY, hotkey_value);
    FinishBindingFlow();
    return;
  }
}

void CFilterKeySettingDlg::PopupToggleKeyBindingDialog()
{
  if (!KeyBinding::IsToggleEnabled())
    return;

  const DWORD current_hotkey = KeyBinding::GetToggleHotkey();

  UnregisterPresetHotkeys();
  opening_toggle_hotkey_dialog_ = true;

  const auto FinishBindingFlow = [&]() {
    opening_toggle_hotkey_dialog_ = false;
    RegisterPresetHotkeys();
    RefreshToggleHotkeyEditText();
    RefreshAllPresetButtonCaptions();
  };

  DWORD next_hotkey = current_hotkey;
  while (true)
  {
    DialogKeyBinding dlg(next_hotkey, this);
    if (dlg.DoModal() != IDOK)
    {
      FinishBindingFlow();
      return;
    }

    const DWORD hotkey_value = dlg.hotkey_value_;
    auto        validation   = KeyBinding::ValidateHotkeyCandidate(
        preset_count_, hotkey_value, -1, true);

    if (!validation.ok)
    {
      AfxMessageBox(validation.error_message);
      next_hotkey = hotkey_value;
      continue;
    }

    GLOBAL_OPTION.set(KEY_TOGGLE_HOTKEY, hotkey_value);
    FinishBindingFlow();
    return;
  }
}

void CFilterKeySettingDlg::ActivatePreset(const int preset, BOOL alert, BOOL beep, LPCTSTR reason)
{
  const int target_preset = PRESET_IS_VALID(preset)
                                ? preset
                                : PRESET_OFF;

  if (process_watch_auto_off_ && !process_watch_switching_)
    process_watch_auto_off_ = false;

  const int current_preset = static_cast<int>(GLOBAL_OPTION.getInteger(KEY_LAST_PRESET));
  if (target_preset == current_preset)
  {
    last_selected_ = target_preset;
    UpdateInterface(target_preset);
    return;
  }

  DevLog::Writef(_T("[Preset] %d -> %d | %s"), current_preset, target_preset, reason ? reason : _T("(unknown)"));

  mouse_tracker_.RequestReset();

  int  last_on = last_on_preset_;
  bool ok      = PresetService::ActivatePreset(target_preset, &last_on);
  if (target_preset < preset_count_)
    last_on_preset_ = last_on;

  if (ok)
    UserPresetOSD::ShowPresetIndex(target_preset);

  if (ok && beep)
    UserPlaySound::PlayPresetAppliedSound(target_preset);

  if (alert)
  {
    if (ok)
      AfxMessageBox(_T("프리셋이 저장되었습니다."));
    else
      AfxMessageBox(_T("프리셋을 수정할 수 없습니다\r\n관리자 권한으로 실행하세요."));
  }

  last_selected_ = target_preset;
  UpdateInterface(target_preset);
}

void CFilterKeySettingDlg::ResetEditMode()
{
  if (auto* btn = static_cast<CButton*>(GetDlgItem(IDC_CHECK_EDIT_MODE)); btn)
    btn->SetCheck(BST_UNCHECKED);
  preset_before_edit_ = PRESET_OFF;
  UpdateEditModeCaption();
}

bool CFilterKeySettingDlg::ConfirmSaveIfEditingAndDirty(const int next_preset)
{
  if (!IsEditModeChecked())
    return true;

  const int editing_preset = PRESET_IS_ON(last_selected_) ? last_selected_ : PRESET_OFF;
  if (!PRESET_IS_ON(editing_preset))
    return true;

  PresetValues edited = {};
  if (!ReadPresetValuesFromEdits(this, &edited))
    return false;

  const auto validation = PresetService::ValidateValues(edited);
  if (!validation.ok)
  {
    AfxMessageBox(validation.error_message);
    return false;
  }

  const auto current_values = PresetService::GetPresetValues(editing_preset);
  const bool changed        = (edited.accept_delay != current_values.accept_delay) ||
                              (edited.repeat_delay != current_values.repeat_delay) ||
                              (edited.repeat_rate != current_values.repeat_rate);
  if (!changed)
    return true;

  const int answer = AfxMessageBox(
      _T("수정된 값이 저장되지 않았습니다. 저장하시겠습니까?"),
      MB_ICONQUESTION | MB_YESNO);
  if (answer != IDYES)
    return true;

  bool saved_changed = false;
  if (!PresetService::SavePresetValues(editing_preset, edited, &saved_changed))
  {
    AfxMessageBox(_T("프리셋을 수정할 수 없습니다\r\n관리자 권한으로 실행하세요."));
    return false;
  }

  return true;
}

void CFilterKeySettingDlg::UpdateEditModeCaption()
{
  if (auto* btn = static_cast<CButton*>(GetDlgItem(IDC_CHECK_EDIT_MODE)); btn)
  {
    const bool checked = IsEditModeChecked();
    btn->SetWindowText(checked ? _T("저장하기") : _T("수정하기"));
    btn->Invalidate();
    btn->UpdateWindow();

    EnsureEditModeHintLabel();
    if (edit_mode_hint_label_ready_ && ::IsWindow(edit_mode_hint_label_.GetSafeHwnd()))
    {
      edit_mode_hint_label_.ShowWindow(checked ? SW_SHOW : SW_HIDE);
      edit_mode_hint_label_.Invalidate();
      edit_mode_hint_label_.UpdateWindow();
    }
  }
}

void CFilterKeySettingDlg::EnsureEditModeHintLabel()
{
  if (edit_mode_hint_label_ready_ && ::IsWindow(edit_mode_hint_label_.GetSafeHwnd()))
    return;

  auto* edit_mode_button = GetDlgItem(IDC_CHECK_EDIT_MODE);
  if (!edit_mode_button)
    return;

  CRect button_rect;
  edit_mode_button->GetWindowRect(&button_rect);
  ScreenToClient(&button_rect);

  CRect client_rect;
  GetClientRect(&client_rect);

  int left   = button_rect.right + 10;
  int top    = button_rect.top + 4;
  int width  = 280;
  int height = button_rect.Height();

  if (left + width > client_rect.right - 10)
  {
    left  = button_rect.left;
    top   = button_rect.bottom + 2;
    width = max(180, client_rect.right - left - 10);
  }

  const CRect label_rect(left, top, left + width, top + height);
  if (!edit_mode_hint_label_.Create(
          _T("저장하려면 이 체크를 다시 눌러 해제하세요."),
          WS_CHILD | SS_LEFT,
          label_rect,
          this))
    return;

  if (auto* font = edit_mode_button->GetFont(); font)
    edit_mode_hint_label_.SetFont(font);

  edit_mode_hint_label_.ShowWindow(SW_HIDE);
  edit_mode_hint_label_ready_ = true;
}

void CFilterKeySettingDlg::UpdateOption(BOOL write /* = TRUE*/)
{
  if (write)
    SyncOptionsFromUI();
  else
    SyncOptionsToUI();

  ApplySubsystemOptions();
  RefreshAllPresetButtonCaptions();
  RefreshToggleHotkeyEditText();
}

void CFilterKeySettingDlg::SyncOptionsFromUI()
{
  CButton* btn = nullptr;

  {
    btn          = static_cast<CButton*>(GetDlgItem(IDC_CHECK_RESTORE_SETTING));
    auto checked = btn ? btn->GetCheck() : false;
    GLOBAL_OPTION.set(KEY_RESTORE_SETTING, static_cast<DWORD>(checked));
  }

  {
    btn          = static_cast<CButton*>(GetDlgItem(IDC_CHECK_DISABLE_HOTKEY));
    auto checked = btn ? btn->GetCheck() : false;
    GLOBAL_OPTION.set(KEY_DISABLE_HOTKEY, static_cast<DWORD>(checked));

    PresetOption option(PRESET_OFF);
    DWORD        value = checked ? WINDOW_FILTER_FLAG & ~(FKF_HOTKEYACTIVE | FKF_CONFIRMHOTKEY | FKF_HOTKEYSOUND)
                                 : WINDOW_FILTER_FLAG;
    option.set(KEY_FILTER_FLAG, value);
  }

  {
    btn          = static_cast<CButton*>(GetDlgItem(IDC_CHECK_MOVE_TO_TRAY));
    auto checked = btn ? btn->GetCheck() : false;
    GLOBAL_OPTION.set(KEY_MOVE_TO_TRAY, static_cast<DWORD>(checked));
  }

  {
    btn          = static_cast<CButton*>(GetDlgItem(IDC_CHECK_ENABLE_KEYBIND));
    auto checked = btn ? btn->GetCheck() : false;
    GLOBAL_OPTION.set(KEY_ENABLE_KEYBIND, static_cast<DWORD>(checked));

    if (!checked)
      alt_hotkey_view_ = false;
  }

  {
    btn          = static_cast<CButton*>(GetDlgItem(IDC_CHECK_ENABLE_TOGGLE_KEYBIND));
    auto checked = btn ? btn->GetCheck() : false;
    GLOBAL_OPTION.set(KEY_ENABLE_TOGGLE_KEYBIND, static_cast<DWORD>(checked));

    if (auto toggle_edit = GetDlgItem(IDC_EDIT_TOGGLE_KEYBIND); toggle_edit)
      toggle_edit->EnableWindow(checked ? TRUE : FALSE);
  }

  {
    btn          = static_cast<CButton*>(GetDlgItem(IDC_CHECK_SET_MOUSE_DBLCLICK_TRACKER));
    auto checked = btn ? btn->GetCheck() : false;
    GLOBAL_OPTION.set(KEY_ENABLE_MOUSE_DBLCLICK_TRACKER, static_cast<DWORD>(checked));
  }

  {
    btn          = static_cast<CButton*>(GetDlgItem(IDC_CHECK_DISABLE_WITH_ESC));
    auto checked = btn ? btn->GetCheck() : false;
    GLOBAL_OPTION.set(KEY_DISABLE_WITH_ESC, static_cast<DWORD>(checked));
  }
}

void CFilterKeySettingDlg::SyncOptionsToUI()
{
  const auto SetCheck = [&](int ctrl_id, const CString& key) {
    if (auto* btn = static_cast<CButton*>(GetDlgItem(ctrl_id)); btn)
      btn->SetCheck(GLOBAL_OPTION.getInteger(key) ? BST_CHECKED : BST_UNCHECKED);
  };

  SetCheck(IDC_CHECK_RESTORE_SETTING, KEY_RESTORE_SETTING);
  SetCheck(IDC_CHECK_DISABLE_HOTKEY, KEY_DISABLE_HOTKEY);
  SetCheck(IDC_CHECK_MOVE_TO_TRAY, KEY_MOVE_TO_TRAY);
  SetCheck(IDC_CHECK_ENABLE_KEYBIND, KEY_ENABLE_KEYBIND);
  SetCheck(IDC_CHECK_ENABLE_TOGGLE_KEYBIND, KEY_ENABLE_TOGGLE_KEYBIND);
  SetCheck(IDC_CHECK_SET_MOUSE_DBLCLICK_TRACKER, KEY_ENABLE_MOUSE_DBLCLICK_TRACKER);
  SetCheck(IDC_CHECK_DISABLE_WITH_ESC, KEY_DISABLE_WITH_ESC);

  if (auto toggle_edit = GetDlgItem(IDC_EDIT_TOGGLE_KEYBIND); toggle_edit)
    toggle_edit->EnableWindow(KeyBinding::IsToggleEnabled() ? TRUE : FALSE);

  if (!KeyBinding::IsEnabled())
    alt_hotkey_view_ = false;
}

void CFilterKeySettingDlg::ApplySubsystemOptions()
{
  RegisterPresetHotkeys();

  const bool move_enabled = (GLOBAL_OPTION.getInteger(KEY_ENABLE_MOUSE_MOVE_TRACKER) != 0);
  const bool dbl_enabled  = (GLOBAL_OPTION.getInteger(KEY_ENABLE_MOUSE_DBLCLICK_TRACKER) != 0);
  if (move_enabled || dbl_enabled)
  {
    if (mouse_tracker_.Initialize(GetSafeHwnd()))
    {
      static constexpr DWORD kSensitivityToPx[] = { 1000, 800, 600, 400, 200 };
      const auto             sens               = static_cast<int>(GLOBAL_OPTION.getInteger(KEY_MOUSE_MOVE_SENSITIVITY, 2));
      const DWORD            dist               = kSensitivityToPx[(sens >= 0 && sens <= 4) ? sens : 2];
      mouse_tracker_.SetDistanceTarget(dist);
      mouse_tracker_.SetTriggerOptions(move_enabled, dbl_enabled);
      mouse_tracker_.Activate();
    }
  }
  else
  {
    mouse_tracker_.Release();
  }

  UpdateEscDisableHotkeyRegistration();
  UpdateProcessWatcherRegistration();
}

void CFilterKeySettingDlg::UpdateEscDisableHotkeyRegistration()
{
  const bool enabled = (GLOBAL_OPTION.getInteger(KEY_DISABLE_WITH_ESC) != 0);
  if (enabled)
  {
    if (!bg_esc_watch_active_)
    {
      if (SetTimer(TIMER_BG_ESC_WATCH, 25, nullptr) != 0)
      {
        bg_esc_watch_active_ = true;
        bg_esc_prev_down_    = false;
      }
      else
      {
        TRACE(_T("SetTimer failed. esc disable watcher, error=%lu\n"), GetLastError());
        GLOBAL_OPTION.set(KEY_DISABLE_WITH_ESC, static_cast<DWORD>(false));
        if (auto esc_button = static_cast<CButton*>(GetDlgItem(IDC_CHECK_DISABLE_WITH_ESC)); esc_button)
          esc_button->SetCheck(BST_UNCHECKED);
      }
    }
  }
  else
  {
    if (bg_esc_watch_active_)
    {
      KillTimer(TIMER_BG_ESC_WATCH);
      bg_esc_watch_active_ = false;
      bg_esc_prev_down_    = false;
    }
  }
}

void CFilterKeySettingDlg::UpdateProcessWatcherRegistration()
{
  const bool enabled = (GLOBAL_OPTION.getInteger(KEY_PROCESS_OFF_ENABLED) != 0);
  CString    name    = GLOBAL_OPTION.getString(KEY_PROCESS_OFF_NAME);
  name.Trim();

  const bool should_run = enabled && !name.IsEmpty();

  if (should_run)
  {
    process_watch_target_ = name;

    if (!process_watch_active_)
    {
      process_watch_was_focused_ = false;
      process_watch_auto_off_    = false;
      process_watch_last_fg_     = nullptr;

      if (SetTimer(TIMER_PROCESS_WATCHER, 250, nullptr) != 0)
        process_watch_active_ = true;
    }
  }
  else
  {
    if (process_watch_active_)
    {
      KillTimer(TIMER_PROCESS_WATCHER);
      process_watch_active_      = false;
      process_watch_was_focused_ = false;
      process_watch_auto_off_    = false;
      process_watch_last_fg_     = nullptr;
    }
  }
}

CString CFilterKeySettingDlg::GetForegroundProcessName()
{
  HWND fg = ::GetForegroundWindow();
  if (!fg)
    return CString();

  DWORD pid = 0;
  ::GetWindowThreadProcessId(fg, &pid);
  if (pid == 0)
    return CString();

  HANDLE proc = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
  if (!proc)
    return CString();

  TCHAR path[MAX_PATH] = {};
  DWORD size           = MAX_PATH;
  BOOL  ok             = ::QueryFullProcessImageName(proc, 0, path, &size);
  ::CloseHandle(proc);

  if (!ok)
    return CString();

  CString full(path);
  int     pos = full.ReverseFind(_T('\\'));
  return (pos >= 0) ? full.Mid(pos + 1) : full;
}

void CFilterKeySettingDlg::TickProcessWatcher()
{
  HWND fg = ::GetForegroundWindow();
  if (fg == process_watch_last_fg_)
    return;
  process_watch_last_fg_ = fg;

  CString fg_name     = GetForegroundProcessName();
  bool    now_focused = (!fg_name.IsEmpty() && fg_name.CompareNoCase(process_watch_target_) == 0);

  if (process_watch_was_focused_ && !now_focused)
  {
    const int current = static_cast<int>(GLOBAL_OPTION.getInteger(KEY_LAST_PRESET));
    if (PRESET_IS_ON(current))
    {
      process_watch_saved_preset_ = current;
      process_watch_switching_    = true;
      ResetEditMode();
      ActivatePreset(PRESET_OFF, FALSE, TRUE, _T("Process watcher (off)"));
      process_watch_switching_ = false;
      process_watch_auto_off_  = true;
    }
  }
  else if (!process_watch_was_focused_ && now_focused)
  {
    const bool restore = (GLOBAL_OPTION.getInteger(KEY_PROCESS_OFF_RESTORE) != 0);
    if (restore && process_watch_auto_off_ && PRESET_IS_VALID(process_watch_saved_preset_))
    {
      const int current = static_cast<int>(GLOBAL_OPTION.getInteger(KEY_LAST_PRESET));
      if (current == PRESET_OFF)
      {
        process_watch_switching_ = true;
        ActivatePreset(process_watch_saved_preset_, FALSE, TRUE, _T("Process watcher (restore)"));
        process_watch_switching_ = false;
      }
    }
    process_watch_auto_off_ = false;
  }

  process_watch_was_focused_ = now_focused;
}

bool CFilterKeySettingDlg::ValidateActiveHotkeysAndAlert() const
{
  CString conflict_message;
  if (KeyBinding::ValidateActiveHotkeys(preset_count_, &conflict_message))
    return true;

  if (!conflict_message.IsEmpty())
    AfxMessageBox(conflict_message);
  return false;
}

void CFilterKeySettingDlg::RefreshToggleHotkeyEditText()
{
  if (auto toggle_edit = GetDlgItem(IDC_EDIT_TOGGLE_KEYBIND); toggle_edit)
  {
    const DWORD hotkey_value = KeyBinding::GetToggleHotkey();
    toggle_edit->SetWindowText(KeyBinding::HotkeyToString(hotkey_value, true));
  }
}

void CFilterKeySettingDlg::UpdateInterface(const int preset)
{
  {
    PresetOption option(GLOBAL_OPTION.getInteger(KEY_LAST_PRESET));
    CString      window_title;
    CString      preset_title = option.getString(KEY_PRESET_TITLE);
    window_title.Format(_T("활성화된 프리셋 : %s"), (LPCTSTR)preset_title);
    if (FilterKey::IsProcessElevatedNow())
      window_title += _T(" (관리자 모드)");
    SetWindowText(window_title);
  }

  if (PRESET_IS_ON(preset))
  {
    PresetValues pv = PresetService::GetPresetValues(preset);
    CString      value;

    const auto SetEditValue = [&](int ctrl_id, DWORD v) {
      if (auto* ctrl = GetDlgItem(ctrl_id); ctrl)
      {
        value.Format(_T("%d"), v);
        ctrl->SetWindowText(value);
      }
    };

    SetEditValue(IDC_EDIT_ACCEPT_DELAY, pv.accept_delay);
    SetEditValue(IDC_EDIT_REPEAT_DELAY, pv.repeat_delay);
    SetEditValue(IDC_EDIT_REPEAT_RATE, pv.repeat_rate);
  }

  auto* edit_btn = static_cast<CButton*>(GetDlgItem(IDC_CHECK_EDIT_MODE));
  if (edit_btn)
    edit_btn->EnableWindow(PRESET_IS_ON(preset) ? TRUE : FALSE);

  UpdateEditModeCaption();

  const bool edit_on = edit_btn ? (edit_btn->GetCheck() != 0) : false;
  const bool enable  = PRESET_IS_ON(preset) && edit_on;

  if (auto* ctrl = GetDlgItem(IDC_EDIT_ACCEPT_DELAY); ctrl)
    ctrl->EnableWindow(enable ? TRUE : FALSE);
  if (auto* ctrl = GetDlgItem(IDC_EDIT_REPEAT_DELAY); ctrl)
    ctrl->EnableWindow(enable ? TRUE : FALSE);
  if (auto* ctrl = GetDlgItem(IDC_EDIT_REPEAT_RATE); ctrl)
    ctrl->EnableWindow(enable ? TRUE : FALSE);
}

void CFilterKeySettingDlg::InitializePresetCount()
{
  preset_count_ = PresetService::ResolvePresetCount();
}

void CFilterKeySettingDlg::BuildPresetButtons()
{
  CWnd* template_off_button = GetDlgItem(IDC_BTN_PRESET_OFF);
  CWnd* template_p1_button  = GetDlgItem(IDC_BTN_PRESET1);

  if (auto ctrl = GetDlgItem(IDC_BTN_PRESET_OFF); ctrl)
    ctrl->ShowWindow(SW_HIDE);
  if (auto ctrl = GetDlgItem(IDC_BTN_PRESET1); ctrl)
    ctrl->ShowWindow(SW_HIDE);
  if (auto ctrl = GetDlgItem(IDC_BTN_PRESET2); ctrl)
    ctrl->ShowWindow(SW_HIDE);

  CRect base_rect;
  CRect next_rect;
  template_off_button->GetWindowRect(&base_rect);
  ScreenToClient(&base_rect);

  int stride = 32;
  if (template_p1_button)
  {
    template_p1_button->GetWindowRect(&next_rect);
    ScreenToClient(&next_rect);
    stride = next_rect.top - base_rect.top;
    if (stride <= 0)
      stride = base_rect.Height() + 6;
  }

  {
    const int button_height = base_rect.Height();
    int       button_gap    = stride - button_height;

    int min_required_gap = 2;
    if (preset_count_ > 1)
    {
      std::vector<UINT> settings_ids = {
        IDC_CHECK_EDIT_MODE,
        IDC_EDIT_ACCEPT_DELAY,
        IDC_EDIT_REPEAT_DELAY,
        IDC_EDIT_REPEAT_RATE,
        IDC_STATIC_LABEL_ACCEPT,
        IDC_STATIC_LABEL_ACCEPT_MS,
        IDC_STATIC_LABEL_REPEAT_DELAY,
        IDC_STATIC_LABEL_REPEAT_DELAY_MS,
        IDC_STATIC_LABEL_REPEAT_RATE,
        IDC_STATIC_LABEL_REPEAT_RATE_MS,
      };

      bool  cluster_initialized = false;
      CRect settings_cluster;
      for (auto id : settings_ids)
      {
        if (auto ctrl = GetDlgItem(id); ctrl)
        {
          CRect rect;
          ctrl->GetWindowRect(&rect);
          ScreenToClient(&rect);

          if (!cluster_initialized)
          {
            settings_cluster    = rect;
            cluster_initialized = true;
          }
          else
          {
            settings_cluster.UnionRect(settings_cluster, rect);
          }
        }
      }

      if (cluster_initialized)
      {
        const int required_height = settings_cluster.Height();
        const int numerator       = required_height - (preset_count_ * button_height);
        if (numerator > 0)
        {
          min_required_gap = max(min_required_gap, (numerator + (preset_count_ - 2)) / (preset_count_ - 1));
        }
      }
    }

    button_gap = max(min_required_gap, button_gap - 10);
    stride     = button_height + button_gap;
  }

  CFont* template_font = template_off_button ? template_off_button->GetFont() : GetFont();

  preset_buttons_.clear();
  preset_buttons_.reserve(preset_count_);

  for (int preset = PRESET_OFF; preset < preset_count_; ++preset)
  {
    auto  button = std::make_unique<CButton>();
    CRect rect(base_rect.left,
               base_rect.top + stride * preset,
               base_rect.right,
               base_rect.top + stride * preset + base_rect.Height());

    button->Create(_T(""), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                   rect, this, GetPresetButtonControlId(preset));

    if (template_font)
      button->SetFont(template_font);

    preset_buttons_.push_back(std::move(button));
  }

  RefreshAllPresetButtonCaptions();
}

void CFilterKeySettingDlg::LayoutDynamicControls()
{
  if (preset_buttons_.empty())
    return;

  const int group_visual_offset_y = 7;

  std::vector<UINT> centered_settings_ids = {
    IDC_CHECK_EDIT_MODE,
    IDC_EDIT_ACCEPT_DELAY,
    IDC_EDIT_REPEAT_DELAY,
    IDC_EDIT_REPEAT_RATE,
    IDC_STATIC_LABEL_ACCEPT,
    IDC_STATIC_LABEL_ACCEPT_MS,
    IDC_STATIC_LABEL_REPEAT_DELAY,
    IDC_STATIC_LABEL_REPEAT_DELAY_MS,
    IDC_STATIC_LABEL_REPEAT_RATE,
    IDC_STATIC_LABEL_REPEAT_RATE_MS,
  };

  CRect first_button;
  CRect last_button;
  preset_buttons_.front()->GetWindowRect(&first_button);
  preset_buttons_.back()->GetWindowRect(&last_button);
  ScreenToClient(&first_button);
  ScreenToClient(&last_button);

  const int button_top    = first_button.top;
  const int button_bottom = last_button.bottom;

  bool  cluster_initialized = false;
  CRect settings_cluster;
  for (auto id : centered_settings_ids)
  {
    if (auto ctrl = GetDlgItem(id); ctrl)
    {
      CRect rect;
      ctrl->GetWindowRect(&rect);
      ScreenToClient(&rect);

      if (!cluster_initialized)
      {
        settings_cluster    = rect;
        cluster_initialized = true;
      }
      else
      {
        settings_cluster.UnionRect(settings_cluster, rect);
      }
    }
  }

  if (cluster_initialized)
  {
    const int target_center_y  = (button_top + button_bottom) / 2 - group_visual_offset_y;
    const int current_center_y = (settings_cluster.top + settings_cluster.bottom) / 2;
    const int delta_y          = target_center_y - current_center_y;

    for (auto id : centered_settings_ids)
    {
      if (auto ctrl = GetDlgItem(id); ctrl)
      {
        CRect rect;
        ctrl->GetWindowRect(&rect);
        ScreenToClient(&rect);
        ctrl->SetWindowPos(nullptr, rect.left, rect.top + delta_y,
                           0, 0, SWP_NOZORDER | SWP_NOSIZE);
      }
    }
  }

  if (auto settings_group = GetDlgItem(IDC_GROUP_SETTINGS); settings_group)
  {
    CRect group_rect;
    settings_group->GetWindowRect(&group_rect);
    ScreenToClient(&group_rect);

    const int group_top_for_layout = max(0, button_top);
    const int group_top_for_render = max(0, group_top_for_layout - group_visual_offset_y);
    const int group_height         = max(1, button_bottom - group_top_for_render);

    settings_group->SetWindowPos(nullptr,
                                 group_rect.left,
                                 group_top_for_render,
                                 group_rect.Width(),
                                 group_height,
                                 SWP_NOZORDER);

    if (auto edit_mode = GetDlgItem(IDC_CHECK_EDIT_MODE); edit_mode)
    {
      CRect edit_rect;
      edit_mode->GetWindowRect(&edit_rect);
      ScreenToClient(&edit_rect);

      const int group_border_line_y = group_top_for_layout + 2;
      const int edit_top            = group_border_line_y - (edit_rect.Height() / 2);
      edit_mode->SetWindowPos(nullptr,
                              edit_rect.left,
                              edit_top,
                              0, 0,
                              SWP_NOZORDER | SWP_NOSIZE);
    }
  }

  std::vector<UINT> lower_ids = {
    IDC_EDIT_TESTING,
    IDC_CHECK_RESTORE_SETTING,
    IDC_CHECK_MOVE_TO_TRAY,
    IDC_CHECK_DISABLE_HOTKEY,
    IDC_CHECK_ENABLE_KEYBIND,
    IDC_CHECK_SET_MOUSE_DBLCLICK_TRACKER,
    IDC_CHECK_DISABLE_WITH_ESC,
    IDC_CHECK_ENABLE_TOGGLE_KEYBIND,
    IDC_EDIT_TOGGLE_KEYBIND,
  };

  bool  lower_initialized = false;
  CRect lower_cluster;
  for (auto id : lower_ids)
  {
    if (auto ctrl = GetDlgItem(id); ctrl)
    {
      CRect rect;
      ctrl->GetWindowRect(&rect);
      ScreenToClient(&rect);

      if (!lower_initialized)
      {
        lower_cluster     = rect;
        lower_initialized = true;
      }
      else
      {
        lower_cluster.UnionRect(lower_cluster, rect);
      }
    }
  }

  if (lower_initialized)
  {
    int next_top = button_bottom + 10;

    CRect group_rect;
    if (auto settings_group = GetDlgItem(IDC_GROUP_SETTINGS); settings_group)
    {
      settings_group->GetWindowRect(&group_rect);
      ScreenToClient(&group_rect);
      next_top = max(next_top, group_rect.bottom + 10);
    }

    const int lower_delta = next_top - lower_cluster.top;
    for (auto id : lower_ids)
    {
      if (auto ctrl = GetDlgItem(id); ctrl)
      {
        CRect rect;
        ctrl->GetWindowRect(&rect);
        ScreenToClient(&rect);
        ctrl->SetWindowPos(nullptr, rect.left, rect.top + lower_delta,
                           0, 0, SWP_NOZORDER | SWP_NOSIZE);
      }
    }

    CRect dialog_rect;
    GetWindowRect(&dialog_rect);
    CRect client_rect;
    GetClientRect(&client_rect);
    const int frame_extra       = dialog_rect.Height() - client_rect.Height();
    const int bottom_margin     = client_rect.bottom - lower_cluster.bottom;
    const int new_client_height = lower_cluster.bottom + lower_delta + bottom_margin;

    SetWindowPos(nullptr, 0, 0, dialog_rect.Width(),
                 new_client_height + frame_extra,
                 SWP_NOMOVE | SWP_NOZORDER);
  }
}

UINT CFilterKeySettingDlg::GetPresetButtonControlId(const int preset) const
{
  return dynamic_preset_button_base_ + static_cast<UINT>(preset);
}

bool CFilterKeySettingDlg::IsEditModeChecked() const
{
  auto edit_button = static_cast<CButton*>(GetDlgItem(IDC_CHECK_EDIT_MODE));
  return (edit_button && edit_button->GetCheck() == BST_CHECKED);
}

bool CFilterKeySettingDlg::IsDialogForeground() const
{
  HWND owner = GetSafeHwnd();
  if (!::IsWindow(owner))
    return false;

  HWND fg = ::GetForegroundWindow();
  if (!::IsWindow(fg))
    return false;

  if (fg == owner)
    return true;

  if (::IsChild(owner, fg))
    return true;

  return (::GetWindow(fg, GW_OWNER) == owner);
}

bool CFilterKeySettingDlg::SaveCurrentEditingValues(const int target_preset, bool* changed)
{
  if (changed)
    *changed = false;

  if (!PRESET_IS_ON(target_preset))
    return true;

  PresetValues values = {};
  if (!ReadPresetValuesFromEdits(this, &values))
    return false;

  auto validation = PresetService::ValidateValues(values);
  if (!validation.ok)
  {
    AfxMessageBox(validation.error_message);
    return false;
  }

  return PresetService::SavePresetValues(target_preset, values, changed);
}

void CFilterKeySettingDlg::OnCommandPresetButton(UINT nID)
{
  const int preset = static_cast<int>(nID - dynamic_preset_button_base_);
  BnClickPreset(preset);
}
