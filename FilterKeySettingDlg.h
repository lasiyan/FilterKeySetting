
// FilterKeySettingDlg.h : header file
//

#pragma once

#include <array>
#include <memory>
#include <vector>

#include "UserMouseTracker.hpp"
#include "UserOption.hpp"
#include "UserTooltip.hpp"

class DialogDebug;

// CFilterKeySettingDlg dialog
class CFilterKeySettingDlg : public CDialogEx
{
 public:
  CFilterKeySettingDlg(CWnd* pParent = nullptr);
  ~CFilterKeySettingDlg();

#ifdef AFX_DESIGN_TIME
  enum
  {
    IDD = IDD_FILTERKEYSETTING_DIALOG
  };
#endif

 protected:
  virtual void DoDataExchange(CDataExchange* pDX);  // DDX/DDV support

  // MFC message handlers
  HICON           m_hIcon;
  BOOL            OnInitDialog() override;
  afx_msg void    OnPaint();
  afx_msg HCURSOR OnQueryDragIcon();
  afx_msg void    OnDestroy();
  afx_msg void    OnHotKey(UINT nHotKeyId, UINT nKey1, UINT nKey2);
  afx_msg LRESULT OnRawInput(WPARAM wParam, LPARAM lParam);
  afx_msg void    OnSize(UINT nType, int cx, int cy);
  afx_msg void    OnTimer(UINT_PTR nIDEvent);
  afx_msg HBRUSH  OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
  afx_msg LRESULT OnTrayIcon(WPARAM wParam, LPARAM lParam);
  afx_msg LRESULT OnMouseTrackerTriggered(WPARAM wParam, LPARAM lParam);
  afx_msg LRESULT OnDebugDialogClosed(WPARAM wParam, LPARAM lParam);
  afx_msg LRESULT OnDebugOptionsChanged(WPARAM wParam, LPARAM lParam);
  afx_msg LRESULT OnActivateExisting(WPARAM wParam, LPARAM lParam);
  afx_msg LRESULT OnClearAppDataAndExit(WPARAM wParam, LPARAM lParam);
  afx_msg LRESULT OnStartMinimizedToTray(WPARAM wParam, LPARAM lParam);
  BOOL            PreTranslateMessage(MSG* pMsg) override;
  DECLARE_MESSAGE_MAP()

  // Button / checkbox click handlers
  afx_msg void OnCommandPresetButton(UINT nID);
  afx_msg void OnBnClickedCheckEditMode();
  afx_msg void OnBnClickedCheckRestoreSetting();
  afx_msg void OnBnClickedCheckMoveToTray();
  afx_msg void OnBnClickedCheckEnableKeybind();
  afx_msg void OnBnClickedCheckEnableToggleKeybind();
  afx_msg void OnBnClickedCheckSetMouseDblclickTracker();
  afx_msg void OnBnClickedCheckDisableWithEsc();
  afx_msg void OnBnClickedCheckDisableWithEnter();

  // Focus handlers
  afx_msg void OnEnSetFocusTesting();
  afx_msg void OnEnKillFocusTesting();
  afx_msg void OnEnSetFocusToggleKeybind();

  // Preset operations
  void BnClickPreset(const int preset);
  void ActivatePreset(const int preset, BOOL alert, BOOL beep = FALSE, LPCTSTR reason = nullptr);
  bool SaveCurrentEditingValues(const int target_preset, bool* changed = nullptr);
  bool ConfirmSaveIfEditingAndDirty(const int next_preset);
  void ResetEditMode();
  void UpdateEditModeCaption();
  void EnsureEditModeHintLabel();

  // Dialog / popup
  void PopupRenameDialog(const int preset);
  void PopupKeyBindingDialog(const int preset);
  void PopupToggleKeyBindingDialog();

 protected:
  void OpenDeveloperDebugDialog();

  // Option sync
  void UpdateOption(BOOL write = TRUE);
  void SyncOptionsFromUI();
  void SyncOptionsToUI();
  void ApplySubsystemOptions();

