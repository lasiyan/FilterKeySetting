// clang-format off
#include "pch.h"
#include "UserLanguage.hpp"
#include "UserOption.hpp"
// clang-format on

namespace Lang {

namespace {

constexpr AppLanguage kMinLanguage = AppLanguage::Korean;
constexpr AppLanguage kMaxLanguage = AppLanguage::Japanese;

bool IsValidLanguage(DWORD value)
{
  return value >= static_cast<DWORD>(kMinLanguage) &&
         value <= static_cast<DWORD>(kMaxLanguage);
}

}  // namespace

AppLanguage GetCurrent()
{
  const DWORD stored = GLOBAL_OPTION.getInteger(KEY_LANGUAGE, 0);
  return IsValidLanguage(stored) ? static_cast<AppLanguage>(stored) : AppLanguage::Korean;
}

LANGID ToLangId(AppLanguage lang)
{
  switch (lang)
  {
    case AppLanguage::English: return MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
    case AppLanguage::Japanese: return MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT);
    case AppLanguage::Korean:
    default: return MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT);
  }
}

LPCTSTR NativeName(AppLanguage lang)
{
  switch (lang)
  {
    case AppLanguage::English: return _T("English");
    case AppLanguage::Japanese: return _T("日本語");
    case AppLanguage::Korean:
    default: return _T("한국어");
  }
}

AppLanguage DetectSystemDefault()
{
  const LANGID ui      = ::GetUserDefaultUILanguage();
  const WORD   primary = PRIMARYLANGID(ui);

  switch (primary)
  {
    case LANG_ENGLISH: return AppLanguage::English;
    case LANG_JAPANESE: return AppLanguage::Japanese;
    case LANG_KOREAN:
    default: return AppLanguage::Korean;
  }
}

void ApplyAtStartup()
{
  const AppLanguage lang   = GetCurrent();
  const LANGID      langid = ToLangId(lang);

  ::SetThreadUILanguage(langid);
  ::SetThreadLocale(MAKELCID(langid, SORT_DEFAULT));
}

CString TFor(AppLanguage lang, UINT ids)
{
  const LANGID prev = ::GetThreadUILanguage();
  ::SetThreadUILanguage(ToLangId(lang));
  CString text;
  text.LoadString(ids);
  ::SetThreadUILanguage(prev);
  return text;
}

CString T(UINT ids)
{
  CString text;
  if (!text.LoadString(ids))
  {
#ifdef _DEBUG
    text.Format(_T("[IDS:%u]"), ids);
#endif
  }
  return text;
}

CString Tf(UINT ids, ...)
{
  CString format_str;
  if (!format_str.LoadString(ids))
  {
#ifdef _DEBUG
    CString fallback;
    fallback.Format(_T("[IDS:%u]"), ids);
    return fallback;
#else
    return CString();
#endif
  }

  CString result;
  va_list args;
  va_start(args, ids);
  result.FormatV(format_str, args);
  va_end(args);
  return result;
}

void ApplyCaption(CWnd* dlg, UINT ids)
{
  if (!dlg || !::IsWindow(dlg->GetSafeHwnd()))
    return;

  const CString text = T(ids);
  if (!text.IsEmpty())
    dlg->SetWindowText(text);
}

void ApplyControlTexts(CWnd* dlg, const ControlTextEntry* table, int count)
{
  if (!dlg || !::IsWindow(dlg->GetSafeHwnd()) || !table || count <= 0)
    return;

  for (int i = 0; i < count; ++i)
  {
    const CString text = T(table[i].ids);
    if (text.IsEmpty())
      continue;

    if (CWnd* ctrl = dlg->GetDlgItem(table[i].control_id); ctrl)
      ctrl->SetWindowText(text);
  }
}

}  // namespace Lang
