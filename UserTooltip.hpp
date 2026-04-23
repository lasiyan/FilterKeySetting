#pragma once

#include <afxcmn.h>

#include "UserDefine.hpp"
#include "resource.h"

class Tooltip
{
 private:
  struct TooltipItem
  {
    UINT control_id;
    UINT ids;
  };
  static constexpr TooltipItem TOOLTIP_TABLE[] = {
    // Main Dialog
    { IDC_EDIT_ACCEPT_DELAY, IDS_TIP_EDIT_ACCEPT_DELAY },
    { IDC_EDIT_REPEAT_DELAY, IDS_TIP_EDIT_REPEAT_DELAY },
    { IDC_EDIT_REPEAT_RATE, IDS_TIP_EDIT_REPEAT_RATE },
    { IDC_CHECK_EDIT_MODE, IDS_TIP_CHECK_EDIT_MODE },
    { IDC_CHECK_RESTORE_SETTING, IDS_TIP_CHECK_RESTORE_SETTING },
    { IDC_CHECK_ENABLE_KEYBIND, IDS_TIP_CHECK_ENABLE_KEYBIND },
    { IDC_CHECK_ENABLE_TOGGLE_KEYBIND, IDS_TIP_CHECK_ENABLE_TOGGLE_KEYBIND },
    { IDC_CHECK_SET_MOUSE_DBLCLICK_TRACKER, IDS_TIP_CHECK_SET_MOUSE_DBLCLICK_TRACKER },
    { IDC_CHECK_DISABLE_WITH_ESC, IDS_TIP_CHECK_DISABLE_WITH_ESC },

    // Debug Dialog
    { IDC_CHECK_DBG_MOVE_TRACKER, IDS_TIP_CHECK_DBG_MOVE_TRACKER },
    { IDC_CHECK_SYNC_FILTERKEY, IDS_TIP_CHECK_SYNC_FILTERKEY },
    { IDC_CHECK_MUTE_SOUND, IDS_TIP_CHECK_MUTE_SOUND },
    { IDC_CHECK_OFF_USE_WINDOWS_DEFAULT, IDS_TIP_CHECK_OFF_USE_WINDOWS_DEFAULT },
    { IDC_CHECK_ENABLE_PRESET_OSD, IDS_TIP_CHECK_ENABLE_PRESET_OSD },
    { IDC_COMBO_PRESET_OSD_CORNER, IDS_TIP_COMBO_PRESET_OSD_CORNER },
    { IDC_SLIDER_PRESET_OSD_ALPHA, IDS_TIP_SLIDER_PRESET_OSD_ALPHA },
    { IDC_RADIO_PRESET_OSD_KEEP, IDS_TIP_RADIO_PRESET_OSD_KEEP },
    { IDC_RADIO_PRESET_OSD_3SEC, IDS_TIP_RADIO_PRESET_OSD_3SEC },
    { IDC_COMBO_PRESET_OSD_SIZE, IDS_TIP_COMBO_PRESET_OSD_SIZE },
    { IDC_COMBO_MOVE_SENSITIVITY, IDS_TIP_COMBO_MOVE_SENSITIVITY },
    { IDC_CHECK_AUTO_START, IDS_TIP_CHECK_AUTO_START },
    { IDC_CHECK_PRESET_OFF_PROCESS, IDS_TIP_CHECK_PRESET_OFF_PROCESS },
    { IDC_CHECK_PRESET_OFF_PROCESS_RESTORE, IDS_TIP_CHECK_PRESET_OFF_PROCESS_RESTORE },
    { IDC_EDIT_PRESET_OFF_PROCESS, IDS_TIP_EDIT_PRESET_OFF_PROCESS },
    { IDC_CHECK_IF_FULL_SCREEN_GAME, IDS_TIP_CHECK_IF_FULL_SCREEN_GAME },
  };

 public:
  // Initialization
  bool Initialize(CWnd* owner)
  {
    if (!owner || !::IsWindow(owner->GetSafeHwnd()))
      return false;

    if (!control_.Create(owner, TTS_ALWAYSTIP | TTS_NOPREFIX))
      return false;

    static constexpr int HOVER_DELAY_MS = 500;  // 마우스 오버 후 툴팁이 나타나기까지의 지연 시간 (ms)

    control_.Activate(TRUE);
    control_.SetMaxTipWidth(420);
    control_.SetDelayTime(TTDT_INITIAL, HOVER_DELAY_MS);
    control_.SetDelayTime(TTDT_RESHOW, 120);
    control_.SetDelayTime(TTDT_AUTOPOP, 15000);

    for (int index = 0; index < _countof(TOOLTIP_TABLE); ++index)
    {
      if (auto* control = owner->GetDlgItem(TOOLTIP_TABLE[index].control_id); control)
        control_.AddTool(control, TOOLTIP_TABLE[index].ids);
    }

    return true;
  }

  // Preset button tooltip
  void RegisterPreset(CWnd* button)
  {
    if (button)
      control_.AddTool(button, IDS_TIP_PRESET_BUTTON);
  }

  // Message relay
  void RelayEvent(MSG* pMsg)
  {
    if (control_.GetSafeHwnd())
      control_.RelayEvent(pMsg);
  }

 private:
  CToolTipCtrl control_;
};
