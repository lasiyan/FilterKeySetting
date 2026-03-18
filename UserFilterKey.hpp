#pragma once

#include <shellapi.h>

#include "UserOption.hpp"

class FilterKey
{
 public:
  // Backup
  static bool BackupCurrentFilterKeysToOption()
  {
    FILTERKEYS fk = { sizeof(FILTERKEYS) };

    if (!SystemParametersInfo(SPI_GETFILTERKEYS, sizeof(FILTERKEYS), &fk, 0))
      return false;

    GLOBAL_OPTION.set(KEY_FILTERKEY_BACKUP_WAIT, static_cast<DWORD>(fk.iWaitMSec));
    GLOBAL_OPTION.set(KEY_FILTERKEY_BACKUP_DELAY, static_cast<DWORD>(fk.iDelayMSec));
    GLOBAL_OPTION.set(KEY_FILTERKEY_BACKUP_REPEAT, static_cast<DWORD>(fk.iRepeatMSec));
    GLOBAL_OPTION.set(KEY_FILTERKEY_BACKUP_FLAGS, static_cast<DWORD>(fk.dwFlags));
    GLOBAL_OPTION.set(KEY_FILTERKEY_BACKUP_VALID, static_cast<DWORD>(true));
    return true;
  }

  static bool EnsureFilterKeysBackup()
  {
    if (GLOBAL_OPTION.getInteger(KEY_FILTERKEY_BACKUP_VALID, 0) != 0)
      return true;

    return BackupCurrentFilterKeysToOption();
  }

  // Apply
  static bool ActivatePreset(const int preset)
  {
    FILTERKEYS fk = { sizeof(FILTERKEYS) };

    const bool off_uses_windows_default =
        (GLOBAL_OPTION.getInteger(KEY_OFF_USE_WINDOWS_DEFAULT, 1) != 0);

    if (PRESET_IS_OFF(preset) && !off_uses_windows_default)
    {
      if (!EnsureFilterKeysBackup())
        return false;

      fk.iWaitMSec   = static_cast<UINT>(GLOBAL_OPTION.getInteger(KEY_FILTERKEY_BACKUP_WAIT, DEFAULT_ACCEPT_DELAY));
      fk.iDelayMSec  = static_cast<UINT>(GLOBAL_OPTION.getInteger(KEY_FILTERKEY_BACKUP_DELAY, DEFAULT_REPEAT_DELAY));
      fk.iRepeatMSec = static_cast<UINT>(GLOBAL_OPTION.getInteger(KEY_FILTERKEY_BACKUP_REPEAT, DEFAULT_REPEAT_RATE));
      fk.dwFlags     = GLOBAL_OPTION.getInteger(KEY_FILTERKEY_BACKUP_FLAGS, WINDOW_FILTER_FLAG);
    }
    else
    {
      PresetOption preset_option(preset);
      fk.iWaitMSec   = preset_option.getInteger(KEY_ACCEPT_DELAY, DEFAULT_ACCEPT_DELAY);
      fk.iDelayMSec  = preset_option.getInteger(KEY_REPEAT_DELAY, DEFAULT_REPEAT_DELAY);
      fk.iRepeatMSec = preset_option.getInteger(KEY_REPEAT_RATE, DEFAULT_REPEAT_RATE);
      fk.dwFlags     = preset_option.getInteger(KEY_FILTER_FLAG);
    }

    auto flag = SPIF_UPDATEINIFILE;

    if (GLOBAL_OPTION.getInteger(KEY_SYNC_FILTERKEY, 0) != 0)
      flag |= SPIF_SENDCHANGE;

    if (SystemParametersInfo(SPI_SETFILTERKEYS, sizeof(FILTERKEYS), &fk, flag))
    {
      GLOBAL_OPTION.set(KEY_LAST_PRESET, static_cast<DWORD>(preset));
      return true;
    }

    return false;
  }

  // Elevated check
  static bool IsProcessElevatedNow()
  {
    HANDLE token = nullptr;
    if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &token))
      return false;

    TOKEN_ELEVATION elevation = {};
    DWORD           size      = 0;
    const BOOL      ok        = ::GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size);
    ::CloseHandle(token);

    return (ok == TRUE && elevation.TokenIsElevated != 0);
  }

  // Restart current program
  static bool RestartProgram(CWnd* close_target = nullptr, bool as_admin = false)
  {
    TCHAR exe_path[MAX_PATH] = {};

    if (::GetModuleFileName(nullptr, exe_path, _countof(exe_path)) == 0)
      return false;

    const LPCTSTR verb          = as_admin ? _T("runas") : _T("open");
    const LPCTSTR parameters    = _T("/restart-bypass-single-instance");
    HINSTANCE     launch_result = ::ShellExecute(close_target ? close_target->GetSafeHwnd() : nullptr, verb, exe_path, parameters, nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(launch_result) <= 32)
      return false;

    CWnd* close_wnd = nullptr;
    if (close_target && ::IsWindow(close_target->GetSafeHwnd()))
    {
      // When called from child dialogs (e.g. debug dialog), close the app main window.
      close_wnd = close_target->GetParent();
      if (!close_wnd || !::IsWindow(close_wnd->GetSafeHwnd()))
        close_wnd = close_target;
    }
    else
    {
      close_wnd = AfxGetMainWnd();
    }

    if (close_wnd && ::IsWindow(close_wnd->GetSafeHwnd()))
      close_wnd->PostMessage(WM_CLOSE);

    return true;
  }
};
