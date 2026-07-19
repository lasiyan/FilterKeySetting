// clang-format off
#include "pch.h"
#include "UserDefine.hpp"
#include "UserLanguage.hpp"
#include "UserOption.hpp"
#include <memory>
// clang-format on

/*********************************************************************************
 * Initializer
 ********************************************************************************/
bool InitializeOptionValues()
{
  bool res = true;

  // Global option
  {
    res &= GLOBAL_OPTION.setInit(KEY_RESTORE_SETTING, true);
    res &= GLOBAL_OPTION.setInit(KEY_MOVE_TO_TRAY, false);
    res &= GLOBAL_OPTION.setInit(KEY_DISABLE_HOTKEY, false);
    res &= GLOBAL_OPTION.setInit(KEY_ENABLE_KEYBIND, false);
    res &= GLOBAL_OPTION.setInit(KEY_ENABLE_TOGGLE_KEYBIND, false);
    res &= GLOBAL_OPTION.setInit(KEY_TOGGLE_HOTKEY, 0);
    res &= GLOBAL_OPTION.setInit(KEY_ENABLE_MOUSE_DBLCLICK_TRACKER, false);
    res &= GLOBAL_OPTION.setInit(KEY_ENABLE_MOUSE_MOVE_TRACKER, false);
    res &= GLOBAL_OPTION.setInit(KEY_DISABLE_WITH_ESC, false);
    res &= GLOBAL_OPTION.setInit(KEY_DISABLE_WITH_ENTER, false);
    res &= GLOBAL_OPTION.setInit(KEY_PROCESS_OFF_ENABLED, false);
    res &= GLOBAL_OPTION.setInit(KEY_PROCESS_OFF_NAME, CString());
    res &= GLOBAL_OPTION.setInit(KEY_PROCESS_OFF_RESTORE, true);
    res &= GLOBAL_OPTION.setInit(KEY_IF_FULL_SCREEN_GAME, false);
    res &= GLOBAL_OPTION.setInit(KEY_MUTE_SOUND, false);
    res &= GLOBAL_OPTION.setInit(KEY_SYNC_FILTERKEY, false);
    res &= GLOBAL_OPTION.setInit(KEY_ENABLE_PRESET_OSD, false);
    res &= GLOBAL_OPTION.setInit(KEY_PRESET_OSD_CORNER, 3);
    res &= GLOBAL_OPTION.setInit(KEY_PRESET_OSD_ALPHA, 220);
    res &= GLOBAL_OPTION.setInit(KEY_PRESET_OSD_KEEP_VISIBLE, false);
    res &= GLOBAL_OPTION.setInit(KEY_PRESET_OSD_SIZE, 5);
    res &= GLOBAL_OPTION.setInit(KEY_PRESET_COUNT, 3);
    res &= GLOBAL_OPTION.setInit(KEY_LAST_PRESET, false);
    res &= GLOBAL_OPTION.setInit(KEY_OFF_USE_WINDOWS_DEFAULT, true);
    res &= GLOBAL_OPTION.setInit(KEY_FILTERKEY_BACKUP_VALID, false);
    res &= GLOBAL_OPTION.setInit(KEY_FILTERKEY_BACKUP_WAIT, static_cast<DWORD>(DEFAULT_ACCEPT_DELAY));
    res &= GLOBAL_OPTION.setInit(KEY_FILTERKEY_BACKUP_DELAY, static_cast<DWORD>(DEFAULT_REPEAT_DELAY));
    res &= GLOBAL_OPTION.setInit(KEY_FILTERKEY_BACKUP_REPEAT, static_cast<DWORD>(DEFAULT_REPEAT_RATE));
    res &= GLOBAL_OPTION.setInit(KEY_FILTERKEY_BACKUP_FLAGS, static_cast<DWORD>(WINDOW_FILTER_FLAG));
    res &= GLOBAL_OPTION.setInit(KEY_LANGUAGE, static_cast<DWORD>(Lang::DetectSystemDefault()));
    res &= GLOBAL_OPTION.setInit(KEY_AUTOSTART_MODE, AUTOSTART_MODE_NONE);
    res &= GLOBAL_OPTION.setInit(KEY_AUTOSTART_TRAY, false);
  }

  // Preset Generator
  auto GetPreset = [&](int number) -> std::unique_ptr<PresetOption> {
    if (PRESET_IS_VALID(number) == false)
      return nullptr;
    return std::make_unique<PresetOption>(number);
  };

  const auto InitPreset = [&](int preset_number, const CString& title,
                              DWORD repeat_delay, DWORD repeat_rate,
                              DWORD filter_flag) {
    if (auto preset = GetPreset(preset_number); preset)
    {
      res &= preset->setInit(KEY_PRESET_TITLE, title);
      res &= preset->setInit(KEY_PRESET_HOTKEY, 0);
      res &= preset->setInit(KEY_ACCEPT_DELAY, 0);
      res &= preset->setInit(KEY_REPEAT_DELAY, repeat_delay);
      res &= preset->setInit(KEY_REPEAT_RATE, repeat_rate);
      res &= preset->setInit(KEY_FILTER_FLAG, filter_flag);
    }
  };

  InitPreset(PRESET_OFF, Lang::T(IDS_PRESET_NAME_OFF), 500, 33, WINDOW_FILTER_FLAG);    // Rename Preset OFF
  InitPreset(PRESET_OFF + 1, Lang::T(IDS_PRESET_NAME_ON), 90, 14, CUSTOM_FILTER_FLAG);  // Rename Default Preset
  for (int preset_number = 2; preset_number < PRESET_MAX_COUNT; ++preset_number)
  {
    CString title;
    title.Format(Lang::T(IDS_FMT_PRESET_DEFAULT_NAME), preset_number);
    InitPreset(preset_number, title, 500, 33, CUSTOM_FILTER_FLAG);
  }

  return res;
}

