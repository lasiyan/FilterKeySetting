#pragma once

#include <windows.h>

#include <atomic>

enum class MouseTrackerTrigger : WPARAM
{
  MoveDistance = 1,
  DoubleClick  = 2,
  RightClick   = 3,
};

class MouseTracker
{
 public:
  MouseTracker();
  ~MouseTracker();

  // Lifecycle
  bool Initialize(HWND  owner_wnd,
                  DWORD interval_ms           = 50,
                  DWORD move_threshold_px     = 2,
                  DWORD distance_target_px    = 120,
                  DWORD double_click_ms       = 350,
                  DWORD double_click_range_px = 12);
  void Release();

  // Activation
  bool Activate();
  void Deactivate();
  void SetTriggerOptions(bool enable_move, bool enable_double_click, bool enable_right_click);
  void SetDistanceTarget(DWORD distance_px) { distance_target_px_ = distance_px; }
  void RequestReset() { reset_pending_.store(true); }

 private:
  // Timer callback & tick
  static VOID CALLBACK TimerProc(PVOID lpParameter, BOOLEAN TimerOrWaitFired);
  void                 Tick();
  void                 ResetTracking();

  // Query
  bool      IsOwnerForeground() const;
  ULONGLONG NowMs() const;

 private:
  // Lifetime / infra
  HWND              owner_wnd_    = nullptr;
  HANDLE            timer_queue_  = nullptr;
  HANDLE            timer_handle_ = nullptr;
  std::atomic<bool> initialized_{ false };
  std::atomic<bool> active_{ false };

  // Config
  DWORD interval_ms_           = 50;
  DWORD move_threshold_px_     = 2;
  DWORD distance_target_px_    = 120;
  DWORD double_click_ms_       = 350;
  DWORD double_click_range_px_ = 12;
  DWORD arm_delay_ms_          = 800;

  // Tracking state
  bool              has_last_cursor_pos_       = false;
  POINT             last_cursor_pos_           = { 0, 0 };
  ULONGLONG         moved_distance_px_         = 0;
  ULONGLONG         last_move_ms_              = 0;
  bool              last_left_button_down_     = false;
  bool              last_right_button_down_    = false;
  bool              has_last_click_            = false;
  POINT             last_click_pos_            = { 0, 0 };
  ULONGLONG         last_click_ms_             = 0;
  ULONGLONG         trigger_cooldown_until_ms_ = 0;
  ULONGLONG         background_since_ms_       = 0;
  std::atomic<bool> enable_move_trigger_{ false };
  std::atomic<bool> enable_double_click_trigger_{ false };
  std::atomic<bool> enable_right_click_trigger_{ false };
  std::atomic<bool> reset_pending_{ false };
  bool              in_background_ = false;
};
