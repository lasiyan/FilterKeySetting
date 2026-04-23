
// FilterKeySetting.cpp : Defines the class behaviors for the application.
//

// clang-format off
#include "pch.h"
#include "framework.h"
#include "FilterKeySetting.h"
#include "FilterKeySettingDlg.h"
#include "UserFilterKey.hpp"
#include "UserLanguage.hpp"
#include <tlhelp32.h>
// clang-format on

#ifdef _DEBUG
  #define new DEBUG_NEW
#endif

// CFilterKeySettingApp

BEGIN_MESSAGE_MAP(CFilterKeySettingApp, CWinApp)
ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

// CFilterKeySettingApp construction

CFilterKeySettingApp::CFilterKeySettingApp()
{
  // support Restart Manager
  m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

  // TODO: add construction code here,
  // Place all significant initialization in InitInstance
}

// The one and only CFilterKeySettingApp object

CFilterKeySettingApp theApp;

// CFilterKeySettingApp initialization

struct ExistingWindowSearchContext
{
  DWORD pid             = 0;
  HWND  dialog_window   = nullptr;
  HWND  fallback_window = nullptr;
};

static BOOL CALLBACK EnumWindowsFindByPID(HWND hwnd, LPARAM lParam)
{
  auto* ctx     = reinterpret_cast<ExistingWindowSearchContext*>(lParam);
  DWORD wnd_pid = 0;
  ::GetWindowThreadProcessId(hwnd, &wnd_pid);

  if (wnd_pid != ctx->pid || ::GetWindow(hwnd, GW_OWNER) != nullptr)
    return TRUE;

  if (ctx->fallback_window == nullptr)
    ctx->fallback_window = hwnd;

  TCHAR class_name[64] = {};
  ::GetClassName(hwnd, class_name, _countof(class_name));
  if (_tcscmp(class_name, _T("#32770")) == 0)
  {
    // Main dialog window in this app is class #32770.
    ctx->dialog_window = hwnd;
    return FALSE;
  }

  return TRUE;
}

static BOOL ActivateExistingInstance()
{
  TCHAR current_path[MAX_PATH] = {};
  if (::GetModuleFileName(nullptr, current_path, MAX_PATH) == 0)
    return FALSE;

  CString current_name(current_path);
  int     pos = current_name.ReverseFind(_T('\\'));
  if (pos >= 0)
    current_name = current_name.Mid(pos + 1);

  const DWORD current_pid = ::GetCurrentProcessId();

  HANDLE snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snapshot == INVALID_HANDLE_VALUE)
    return FALSE;

  PROCESSENTRY32 entry = {};
  entry.dwSize         = sizeof(entry);
  DWORD found_pid      = 0;

  if (::Process32First(snapshot, &entry))
  {
    do
    {
      if (entry.th32ProcessID != current_pid &&
          current_name.CompareNoCase(entry.szExeFile) == 0)
      {
        found_pid = entry.th32ProcessID;
        break;
      }
    } while (::Process32Next(snapshot, &entry));
  }
  ::CloseHandle(snapshot);

  if (found_pid == 0)
    return FALSE;

  // Let the already-running instance take foreground focus.
  ::AllowSetForegroundWindow(found_pid);

  ExistingWindowSearchContext search = {};
  search.pid                         = found_pid;
  ::EnumWindows(EnumWindowsFindByPID, reinterpret_cast<LPARAM>(&search));

  const HWND target_window = search.dialog_window ? search.dialog_window : search.fallback_window;

  if (target_window)
  {
    DWORD_PTR  message_result = 0;
    const auto send_ok        = ::SendMessageTimeout(
        target_window,
        CFilterKeySettingDlg::WM_ACTIVATE_EXISTING_INST,
        0, 0,
        SMTO_ABORTIFHUNG | SMTO_NORMAL,
        1000,
        &message_result);

    // Fallback path for cases where custom app messages are blocked
    // (e.g. integrity level mismatch) or target window is hidden.
    if (send_ok == 0)
    {
      ::ShowWindowAsync(target_window, SW_SHOW);
      ::ShowWindowAsync(target_window, SW_RESTORE);
      ::BringWindowToTop(target_window);
      ::SetForegroundWindow(target_window);
    }
  }

  return TRUE;
}

BOOL CFilterKeySettingApp::InitInstance()
{
  const CString cmd_line               = ::GetCommandLine();
  const bool    bypass_single_instance = (cmd_line.Find(_T("/restart-bypass-single-instance")) >= 0);

  if (!bypass_single_instance && ActivateExistingInstance())
    return FALSE;

  // InitCommonControlsEx() is required on Windows XP if an application
  // manifest specifies use of ComCtl32.dll version 6 or later to enable
  // visual styles.  Otherwise, any window creation will fail.
  INITCOMMONCONTROLSEX InitCtrls;
  InitCtrls.dwSize = sizeof(InitCtrls);
  // Set this to include all the common control classes you want to use
  // in your application.
  InitCtrls.dwICC = ICC_WIN95_CLASSES;
  InitCommonControlsEx(&InitCtrls);

  CWinApp::InitInstance();

  AfxEnableControlContainer();

  // Create the shell manager, in case the dialog contains
  // any shell tree view or shell list view controls.
  CShellManager* pShellManager = new CShellManager;

  // Activate "Windows Native" visual manager for enabling themes in MFC
  // controls
  CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

  // Standard initialization
  // If you are not using these features and wish to reduce the size
  // of your final executable, you should remove from the following
  // the specific initialization routines you do not need
  // Change the registry key under which our settings are stored
  // TODO: You should modify this string to be something appropriate
  // such as the name of your company or organization
  if (InitializeOptionValues() == false)
  {
    AfxMessageBox(Lang::T(IDS_MSG_REGISTRY_CREATE_FAIL));
    return FALSE;
  }

  Lang::ApplyAtStartup();

  CFilterKeySettingDlg dlg;
  m_pMainWnd        = &dlg;
  INT_PTR nResponse = dlg.DoModal();
  if (nResponse == IDOK)
  {
    // TODO: Place code here to handle when the dialog is
    //  dismissed with OK
  }
  else if (nResponse == IDCANCEL)
  {
    // TODO: Place code here to handle when the dialog is
    //  dismissed with Cancel
  }
  else if (nResponse == -1)
  {
    TRACE(traceAppMsg, 0,
          "Warning: dialog creation failed, so application is terminating "
          "unexpectedly.\n");
    TRACE(traceAppMsg, 0,
          "Warning: if you are using MFC controls on the dialog, you cannot "
          "#define _AFX_NO_MFC_CONTROLS_IN_DIALOGS.\n");
  }

  // Delete the shell manager created above.
  if (pShellManager != nullptr)
  {
    delete pShellManager;
  }

#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
  ControlBarCleanUp();
#endif

  // Since the dialog has been closed, return FALSE so that we exit the
  //  application, rather than start the application's message pump.
  return FALSE;
}

int CFilterKeySettingApp::ExitInstance()
{
  return CWinApp::ExitInstance();
}