  // UI update
  void           UpdateInterface(const int preset);
  void           RefreshPresetButtonCaption(const int preset);
  void           RefreshAllPresetButtonCaptions();
  void           RefreshToggleHotkeyEditText();
  void           HandleBackgroundKeyOff(UINT vk, LPCTSTR option_key, LPCTSTR reason);
  void           UpdateProcessWatcherRegistration();
  void           TickProcessWatcher();
  static CString GetForegroundProcessName();
  bool           ValidateActiveHotkeysAndAlert() const;

  // Tray icon
  bool AddTrayIcon();
  void RemoveTrayIcon();
  void RestoreFromTray();
  void BringDialogToForeground();

  // Hotkey registration
  void RegisterPresetHotkeys();
  void UnregisterPresetHotkeys();
  bool RegisterRawInputHotkeys();
  void UnregisterRawInputHotkeys();
  void SyncRawInputDevice();
  void UpdateRawInputModifierState(UINT vk, bool key_down);
  int  ResolveRawInputHotkeyId(UINT vk, UINT modifiers) const;
  bool HandleResolvedHotkeyId(UINT hotkey_id, bool from_raw_input);
  bool EnsureAdminGuardForOptionEnable(const CString& option_key, bool request_enable, bool previous_enabled);
  bool EnsureAdminGuardOnStartup();

  // Layout / init
  void InitializePresetCount();
  void BuildPresetButtons();
  void LayoutDynamicControls();

  // Query
  UINT GetPresetButtonControlId(const int preset) const;
  bool IsEditModeChecked() const;
  bool IsDialogForeground() const;

 private:
  // Preset state
  int last_selected_      = PRESET_OFF;
  int preset_before_edit_ = PRESET_OFF;
  int preset_count_       = 0;
  int last_on_preset_     = 1;

  // Message IDs and timer IDs
 public:
  static constexpr UINT     WM_TRAYICON_MSG             = WM_APP + 1;
  static constexpr UINT     WM_CLEAR_APP_DATA_AND_EXIT  = WM_APP + 62;
  static constexpr UINT     WM_START_MINIMIZED_TO_TRAY  = WM_APP + 63;
  static constexpr UINT     WM_ACTIVATE_EXISTING_INST   = WM_APP + 100;
  static constexpr UINT_PTR TIMER_PROCESS_WATCHER       = 0x12F2;
  static constexpr UINT     TRAY_ICON_ID                = 1;
  static constexpr UINT     dynamic_preset_button_base_ = 2000;

  // Tray icon
 private:
  NOTIFYICONDATA tray_icon_data_  = {};
  bool           tray_icon_added_ = false;

  // Hotkey registration state
  bool                  hotkey_registered_[PRESET_MAX_COUNT] = { false };
  bool                  toggle_hotkey_registered_            = false;
  bool                  raw_input_registered_                = false;
  bool                  using_raw_input_mode_                = false;
  UINT                  raw_input_modifiers_down_            = 0;
  std::array<bool, 256> raw_input_key_down_                  = {};

  CStatic  edit_mode_hint_label_;
  bool     edit_mode_hint_label_ready_ = false;
  COLORREF edit_mode_hint_text_color_  = RGB(220, 20, 60);

  // Dynamic UI
  std::vector<std::unique_ptr<CButton>> preset_buttons_;
  bool                                  alt_hotkey_view_              = false;
  bool                                  opening_toggle_hotkey_dialog_ = false;

  // Process watcher
  bool    process_watch_active_       = false;
  bool    process_watch_was_focused_  = false;
  bool    process_watch_auto_off_     = false;
  bool    process_watch_switching_    = false;
  int     process_watch_saved_preset_ = PRESET_OFF;
  CString process_watch_target_;
  HWND    process_watch_last_fg_ = nullptr;

  // Subsystems
  Tooltip      tooltip_;
  MouseTracker mouse_tracker_;
  DialogDebug* debug_dialog_ = nullptr;
};
