// clang-format off
#include "pch.h"
#include "UserPresetService.hpp"
#include "UserFilterKey.hpp"
#include "UserLanguage.hpp"
#include "UserOption.hpp"
// clang-format on

namespace PresetService {

bool ActivatePreset(int preset, int* out_last_on)
{
  const int target_preset = PRESET_IS_VALID(preset) ? preset : PRESET_OFF;

  if (target_preset == static_cast<int>(GLOBAL_OPTION.getInteger(KEY_LAST_PRESET)))
    return true;

  bool ok = FilterKey::ActivatePreset(target_preset);

  if (ok && PRESET_IS_ON(target_preset) && out_last_on)
    *out_last_on = target_preset;

  return ok;
}

PresetValues GetPresetValues(int preset)
{
  PresetOption option(preset);
  PresetValues values = {};
  values.accept_delay = option.getInteger(KEY_ACCEPT_DELAY, DEFAULT_ACCEPT_DELAY);
  values.repeat_delay = option.getInteger(KEY_REPEAT_DELAY, DEFAULT_REPEAT_DELAY);
  values.repeat_rate  = option.getInteger(KEY_REPEAT_RATE, DEFAULT_REPEAT_RATE);
  return values;
}

PresetValidation ValidateValues(const PresetValues& values)
{
  PresetValidation result = { true, {}, {} };

  if (values.accept_delay > MAX_ACCEPT_DELAY)
  {
    result.ok         = false;
    result.field_name = _T("Accept Delay");
    result.error_message.Format(Lang::T(IDS_FMT_VALUE_RANGE),
                                _T("Accept Delay"), 0UL, static_cast<unsigned long>(MAX_ACCEPT_DELAY));
    return result;
  }

  if (values.repeat_delay > MAX_REPEAT_DELAY)
  {
    result.ok         = false;
    result.field_name = _T("Repeat Delay");
    result.error_message.Format(Lang::T(IDS_FMT_VALUE_RANGE),
                                _T("Repeat Delay"), 0UL, static_cast<unsigned long>(MAX_REPEAT_DELAY));
    return result;
  }

  if (values.repeat_rate > MAX_REPEAT_RATE)
  {
    result.ok         = false;
    result.field_name = _T("Repeat Rate");
    result.error_message.Format(Lang::T(IDS_FMT_VALUE_RANGE),
                                _T("Repeat Rate"), 0UL, static_cast<unsigned long>(MAX_REPEAT_RATE));
    return result;
  }

  return result;
}

bool SavePresetValues(int preset, const PresetValues& values, bool* changed)
{
  if (!PRESET_IS_ON(preset))
    return true;

  PresetOption option(preset);
  bool         has_changed = false;

  if (option.getInteger(KEY_ACCEPT_DELAY, values.accept_delay) != values.accept_delay)
    has_changed = true;
  if (option.getInteger(KEY_REPEAT_DELAY, values.repeat_delay) != values.repeat_delay)
    has_changed = true;
  if (option.getInteger(KEY_REPEAT_RATE, values.repeat_rate) != values.repeat_rate)
    has_changed = true;

  option.set(KEY_ACCEPT_DELAY, values.accept_delay);
  option.set(KEY_REPEAT_DELAY, values.repeat_delay);
  option.set(KEY_REPEAT_RATE, values.repeat_rate);

  if (changed)
    *changed = has_changed;

  return true;
}

int ResolvePresetCount()
{
  int count = static_cast<int>(GLOBAL_OPTION.getInteger(KEY_PRESET_COUNT));
  if (count < PRESET_MIN_COUNT)
    count = PRESET_MIN_COUNT;
  if (count > PRESET_MAX_COUNT)
    count = PRESET_MAX_COUNT;

  GLOBAL_OPTION.set(KEY_PRESET_COUNT, static_cast<DWORD>(count));
  return count;
}

}  // namespace PresetService
