#pragma once

#include "UserDevLog.hpp"
#include "UserOption.hpp"
#include "UserTooltip.hpp"

class DialogDebug : public CDialogEx, public DevLogSink
{
  DECLARE_DYNAMIC(DialogDebug)

 public:
  explicit DialogDebug(CWnd* pParent = nullptr);
  ~DialogDebug() override;

#ifdef AFX_DESIGN_TIME
  enum
  {
    IDD = IDD_DEBUG_CONSOLE
  };
#endif

 protected:
  // MFC overrides
  void DoDataExchange(CDataExchange* pDX) override;
  BOOL OnInitDialog() override;
  BOOL PreTranslateMessage(MSG* pMsg) override;
  void OnCancel() override;
  void PostNcDestroy() override;

  // Message handlers
  afx_msg void OnClose();
  afx_msg void OnDestroy();
  afx_msg void OnBnClickedCheckDbgMoveTracker();
  afx_msg void OnBnClickedCheckSyncFilterkey();
  afx_msg void OnBnClickedCheckMuteSound();
  afx_msg void OnBnClickedCheckOffUseWindowsDefault();
  afx_msg void OnBnClickedCheckEnablePresetOsd();
  afx_msg void OnCbnSelchangeComboPresetOsdCorner();
  afx_msg void OnCbnSelchangeComboDbgPresetCount();
  afx_msg void OnCbnSelchangeComboPresetOsdSize();
  afx_msg void OnBnClickedRadioPresetOsdKeep();
  afx_msg void OnBnClickedRadioPresetOsd3sec();
  afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
  afx_msg void OnCbnSelchangeComboMouseSensitivity();
  afx_msg void OnBnClickedCheckAutoStart();
  afx_msg void OnBnClickedCheckPresetOffProcess();
  afx_msg void OnBnClickedCheckPresetOffProcessRestore();
  afx_msg void OnBnClickedCheckIfFullScreenGame();
  afx_msg void OnBnClickedCheckDisableHotkey();
  afx_msg void OnEnKillFocusEditPresetOffProcess();
  afx_msg void OnCbnSelchangeComboLang();
  afx_msg void OnBnClickedBtnClearAppData();
  DECLARE_MESSAGE_MAP()

 public:
  // DevLogSink
  void AppendDevLog(const CString& line) override;

 private:
  // Option init / sync
  void InitializeOptions();
  void NotifyOptionsChanged();
  void InitializeOsdOptions();
  void InitializeMouseSensitivity();
  void InitializeAutoStart();
  void InitializeProcessOffOptions();
  void UpdateProcessOffControlStates();
  void BrowseProcessFile();
  void ClearProcessFile();
  void SaveOsdOptionsFromUI();

  // State
  CString log_text_;
  Tooltip tooltip_;
  bool    preset_count_combo_updating_ = false;

  static constexpr int kLogMaxChars   = 200000;
  static constexpr int kTrimKeepChars = 120000;
};