/*********************************************************************************
 * GlobalOption
 ********************************************************************************/
GlobalOption::GlobalOption() : OptionBase(USER_REGISTRY_PATH)
{
  //
}

/*********************************************************************************
 * PresetOption
 ********************************************************************************/
CString PresetOption::SECTION(int number)
{
  // Clamp number to valid range
  CString section;
  section.Format(_T("%s\\Preset%d"), USER_REGISTRY_PATH, number);
  return section;
}

CString PresetOption::TAG(const int number)
{
  PresetOption option(number);
  return option.getString(KEY_PRESET_TITLE);
}

PresetOption::PresetOption(const int number) : OptionBase(PresetOption::SECTION(number))
{
  //
}

/*********************************************************************************
 * Option Base Implementation
 ********************************************************************************/
OptionBase::OptionBase(const CString& section) : section_(section)
{
  RegCreateKeyEx(REG_TARGET, section_, 0, NULL, REG_OPTION_NON_VOLATILE,
                 KEY_ALL_ACCESS, NULL, &registry_, &dwDisposition_);
}

OptionBase::~OptionBase()
{
  if (registry_ != NULL)
  {
    RegCloseKey(registry_);
  }
}

template <typename T>
bool OptionBase::set(const CString& key, const T& value)
{
  using U = std::decay_t<T>;

  auto SetValue = [&](DWORD type, const BYTE* data, DWORD cbData) -> bool {
    return (RegSetValueEx(registry_, (LPCTSTR)key, 0, type, data, cbData) == ERROR_SUCCESS);
  };

  if constexpr (std::is_same_v<U, CString>)
  {
    return SetValue(REG_SZ, reinterpret_cast<const BYTE*>((LPCTSTR)value), static_cast<DWORD>((value.GetLength() + 1) * sizeof(TCHAR)));
  }
  else if constexpr (std::is_same_v<U, bool> ||
                     std::is_same_v<U, int> ||
                     std::is_same_v<U, DWORD>)
  {
    DWORD d = 0;
    if constexpr (std::is_same_v<U, bool>)
      d = value ? 1u : 0u;
    else
      d = static_cast<DWORD>(value);

    return SetValue(REG_DWORD, reinterpret_cast<const BYTE*>(&d), static_cast<DWORD>(sizeof(d)));
  }

  return false;
}
template bool OptionBase::set<CString>(const CString& key, const CString& value);
template bool OptionBase::set<int>(const CString& key, const int& value);
template bool OptionBase::set<DWORD>(const CString& key, const DWORD& value);
template bool OptionBase::set<bool>(const CString& key, const bool& value);

template <typename T>
bool OptionBase::setInit(const CString& key, const T& value)
{
  using U = std::decay_t<T>;

  auto ExistsValue = [&](DWORD expectedType) -> bool {
    DWORD type = 0;
    LONG  r    = RegQueryValueEx(registry_, (LPCTSTR)key, nullptr, &type, nullptr, nullptr);
    if (r != ERROR_SUCCESS)
      return false;
    return (type == expectedType);
  };

  if constexpr (std::is_same_v<U, CString>)
  {
    if (ExistsValue(REG_SZ))
      return true;
    return set<CString>(key, value);
  }
  else if constexpr (std::is_same_v<U, bool> ||
                     std::is_same_v<U, int> ||
                     std::is_same_v<U, DWORD>)
  {
    if (ExistsValue(REG_DWORD))
      return true;
    return set<U>(key, value);
  }

  return false;
}

DWORD OptionBase::getInteger(const CString& key, DWORD default_value)
{
  DWORD type  = 0;
  DWORD value = default_value;
  DWORD size  = sizeof(value);

  LONG result = RegQueryValueEx(registry_, key, NULL, &type, (LPBYTE)&value, &size);
  if (result != ERROR_SUCCESS || type != REG_DWORD)
    return default_value;

  return value;
}

CString OptionBase::getString(const CString& key, const CString& default_value)
{
  DWORD data_size = 0;
  DWORD type      = 0;

  if (RegQueryValueEx(registry_, key, NULL, &type, NULL, &data_size) !=
          ERROR_SUCCESS ||
      (type != REG_SZ && type != REG_EXPAND_SZ) || data_size == 0)
  {
    return default_value;
  }

  const DWORD char_count = (data_size / sizeof(TCHAR)) + 1;
  auto        data       = std::make_unique<TCHAR[]>(char_count);
  ZeroMemory(data.get(), char_count * sizeof(TCHAR));

  if (RegQueryValueEx(registry_, key, NULL, NULL, (LPBYTE)data.get(),
                      &data_size) != ERROR_SUCCESS)
  {
    return default_value;
  }

  return CString(data.get());
}
