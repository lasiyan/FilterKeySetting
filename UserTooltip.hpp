#pragma once

#include <afxcmn.h>

#include "UserDefine.hpp"
#include "resource.h"

class Tooltip
{
 private:
  struct TooltipItem
  {
    UINT    control_id;
    LPCTSTR text;
  };
  static constexpr TooltipItem TOOLTIP_TABLE[] = {
    // Main Dialog
    { IDC_EDIT_ACCEPT_DELAY, _T("키 입력 인식까지의 대기 시간(ms)입니다") },
    { IDC_EDIT_REPEAT_DELAY, _T("키 반복이 시작되기 전 대기 시간(ms)입니다") },
    { IDC_EDIT_REPEAT_RATE, _T("키 반복 입력 간격(ms)입니다") },
    { IDC_CHECK_EDIT_MODE, _T("체크하면 현재 프리셋의 값을 수정할 수 있고, 체크 해제 시 값이 유효한 경우 저장됩니다") },
    { IDC_CHECK_RESTORE_SETTING, _T("프로그램 종료 시 필터키를 OFF 상태로 전환합니다\n체크하지 않을 경우 프로그램이 종료되어도 활성화된 프리셋이 유지됩니다") },
    { IDC_CHECK_ENABLE_KEYBIND, _T("키 바인딩에 따라 프리셋을 전환하는 기능을 활성화합니다\nAlt+프리셋 버튼 클릭으로 각 프리셋의 단축키를 설정할 수 있습니다") },
    { IDC_CHECK_ENABLE_TOGGLE_KEYBIND, _T("토글 단축키로 마지막 ON 프리셋과 OFF를 전환하는 기능을 활성화합니다") },
    { IDC_CHECK_SET_MOUSE_DBLCLICK_TRACKER, _T("프로그램이 백그라운드 상태일 때, 마우스 더블클릭 시 프리셋을 OFF로 전환합니다") },
    { IDC_CHECK_DISABLE_WITH_ESC, _T("프로그램이 백그라운드 상태일 때, ESC 입력 시 프리셋을 OFF로 전환합니다") },

    // Debug Dialog
    { IDC_CHECK_DBG_MOVE_TRACKER, _T("프로그램이 백그라운드 상태일 때, 마우스를 일정 거리 이상 이동하면 프리셋을 OFF로 전환합니다") },
    { IDC_BTN_DBG_APPLY_PRESET_COUNT, _T("선택한 프리셋 개수를 저장합니다. 프로그램을 재시작 후 적용됩니다") },
    { IDC_CHECK_SYNC_FILTERKEY, _T("필터키 적용 시 시스템 변경 알림을 함께 보냅니다\n필터키 간 약간의 블로킹(멈춤) 현상이 발생할 수 있습니다\n제어판과 이 프로그램을 동시에 사용할 경우 체크하세요") },
    { IDC_CHECK_MUTE_SOUND, _T("프리셋 적용 사운드를 재생하지 않습니다") },
    { IDC_CHECK_OFF_USE_WINDOWS_DEFAULT, _T("OFF 전환 시 Windows 기본 설정을 사용합니다\n체크가 해제된 경우 프로그램 실행 전 레지스트리 설정이 유지됩니다") },
    { IDC_CHECK_ENABLE_PRESET_OSD, _T("프리셋 변경 시 화면에 OSD를 표시합니다") },
    { IDC_COMBO_PRESET_OSD_CORNER, _T("OSD가 나타날 화면 위치를 선택합니다") },
    { IDC_SLIDER_PRESET_OSD_ALPHA, _T("OSD 투명도를 조절합니다") },
    { IDC_RADIO_PRESET_OSD_KEEP, _T("OSD를 다음 프리셋 변경 전까지 유지합니다") },
    { IDC_RADIO_PRESET_OSD_3SEC, _T("OSD를 3초 후 자동으로 숨깁니다") },
    { IDC_COMBO_PRESET_OSD_SIZE, _T("OSD 글자 크기를 선택합니다") },
    { IDC_COMBO_MOVE_SENSITIVITY, _T("마우스 이동에 따른 종료 트리거 감도를 선택합니다\n높을수록 작은 움직임에도 필터키가 종료됩니다") },
    { IDC_CHECK_AUTO_START, _T("윈도우 시작 시 프로그램을 자동으로 실행합니다. 시스템 환경에 따라 시작 프로그램 등록이 실패할 수 있습니다. 일부 환경에서는 관리자 권한이 필요할 수 있습니다") },
    { IDC_CHECK_PRESET_OFF_PROCESS, _T("프리셋이 설정된 상태에서 특정 프로세스에서 벗어나면 프리셋을 OFF로 전환합니다") },
    { IDC_CHECK_PRESET_OFF_PROCESS_RESTORE, _T("프리셋이 OFF로 전환된 후, 다시 해당 프로세스에 진입하면 이전 프리셋으로 복원합니다\n단, 도중에 다른 프리셋으로 설정된 경우에는 복원되지 않습니다") },
    { IDC_EDIT_PRESET_OFF_PROCESS, _T("기능을 사용할 프로세스를 선택합니다(예: MapleStory.exe)\n마우스 클릭 시 선택창이 나타납니다\n마우스 우클릭 시 프로세스를 초기화합니다") },
    { IDC_CHECK_IF_FULL_SCREEN_GAME, _T("체크하면 단축키 처리에 HotKey 대신 RawInput을 사용합니다\n전체화면 게임 환경 호환성 개선을 위한 옵션입니다\n일반 권한에서는 단축키가 동작하지 않을 수 있습니다") },
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
        control_.AddTool(control, TOOLTIP_TABLE[index].text);
    }

    return true;
  }

  // Preset button tooltip
  void RegisterPreset(CWnd* button)
  {
    if (button)
      control_.AddTool(button, _T("클릭: 프리셋 적용\nCtrl+클릭: 프리셋 이름 변경\nAlt+클릭: 단축키 지정 (기능 활성화 시)"));
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
