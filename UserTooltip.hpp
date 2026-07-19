#pragma once

#include <afxcmn.h>

#include <list>

#include "UserDefine.hpp"
#include "UserLanguage.hpp"
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
    { IDC_CHECK_MOVE_TO_TRAY, IDS_TIP_CHECK_MOVE_TO_TRAY },
    { IDC_CHECK_ENABLE_KEYBIND, IDS_TIP_CHECK_ENABLE_KEYBIND },
    { IDC_CHECK_ENABLE_TOGGLE_KEYBIND, IDS_TIP_CHECK_ENABLE_TOGGLE_KEYBIND },
    { IDC_EDIT_TOGGLE_KEYBIND, IDS_TIP_EDIT_TOGGLE_KEYBIND },
    { IDC_CHECK_SET_MOUSE_DBLCLICK_TRACKER, IDS_TIP_CHECK_SET_MOUSE_DBLCLICK_TRACKER },
    { IDC_CHECK_DISABLE_WITH_ESC, IDS_TIP_CHECK_DISABLE_WITH_ESC },
    { IDC_CHECK_DISABLE_WITH_ENTER, IDS_TIP_CHECK_DISABLE_WITH_ENTER },
    { IDC_STATIC_LABEL_ACCEPT, IDS_TIP_EDIT_ACCEPT_DELAY },
    { IDC_STATIC_LABEL_REPEAT_DELAY, IDS_TIP_EDIT_REPEAT_DELAY },
    { IDC_STATIC_LABEL_REPEAT_RATE, IDS_TIP_EDIT_REPEAT_RATE },

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
    { IDC_STATIC_PRESET_COUNT_LABEL, IDS_TIP_COMBO_DBG_PRESET_COUNT },
    { IDC_COMBO_DBG_PRESET_COUNT, IDS_TIP_COMBO_DBG_PRESET_COUNT },
    { IDC_CHECK_DISABLE_HOTKEY, IDS_TIP_CHECK_DISABLE_HOTKEY },
    { IDC_STATIC_MOVE_SENSITIVITY_LABEL, IDS_TIP_COMBO_MOVE_SENSITIVITY },

    // Auto Start Dialog
    { IDC_RADIO_AUTOSTART_NORMAL, IDS_TIP_RDO_AUTOSTART_NORMAL },
    { IDC_RADIO_AUTOSTART_ADMIN, IDS_TIP_RDO_AUTOSTART_ADMIN },
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

    // std::list 사용: 요소 추가 시 기존 요소 주소가 무효화되지 않음 (툴팁이 포인터를 장기 보유).
    tooltip_texts_.clear();

    for (int index = 0; index < _countof(TOOLTIP_TABLE); ++index)
    {
      if (auto* ctrl = owner->GetDlgItem(TOOLTIP_TABLE[index].control_id); ctrl)
      {
        tooltip_texts_.emplace_back(Lang::T(TOOLTIP_TABLE[index].ids));
        auto* text = const_cast<LPTSTR>(static_cast<LPCTSTR>(tooltip_texts_.back()));

        // (1) HWND 기반 툴: 활성 컨트롤이 직접 WM_MOUSEMOVE를 받는 경우 (relay hwnd = child)
        {
          TOOLINFO ti = {};
          ti.cbSize   = sizeof(TOOLINFO);
          ti.uFlags   = TTF_IDISHWND;
          ti.hwnd     = owner->GetSafeHwnd();
          ti.uId      = reinterpret_cast<UINT_PTR>(ctrl->GetSafeHwnd());
          ti.lpszText = text;
          control_.SendMessage(TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&ti));
        }

        // (2) RECT 기반 툴: SS_NOTIFY 없는 Static(HTTRANSPARENT)·disabled 컨트롤은
        //     WM_MOUSEMOVE가 parent로 흘러가므로 parent 좌표계 rect로 등록
        {
          CRect rc;
          ctrl->GetWindowRect(&rc);
          owner->ScreenToClient(&rc);

          TOOLINFO ti = {};
          ti.cbSize   = sizeof(TOOLINFO);
          ti.uFlags   = 0;
          ti.hwnd     = owner->GetSafeHwnd();
          ti.uId      = TOOLTIP_TABLE[index].control_id;
          ti.rect     = rc;
          ti.lpszText = text;
          control_.SendMessage(TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&ti));
        }
      }
    }

    return true;
  }

  // Preset button tooltip
  void RegisterPreset(CWnd* button)
  {
    if (button && button->GetParent())
    {
      tooltip_texts_.emplace_back(Lang::T(IDS_TIP_PRESET_BUTTON));
      TOOLINFO ti = {};
      ti.cbSize   = sizeof(TOOLINFO);
      ti.uFlags   = TTF_IDISHWND | TTF_SUBCLASS;
      ti.hwnd     = button->GetParent()->GetSafeHwnd();
      ti.uId      = reinterpret_cast<UINT_PTR>(button->GetSafeHwnd());
      ti.lpszText = const_cast<LPTSTR>(static_cast<LPCTSTR>(tooltip_texts_.back()));
      control_.SendMessage(TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&ti));
    }
  }

  // Message relay
  void RelayEvent(MSG* pMsg)
  {
    if (control_.GetSafeHwnd())
      control_.RelayEvent(pMsg);
  }

 private:
  CToolTipCtrl       control_;
  std::list<CString> tooltip_texts_;  // 포인터 안정성을 위해 list 사용
};
