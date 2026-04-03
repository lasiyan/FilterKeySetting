#pragma once

#include <tchar.h>
#include <winreg.h>

// Registry
#define REG_TARGET         (HKEY_CURRENT_USER)
#define USER_REGISTRY_PATH (_T("Software\\FilterKeyHelper"))

// Registry keys - global options
#define KEY_RESTORE_SETTING               (_T("restore_setting"))
#define KEY_DISABLE_HOTKEY                (_T("disable_hotkey"))
#define KEY_MOVE_TO_TRAY                  (_T("move_to_tray"))
#define KEY_ENABLE_KEYBIND                (_T("enable_keybind"))
#define KEY_ENABLE_TOGGLE_KEYBIND         (_T("enable_toggle"))
#define KEY_TOGGLE_HOTKEY                 (_T("toggle_hotkey"))
#define KEY_ENABLE_MOUSE_MOVE_TRACKER     (_T("enable_mouse_move_tracker"))
#define KEY_MOUSE_MOVE_SENSITIVITY        (_T("mouse_move_sensitivity"))
#define KEY_ENABLE_MOUSE_DBLCLICK_TRACKER (_T("enable_mouse_dblclick_tracker"))
#define KEY_DISABLE_WITH_ESC              (_T("disable_with_esc"))
#define KEY_PROCESS_OFF_ENABLED           (_T("process_off_enabled"))
#define KEY_PROCESS_OFF_NAME              (_T("process_off_name"))
#define KEY_PROCESS_OFF_RESTORE           (_T("process_off_restore"))
#define KEY_IF_FULL_SCREEN_GAME           (_T("if_full_screen_game"))
#define KEY_MUTE_SOUND                    (_T("mute_sound"))
#define KEY_SYNC_FILTERKEY                (_T("sync_filterkey"))
#define KEY_ENABLE_PRESET_OSD             (_T("enable_preset_osd"))
#define KEY_PRESET_OSD_CORNER             (_T("preset_osd_corner"))
#define KEY_PRESET_OSD_ALPHA              (_T("preset_osd_alpha"))
#define KEY_PRESET_OSD_KEEP_VISIBLE       (_T("preset_osd_keep_visible"))
#define KEY_PRESET_OSD_SIZE               (_T("preset_osd_size"))
#define KEY_PRESET_COUNT                  (_T("preset_count"))
#define KEY_LAST_PRESET                   (_T("last_preset_number"))
#define KEY_OFF_USE_WINDOWS_DEFAULT       (_T("off_use_windows_default"))
#define KEY_FILTERKEY_BACKUP_VALID        (_T("filterkey_backup_valid"))
#define KEY_FILTERKEY_BACKUP_WAIT         (_T("filterkey_backup_wait"))
#define KEY_FILTERKEY_BACKUP_DELAY        (_T("filterkey_backup_delay"))
#define KEY_FILTERKEY_BACKUP_REPEAT       (_T("filterkey_backup_repeat"))
#define KEY_FILTERKEY_BACKUP_FLAGS        (_T("filterkey_backup_flags"))

// Registry keys - preset options
#define KEY_PRESET_TITLE  (_T("preset_title"))
#define KEY_PRESET_HOTKEY (_T("preset_hotkey"))
#define KEY_ACCEPT_DELAY  (_T("accept_delay"))
#define KEY_REPEAT_DELAY  (_T("repeat_delay"))
#define KEY_REPEAT_RATE   (_T("repeat_rate"))
#define KEY_FILTER_FLAG   (_T("filter_flags"))

// FilterKey defaults
#define DEFAULT_ACCEPT_DELAY (1000)
#define DEFAULT_REPEAT_DELAY (1000)
#define DEFAULT_REPEAT_RATE  (500)
#define WINDOW_FILTER_FLAG   (126)
#define CUSTOM_FILTER_FLAG   (35)

// Preset value validation limits
static constexpr DWORD MAX_ACCEPT_DELAY = 10000;
static constexpr DWORD MAX_REPEAT_DELAY = 10000;
static constexpr DWORD MAX_REPEAT_RATE  = 10000;

// Preset constants
static constexpr int PRESET_OFF       = 0;
static constexpr int PRESET_MIN_COUNT = 2;
static constexpr int PRESET_MAX_COUNT = 10;

static constexpr bool PRESET_IS_OFF(const int preset)
{
  return (preset == PRESET_OFF);
}

static constexpr bool PRESET_IS_ON(const int preset)
{
  return (preset > PRESET_OFF && preset < PRESET_MAX_COUNT);
}

static constexpr bool PRESET_IS_VALID(const int preset)
{
  return (PRESET_IS_OFF(preset) || PRESET_IS_ON(preset));
}

// Inter-dialog message IDs (shared between FilterKeySettingDlg and DialogDebug)
static constexpr UINT WM_MOUSE_TRACKER_TRIGGERED   = WM_APP + 2;
static constexpr UINT WM_DEV_DEBUG_CLOSED          = WM_APP + 60;
static constexpr UINT WM_DEV_DEBUG_OPTIONS_CHANGED = WM_APP + 61;
