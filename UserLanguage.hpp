#pragma once

#include "UserDefine.hpp"
#include "resource.h"

enum class AppLanguage : DWORD
{
  Korean   = 0,
  English  = 1,
  Japanese = 2,
};

namespace Lang {

// Current language persisted in registry (falls back to Korean on out-of-range).
AppLanguage GetCurrent();

// Win32 LANGID for the given AppLanguage (0x0412 / 0x0409 / 0x0411).
LANGID ToLangId(AppLanguage lang);

// Native display name for combobox items (not translated).
LPCTSTR NativeName(AppLanguage lang);

// AppLanguage inferred from the OS user default UI language primary id.
AppLanguage DetectSystemDefault();

// Apply selected language to the current thread's locale and UI language.
// Must be called once at app startup, after option init and before any UI loads resources.
void ApplyAtStartup();

// Load a string from the current language's STRINGTABLE.
CString T(UINT ids);

// Load a string as if the given language were active (thread locale is temporarily switched).
CString TFor(AppLanguage lang, UINT ids);

// LoadString + CString::Format wrapper. Uses current language's STRINGTABLE for the format.
CString Tf(UINT ids, ...);

// Override dialog caption with LoadString(ids). No-op when current language is Korean.
void ApplyCaption(CWnd* dlg, UINT ids);

// Override dialog control texts with LoadString. No-op when current language is Korean.
struct ControlTextEntry
{
  UINT control_id;
  UINT ids;
};
void ApplyControlTexts(CWnd* dlg, const ControlTextEntry* table, int count);

}  // namespace Lang
