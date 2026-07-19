#pragma once

#include "UserOption.hpp"

// 자동 실행 등록 관리 (Run 키 / 작업 스케줄러)
namespace AutoStart {

constexpr LPCTSTR kTaskName     = _T("FilterKeySetting_AutoStart");
constexpr LPCTSTR kAutoStartArg = _T("/autostart");
constexpr LPCTSTR kRunKeyPath   = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run");

inline CString CurrentExePath()
{
  TCHAR path[MAX_PATH] = {};
  ::GetModuleFileName(nullptr, path, MAX_PATH);
  return CString(path);
}

/*********************************************************************************
 * Run 키 (일반 권한)
 ********************************************************************************/
inline bool IsRunKeyRegistered()
{
  HKEY hKey = nullptr;
  if (::RegOpenKeyEx(HKEY_CURRENT_USER, kRunKeyPath, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    return false;

  const LONG result = ::RegQueryValueEx(hKey, GetAppName(), nullptr, nullptr, nullptr, nullptr);
  ::RegCloseKey(hKey);
  return (result == ERROR_SUCCESS);
}

inline bool RegisterRunKey()
{
  HKEY hKey = nullptr;
  if (::RegOpenKeyEx(HKEY_CURRENT_USER, kRunKeyPath, 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS)
    return false;

  CString value;
  value.Format(_T("\"%s\" %s"), (LPCTSTR)CurrentExePath(), kAutoStartArg);

  const LONG result = ::RegSetValueEx(hKey, GetAppName(), 0, REG_SZ,
                                      reinterpret_cast<const BYTE*>((LPCTSTR)value),
                                      static_cast<DWORD>((value.GetLength() + 1) * sizeof(TCHAR)));
  ::RegCloseKey(hKey);
  return (result == ERROR_SUCCESS);
}

inline void DeleteRunKey()
{
  HKEY hKey = nullptr;
  if (::RegOpenKeyEx(HKEY_CURRENT_USER, kRunKeyPath, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
  {
    ::RegDeleteValue(hKey, GetAppName());
    ::RegCloseKey(hKey);
  }
}

/*********************************************************************************
 * 작업 스케줄러 (최고 권한, UAC 없이 자동 실행)
 * 등록/삭제는 관리자 권한 프로세스에서만 성공한다 (호출 측에서 보장/안내).
 ********************************************************************************/
inline bool RunSchtasks(const CString& arguments, DWORD* exit_code)
{
  if (exit_code)
    *exit_code = static_cast<DWORD>(-1);

  TCHAR system_dir[MAX_PATH] = {};
  if (::GetSystemDirectory(system_dir, MAX_PATH) == 0)
    return false;

  CString command;
  command.Format(_T("\"%s\\schtasks.exe\" %s"), system_dir, (LPCTSTR)arguments);

  STARTUPINFO si = { sizeof(si) };
  si.dwFlags     = STARTF_USESHOWWINDOW;
  si.wShowWindow = SW_HIDE;

  PROCESS_INFORMATION pi = {};

  // CreateProcess는 쓰기 가능한 커맨드라인 버퍼를 요구한다
  if (!::CreateProcess(nullptr, command.GetBuffer(), nullptr, nullptr, FALSE,
                       CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
  {
    command.ReleaseBuffer();
    return false;
  }
  command.ReleaseBuffer();

  ::WaitForSingleObject(pi.hProcess, 15000);

  DWORD code = static_cast<DWORD>(-1);
  ::GetExitCodeProcess(pi.hProcess, &code);
  ::CloseHandle(pi.hThread);
  ::CloseHandle(pi.hProcess);

  if (exit_code)
    *exit_code = code;
  return true;
}

inline bool IsTaskRegistered()
{
  CString args;
  args.Format(_T("/Query /TN \"%s\""), kTaskName);

  DWORD code = 0;
  return RunSchtasks(args, &code) && code == 0;
}

inline bool RegisterTask()
{
  CString args;
  args.Format(_T("/Create /F /SC ONLOGON /RL HIGHEST /TN \"%s\" /TR \"\\\"%s\\\" %s\""),
              kTaskName, (LPCTSTR)CurrentExePath(), kAutoStartArg);

  DWORD code = 0;
  return RunSchtasks(args, &code) && code == 0;
}

inline bool DeleteTask()
{
  CString args;
  args.Format(_T("/Delete /F /TN \"%s\""), kTaskName);

  DWORD code = 0;
  return RunSchtasks(args, &code) && code == 0;
}

// 앱 시작 시 등록 상태 정리 (exe 이동/관리자 선택 후 재시작 대기 상태 복구)
// - 일반 모드: Run 키를 현재 exe 경로로 재등록
// - 관리자 모드 + 현재 관리자 권한: 작업을 현재 경로로 (재)등록하고 Run 키 제거
inline void HealOnStartup(bool elevated)
{
  const DWORD mode = GLOBAL_OPTION.getInteger(KEY_AUTOSTART_MODE, AUTOSTART_MODE_NONE);

  if (mode == AUTOSTART_MODE_RUN)
  {
    RegisterRunKey();
  }
  else if (mode == AUTOSTART_MODE_TASK && elevated)
  {
    DeleteRunKey();
    RegisterTask();
  }
}

}  // namespace AutoStart
