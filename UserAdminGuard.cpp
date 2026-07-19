// clang-format off
#include "pch.h"
#include "UserAdminGuard.hpp"
#include "UserDefine.hpp"
#include "UserFilterKey.hpp"
#include "UserLanguage.hpp"
// clang-format on

namespace AdminGuard {
namespace {

constexpr int kBtnProceed = 1001;
constexpr int kBtnRestart = 1002;
constexpr int kBtnCancel  = 1003;

struct AdminRequiredOption
{
  LPCTSTR key;
  UINT    label_ids;
};

constexpr std::array<AdminRequiredOption, 8> kAdminRequiredOptions = {
  AdminRequiredOption{ KEY_ENABLE_KEYBIND, IDS_CHK_ENABLE_KEYBIND },
  AdminRequiredOption{ KEY_ENABLE_TOGGLE_KEYBIND, IDS_CHK_TOGGLE_KEYBIND },
  AdminRequiredOption{ KEY_DISABLE_WITH_ESC, IDS_CHK_DISABLE_WITH_ESC },
  AdminRequiredOption{ KEY_DISABLE_WITH_ENTER, IDS_CHK_DISABLE_WITH_ENTER },
  AdminRequiredOption{ KEY_IF_FULL_SCREEN_GAME, IDS_CHK_IF_FULL_SCREEN_GAME },
  AdminRequiredOption{ KEY_ENABLE_MOUSE_MOVE_TRACKER, IDS_CHK_DBG_MOVE_TRACKER },
  AdminRequiredOption{ KEY_ENABLE_MOUSE_DBLCLICK_TRACKER, IDS_CHK_MOUSE_DBLCLICK_TRACKER },
  AdminRequiredOption{ KEY_PROCESS_OFF_ENABLED, IDS_CHK_PRESET_OFF_PROCESS },
};

CString StripStarPrefix(const CString& s)
{
  return (s.GetLength() >= 2 && s.Left(2) == _T("★ ")) ? s.Mid(2) : s;
}

CString FindOptionLabel(const CString& option_key)
{
  for (const auto& option : kAdminRequiredOptions)
  {
    if (option_key.Compare(option.key) == 0)
      return StripStarPrefix(Lang::T(option.label_ids));
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
    summary += StripStarPrefix(Lang::T(option.label_ids));
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
      content.Format(Lang::T(IDS_FMT_ADMIN_REQUIRED),
                     static_cast<LPCTSTR>(label));
    }
  }

  if (content.IsEmpty())
  {
    CString enabled = BuildEnabledOptionSummary();
    if (!enabled.IsEmpty())
    {
      content = Lang::T(IDS_MSG_ADMIN_ENABLED_PREFIX);
      content += enabled;
    }
    else
    {
      content = Lang::T(IDS_MSG_ADMIN_GENERIC);
    }
  }

  content += Lang::T(IDS_MSG_ADMIN_RESTART_SUFFIX);

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
  ::MessageBox(parent, Lang::T(IDS_MSG_RESTART_FAILED), GetAppName(), MB_OK | MB_ICONERROR | MB_TOPMOST);
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

const std::array<LPCTSTR, 8>& GetAdminRequiredOptionList()
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

  CString btn_proceed = Lang::T(IDS_BTN_PROCEED_NO_ADMIN);
  CString btn_restart = Lang::T(IDS_BTN_RESTART_AS_ADMIN);
  CString btn_cancel  = Lang::T(IDS_BTN_CANCEL);

  TASKDIALOG_BUTTON buttons[] = {
    { kBtnProceed, static_cast<LPCTSTR>(btn_proceed) },
    { kBtnRestart, static_cast<LPCTSTR>(btn_restart) },
    { kBtnCancel, static_cast<LPCTSTR>(btn_cancel) },
  };

  TASKDIALOGCONFIG config = {};
  config.cbSize           = sizeof(config);
  config.hwndParent       = parent;
  config.dwFlags          = TDF_POSITION_RELATIVE_TO_WINDOW;
  config.pszWindowTitle   = GetAppName();
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
  fallback_message += Lang::T(IDS_FMT_ADMIN_FALLBACK);
  const int fallback = ::MessageBox(parent, fallback_message, GetAppName(), MB_YESNOCANCEL | MB_ICONINFORMATION | MB_TOPMOST);

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
