// clang-format off
#include "pch.h"
#include "UserAdminGuard.hpp"
#include "UserDefine.hpp"
#include "UserFilterKey.hpp"
// clang-format on

namespace AdminGuard {
namespace {

constexpr int kBtnProceed = 1001;
constexpr int kBtnRestart = 1002;
constexpr int kBtnCancel  = 1003;

struct AdminRequiredOption
{
  LPCTSTR key;
  LPCTSTR label;
};

constexpr std::array<AdminRequiredOption, 7> kAdminRequiredOptions = {
  AdminRequiredOption{ KEY_ENABLE_KEYBIND, _T("단축키를 통한 프리셋 변경 기능 사용 (Alt + 프리셋 버튼 클릭)") },
  AdminRequiredOption{ KEY_ENABLE_TOGGLE_KEYBIND, _T("단축키를 통한 프리셋 토글 기능 활성화") },
  AdminRequiredOption{ KEY_DISABLE_WITH_ESC, _T("백그라운드에서 ESC 버튼이 입력되면 필터키 끄기") },
  AdminRequiredOption{ KEY_IF_FULL_SCREEN_GAME, _T("전체화면 게임 안에서 단축키 기능 활성화") },
  AdminRequiredOption{ KEY_ENABLE_MOUSE_MOVE_TRACKER, _T("백그라운드 마우스 이동 시 필터키 종료") },
  AdminRequiredOption{ KEY_ENABLE_MOUSE_DBLCLICK_TRACKER, _T("백그라운드에서 마우스 더블클릭 시 필터키 끄기") },
  AdminRequiredOption{ KEY_PROCESS_OFF_ENABLED, _T("특정 프로세스에서 벗어나면 필터키 종료") },
};

CString FindOptionLabel(const CString& option_key)
{
  for (const auto& option : kAdminRequiredOptions)
  {
    if (option_key.Compare(option.key) == 0)
      return CString(option.label);
  }

  return CString();
}

bool IsAdminRequiredOptionKey(const CString& option_key)
{
  for (const auto& option : kAdminRequiredOptions)
  {
    if (option_key.Compare(option.key) == 0)
      return true;
  }

  return false;
}

CString BuildEnabledOptionSummary()
{
  CString summary;
  for (const auto& option : kAdminRequiredOptions)
  {
    if (GLOBAL_OPTION.getInteger(option.key, 0) == 0)
      continue;

    if (!summary.IsEmpty())
      summary += _T("\r\n");

    summary += _T("- ");
    summary += option.label;
  }

  return summary;
}

CString BuildPromptMessage(const CString* option_key)
{
  CString content;
  if (option_key != nullptr)
  {
    CString label = FindOptionLabel(*option_key);
    if (!label.IsEmpty())
    {
      content.Format(_T("다음 옵션은 관리자 권한에서 더 안정적으로 동작합니다.\r\n- %s"),
                     static_cast<LPCTSTR>(label));
    }
  }

  if (content.IsEmpty())
  {
    CString enabled = BuildEnabledOptionSummary();
    if (!enabled.IsEmpty())
    {
      content = _T("다음 활성화된 옵션은 관리자 권한에서 더 안정적으로 동작합니다.\r\n");
      content += enabled;
    }
    else
    {
      content = _T("일부 기능은 관리자 권한에서 더 안정적으로 동작합니다.");
    }
  }

  content += _T("\r\n\r\n관리자 권한으로 프로그램을 다시 실행하시겠습니까?");

  return content;
}

HWND ResolvePromptOwnerWindow(CWnd* owner)
{
  if (owner && ::IsWindow(owner->GetSafeHwnd()))
    return owner->GetSafeHwnd();

  HWND fg = ::GetForegroundWindow();
  if (::IsWindow(fg))
    return fg;

  return nullptr;
}

void ShowRestartFailedMessage(HWND parent)
{
  ::MessageBox(parent, _T("프로그램 재시작에 실패했습니다."), _T("알림"), MB_OK | MB_ICONERROR | MB_TOPMOST);
}

void RequestForeground(HWND parent)
{
  if (::IsWindow(parent))
    ::SetForegroundWindow(parent);
}

struct PromptPositionContext
{
  HMONITOR monitor = nullptr;
};

HRESULT CALLBACK PromptCallback(HWND hwnd, UINT notification, WPARAM wParam, LPARAM lParam, LONG_PTR ref_data)
{
  UNREFERENCED_PARAMETER(wParam);
  UNREFERENCED_PARAMETER(lParam);

  if (notification != TDN_CREATED)
    return S_OK;

  const auto* context = reinterpret_cast<const PromptPositionContext*>(ref_data);
  HMONITOR    monitor = context ? context->monitor : nullptr;
  if (monitor == nullptr)
    monitor = ::MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);

