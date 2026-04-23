// clang-format off
#include "pch.h"
#include "UserKeyBinding.hpp"
#include "UserLanguage.hpp"
#include <vector>
// clang-format on

namespace KeyBinding {
namespace {
struct ActiveHotkeyEntry
{
  DWORD   hotkey = 0;
  CString desc;
};

CString ModifierToString(UINT modifiers)
{
  CString value;
  // clang-format off
  if (modifiers & MOD_CONTROL) value += _T("Ctrl+");
  if (modifiers & MOD_SHIFT)   value += _T("Shift+");
  if (modifiers & MOD_ALT)     value += _T("Alt+");
  if (modifiers & MOD_WIN)     value += _T("Win+");
  // clang-format on
  return value;
}

CString KeyToString(UINT vk)
{
  UINT scan_code = MapVirtualKey(vk, MAPVK_VK_TO_VSC);
  if (vk == VK_LEFT || vk == VK_UP || vk == VK_RIGHT || vk == VK_DOWN ||
      vk == VK_PRIOR || vk == VK_NEXT || vk == VK_END || vk == VK_HOME ||
      vk == VK_INSERT || vk == VK_DELETE || vk == VK_DIVIDE || vk == VK_NUMLOCK)
  {
    scan_code |= KF_EXTENDED;
  }

  TCHAR name[64];
  memset(name, 0, sizeof(name));

  if (GetKeyNameText(static_cast<LONG>(scan_code << 16), name, _countof(name)) > 0)
    return CString(name);

  CString unknown;
  unknown.Format(_T("VK_%u"), vk);
  return unknown;
}

void BuildActiveHotkeyEntries(int preset_count, std::vector<ActiveHotkeyEntry>* entries,
                              int ignore_preset, bool ignore_toggle)
{
  if (!entries)
    return;

  entries->clear();

  if (IsEnabled())
  {
    for (int preset = PRESET_OFF; preset < preset_count; ++preset)
    {
      if (preset == ignore_preset)
        continue;

      PresetOption option(preset);
      DWORD        hotkey_value = option.getInteger(KEY_PRESET_HOTKEY, 0);
      if (HotkeyVK(hotkey_value) == 0)
        continue;

      entries->push_back({ hotkey_value, PresetOption::TAG(preset) });
    }
  }

  if (IsToggleEnabled() && !ignore_toggle)
  {
    DWORD toggle_hotkey = GetToggleHotkey();
    if (HotkeyVK(toggle_hotkey) != 0)
      entries->push_back({ toggle_hotkey, Lang::T(IDS_LBL_PRESET_TOGGLE) });
  }
}
}  // namespace

bool IsEnabled()
{
  return (GLOBAL_OPTION.getInteger(KEY_ENABLE_KEYBIND, 0) != 0);
}

bool IsToggleEnabled()
{
  return (GLOBAL_OPTION.getInteger(KEY_ENABLE_TOGGLE_KEYBIND, 0) != 0);
}

DWORD GetToggleHotkey()
{
  return GLOBAL_OPTION.getInteger(KEY_TOGGLE_HOTKEY, 0);
}

CString HotkeyToString(DWORD hotkey_value, bool with_not_set_placeholder)
{
  const UINT vk = HotkeyVK(hotkey_value);
  if (vk == 0)
    return with_not_set_placeholder ? Lang::T(IDS_LBL_HOTKEY_NOT_SET) : CString();

  CString result = ModifierToString(HotkeyModifiers(hotkey_value));
  result += KeyToString(vk);
  return result;
}

bool IsModifierVirtualKey(UINT vk)
{
  return (vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU ||
          vk == VK_LSHIFT || vk == VK_RSHIFT || vk == VK_LCONTROL ||
          vk == VK_RCONTROL || vk == VK_LMENU || vk == VK_RMENU ||
          vk == VK_LWIN || vk == VK_RWIN);
}

UINT CurrentModifiers()
{
  UINT modifiers = 0;

  if (GetKeyState(VK_CONTROL) & 0x8000)
    modifiers |= MOD_CONTROL;

  if (GetKeyState(VK_SHIFT) & 0x8000)
    modifiers |= MOD_SHIFT;

  if (GetKeyState(VK_MENU) & 0x8000)
    modifiers |= MOD_ALT;

  if (GetKeyState(VK_LWIN) & 0x8000 || GetKeyState(VK_RWIN) & 0x8000)
    modifiers |= MOD_WIN;

  return modifiers;
}

bool FindDuplicatePresetHotkey(int target_preset, int preset_count,
                               DWORD hotkey_value, int* duplicate_preset)
{
  if (duplicate_preset)
    *duplicate_preset = PRESET_OFF;

  if (hotkey_value == 0)
    return false;

  const UINT vk        = HotkeyVK(hotkey_value);
  const UINT modifiers = HotkeyModifiers(hotkey_value);

  for (int preset = PRESET_OFF; preset < preset_count; ++preset)
  {
    if (preset == target_preset)
      continue;

    PresetOption option(preset);
    DWORD        other_hotkey = option.getInteger(KEY_PRESET_HOTKEY, 0);
    if (vk == HotkeyVK(other_hotkey) && modifiers == HotkeyModifiers(other_hotkey))
    {
      if (duplicate_preset)
        *duplicate_preset = preset;
      return true;
    }
  }

  return false;
}

bool FindDuplicateActiveHotkeyCandidate(int      preset_count,
                                        DWORD    hotkey_value,
                                        int      ignore_preset,
                                        bool     ignore_toggle,
                                        CString* duplicate_target_desc)
{
  if (duplicate_target_desc)
    duplicate_target_desc->Empty();

  if (HotkeyVK(hotkey_value) == 0)
    return false;

  std::vector<ActiveHotkeyEntry> entries;
  BuildActiveHotkeyEntries(preset_count, &entries, ignore_preset, ignore_toggle);

  const UINT vk        = HotkeyVK(hotkey_value);
  const UINT modifiers = HotkeyModifiers(hotkey_value);

  for (const auto& entry : entries)
  {
    if (vk == HotkeyVK(entry.hotkey) && modifiers == HotkeyModifiers(entry.hotkey))
    {
      if (duplicate_target_desc)
        *duplicate_target_desc = entry.desc;
      return true;
    }
  }

  return false;
}

bool ValidateActiveHotkeys(int preset_count, CString* conflict_message)
{
  if (conflict_message)
    conflict_message->Empty();

  std::vector<ActiveHotkeyEntry> entries;
  BuildActiveHotkeyEntries(preset_count, &entries, -1, false);

  for (size_t i = 0; i < entries.size(); ++i)
  {
    const UINT left_vk        = HotkeyVK(entries[i].hotkey);
    const UINT left_modifiers = HotkeyModifiers(entries[i].hotkey);

    for (size_t j = i + 1; j < entries.size(); ++j)
    {
      if (left_vk == HotkeyVK(entries[j].hotkey) &&
          left_modifiers == HotkeyModifiers(entries[j].hotkey))
      {
        if (conflict_message)
        {
          conflict_message->Format(
              Lang::T(IDS_FMT_HOTKEY_DUPLICATE),
              (LPCTSTR)entries[i].desc,
              (LPCTSTR)entries[j].desc,
              (LPCTSTR)HotkeyToString(entries[i].hotkey, true));
        }
        return false;
      }
    }
  }

  return true;
}

bool IsActiveHotkeyAssigned(UINT vk, UINT modifiers, int preset_count)
{
  const DWORD hotkey_value = ComposeHotkey(vk, modifiers);

  std::vector<ActiveHotkeyEntry> entries;
  BuildActiveHotkeyEntries(preset_count, &entries, -1, false);

  for (const auto& entry : entries)
  {
    if (HotkeyVK(entry.hotkey) == vk && HotkeyModifiers(entry.hotkey) == modifiers)
      return true;
  }

  return false;
}

int ResolveToggleTargetPreset(int current_preset, int last_on_preset, int preset_count)
{
  if (PRESET_IS_ON(current_preset))
    return PRESET_OFF;

  int fallback = last_on_preset;
  if (!PRESET_IS_ON(fallback) || fallback >= preset_count)
    fallback = 1;

  if (fallback >= preset_count)
    fallback = PRESET_OFF;

  return fallback;
}

void RegisterPresetHotkeys(HWND hWnd, int preset_count,
                           bool registered_flags[PRESET_MAX_COUNT],
                           int  hotkey_id_base)
{
  UnregisterPresetHotkeys(hWnd, preset_count, registered_flags, hotkey_id_base);

  if (IsEnabled() == false)
    return;

  for (int preset = PRESET_OFF; preset < preset_count; ++preset)
  {
    PresetOption option(preset);
    DWORD        hotkey_value = option.getInteger(KEY_PRESET_HOTKEY, 0);

    const UINT vk        = HotkeyVK(hotkey_value);
    const UINT modifiers = HotkeyModifiers(hotkey_value);
    if (vk == 0)
      continue;

    if (RegisterHotKey(hWnd, hotkey_id_base + preset, modifiers, vk))
    {
      registered_flags[preset] = true;
    }
    else
    {
      registered_flags[preset] = false;
      TRACE(_T("RegisterHotKey failed. preset=%d, modifiers=%u, vk=%u, error=%lu\n"),
            preset, modifiers, vk, GetLastError());
    }
  }
}

void UnregisterPresetHotkeys(HWND hWnd, int preset_count,
                             bool registered_flags[PRESET_MAX_COUNT],
                             int  hotkey_id_base)
{
  for (int preset = PRESET_OFF; preset < preset_count; ++preset)
  {
    if (registered_flags[preset])
    {
      UnregisterHotKey(hWnd, hotkey_id_base + preset);
      registered_flags[preset] = false;
    }
  }
}

void RegisterConfiguredHotkeys(HWND hWnd, int preset_count,
                               bool  registered_flags[PRESET_MAX_COUNT],
                               bool* toggle_registered,
                               int   hotkey_id_base,
                               int   toggle_hotkey_id)
{
  UnregisterConfiguredHotkeys(hWnd, preset_count, registered_flags,
                              toggle_registered, hotkey_id_base, toggle_hotkey_id);

  RegisterPresetHotkeys(hWnd, preset_count, registered_flags, hotkey_id_base);

  if (!toggle_registered)
    return;

  if (!IsToggleEnabled())
    return;

  const DWORD toggle_hotkey = GetToggleHotkey();
  const UINT  vk            = HotkeyVK(toggle_hotkey);
  const UINT  modifiers     = HotkeyModifiers(toggle_hotkey);
  if (vk == 0)
    return;

  if (RegisterHotKey(hWnd, toggle_hotkey_id, modifiers, vk))
  {
    *toggle_registered = true;
  }
  else
  {
    *toggle_registered = false;
    TRACE(_T("RegisterHotKey failed. toggle, modifiers=%u, vk=%u, error=%lu\n"),
          modifiers, vk, GetLastError());
  }
}

void UnregisterConfiguredHotkeys(HWND hWnd, int preset_count,
                                 bool  registered_flags[PRESET_MAX_COUNT],
                                 bool* toggle_registered,
                                 int   hotkey_id_base,
                                 int   toggle_hotkey_id)
{
  UnregisterPresetHotkeys(hWnd, preset_count, registered_flags, hotkey_id_base);

  if (toggle_registered && *toggle_registered)
  {
    UnregisterHotKey(hWnd, toggle_hotkey_id);
    *toggle_registered = false;
  }
}

HotkeyValidation ValidateHotkeyCandidate(int preset_count, DWORD hotkey_value,
                                         int exclude_preset, bool is_toggle)
{
  HotkeyValidation result = { true, {} };

  if (HotkeyVK(hotkey_value) == 0)
    return result;

  const DWORD reserved_debug_hotkey = ComposeHotkey(VK_F12, MOD_CONTROL | MOD_ALT);
  if (hotkey_value == reserved_debug_hotkey)
  {
    result.ok            = false;
    result.error_message = Lang::T(IDS_MSG_HOTKEY_RESERVED);
    return result;
  }

  CString duplicate_desc;
  if (FindDuplicateActiveHotkeyCandidate(preset_count, hotkey_value,
                                         exclude_preset, is_toggle,
                                         &duplicate_desc))
  {
    result.ok = false;
    result.error_message.Format(Lang::T(IDS_FMT_HOTKEY_ALREADY_USED),
                                (LPCTSTR)duplicate_desc);
    return result;
  }

  return result;
}

}  // namespace KeyBinding