  MONITORINFO info = {};
  info.cbSize      = sizeof(info);
  if (!::GetMonitorInfo(monitor, &info))
    return S_OK;

  RECT dialog_rect = {};
  if (!::GetWindowRect(hwnd, &dialog_rect))
    return S_OK;

  const int width  = dialog_rect.right - dialog_rect.left;
  const int height = dialog_rect.bottom - dialog_rect.top;
  const int x      = info.rcWork.left + ((info.rcWork.right - info.rcWork.left - width) / 2);
  const int y      = info.rcWork.top + ((info.rcWork.bottom - info.rcWork.top - height) / 2);

  ::SetWindowPos(hwnd, HWND_TOP, x, y, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE);
  return S_OK;
}

}  // namespace

const std::array<LPCTSTR, 7>& GetAdminRequiredOptionList()
{
  static const auto keys = []() {
    std::array<LPCTSTR, kAdminRequiredOptions.size()> values = {};
    for (size_t i = 0; i < kAdminRequiredOptions.size(); ++i)
      values[i] = kAdminRequiredOptions[i].key;
    return values;
  }();
  return keys;
}

bool IsAdminRequiredOptionEnabled(const CString& option_key)
{
  if (!IsAdminRequiredOptionKey(option_key))
    return false;

  return (GLOBAL_OPTION.getInteger(option_key, 0) != 0);
}

bool HasAnyAdminRequiredOptionEnabled()
{
  for (const auto& option : kAdminRequiredOptions)
  {
    if (GLOBAL_OPTION.getInteger(option.key, 0) != 0)
      return true;
  }

  return false;
}

PromptResult PromptAdminRestartIfNeeded(CWnd* owner, const CString* option_key)
{
  if (FilterKey::IsProcessElevatedNow())
    return PromptResult::Proceed;

  if (option_key != nullptr && !IsAdminRequiredOptionKey(*option_key))
    return PromptResult::Proceed;

  if (option_key == nullptr && !HasAnyAdminRequiredOptionEnabled())
    return PromptResult::Proceed;

  const CString content = BuildPromptMessage(option_key);
  HWND          parent  = ResolvePromptOwnerWindow(owner);
  HWND          fg      = ::GetForegroundWindow();

  PromptPositionContext position_context = {};
  if (::IsWindow(fg))
    position_context.monitor = ::MonitorFromWindow(fg, MONITOR_DEFAULTTONEAREST);
  else if (::IsWindow(parent))
    position_context.monitor = ::MonitorFromWindow(parent, MONITOR_DEFAULTTONEAREST);

  TASKDIALOG_BUTTON buttons[] = {
    { kBtnProceed, _T("권한 없이 진행") },
    { kBtnRestart, _T("관리자 권한으로 재실행") },
    { kBtnCancel, _T("취소") },
  };

  TASKDIALOGCONFIG config = {};
  config.cbSize           = sizeof(config);
  config.hwndParent       = parent;
  config.dwFlags          = TDF_POSITION_RELATIVE_TO_WINDOW;
  config.pszWindowTitle   = _T("알림");
  config.pszContent       = content;
  config.pButtons         = buttons;
  config.cButtons         = _countof(buttons);
  config.nDefaultButton   = kBtnProceed;
  config.pfCallback       = &PromptCallback;
  config.lpCallbackData   = reinterpret_cast<LONG_PTR>(&position_context);

  int pressed = 0;
  if (SUCCEEDED(::TaskDialogIndirect(&config, &pressed, nullptr, nullptr)))
  {
    if (pressed == kBtnProceed)
      return PromptResult::Proceed;

    if (pressed == kBtnCancel)
      return PromptResult::Cancelled;

    if (pressed == kBtnRestart)
    {
      RequestForeground(parent);
      if (!FilterKey::RestartProgram(owner, true))
      {
        ShowRestartFailedMessage(parent);
        return PromptResult::Cancelled;
      }

      return PromptResult::RestartRequested;
    }
  }

  CString fallback_message = content;
  fallback_message += _T("\r\n\r\n[예] 권한 없이 진행\r\n[아니오] 관리자 권한으로 실행\r\n[취소] 취소");
  const int fallback = ::MessageBox(parent, fallback_message, _T("알림"), MB_YESNOCANCEL | MB_ICONINFORMATION | MB_TOPMOST);

  if (fallback == IDYES)
    return PromptResult::Proceed;

  if (fallback == IDCANCEL)
    return PromptResult::Cancelled;

  RequestForeground(parent);
  if (!FilterKey::RestartProgram(owner, true))
  {
    ShowRestartFailedMessage(parent);
    return PromptResult::Cancelled;
  }

  return PromptResult::RestartRequested;
}

}  // namespace AdminGuard
