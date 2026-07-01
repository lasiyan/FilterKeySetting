// clang-format off
#include "pch.h"
#include "UserMouseTracker.hpp"
#include "UserDefine.hpp"
#include <cstdlib>
// clang-format on

MouseTracker::MouseTracker() = default;

MouseTracker::~MouseTracker()
{
}

bool MouseTracker::Initialize(HWND  owner_wnd,
                              DWORD interval_ms,
                              DWORD move_threshold_px,
                              DWORD distance_target_px,
                              DWORD double_click_ms,
                              DWORD double_click_range_px)
{
  if (initialized_)
    return true;

  owner_wnd_             = owner_wnd;
  interval_ms_           = interval_ms;
  move_threshold_px_     = move_threshold_px;
  distance_target_px_    = distance_target_px;
  double_click_ms_       = double_click_ms;
  double_click_range_px_ = double_click_range_px;

  timer_queue_ = ::CreateTimerQueue();
  if (!timer_queue_)
    return false;

  ResetTracking();
  initialized_ = true;
  return true;
}

void MouseTracker::Release()
{
  if (!initialized_)
    return;

  Deactivate();

  if (timer_queue_)
  {
    ::DeleteTimerQueueEx(timer_queue_, INVALID_HANDLE_VALUE);
    timer_queue_ = nullptr;
  }

  owner_wnd_   = nullptr;
  initialized_ = false;
}

bool MouseTracker::Activate()
{
  if (!initialized_ || active_)
    return false;

  if (!timer_queue_)
    return false;

  BOOL ok = ::CreateTimerQueueTimer(
      &timer_handle_,
      timer_queue_,
      &MouseTracker::TimerProc,
      this,
      0,
      interval_ms_,
      WT_EXECUTEINTIMERTHREAD);  // WT_EXECUTEDEFAULT

  if (!ok)
    return false;

  active_ = true;
  return true;
}

void MouseTracker::Deactivate()
{
  if (!active_)
    return;

  if (timer_handle_)
  {
    ::DeleteTimerQueueTimer(timer_queue_, timer_handle_, INVALID_HANDLE_VALUE);
    timer_handle_ = nullptr;
  }

  active_ = false;
  ResetTracking();
}

void MouseTracker::SetTriggerOptions(bool enable_move, bool enable_double_click, bool enable_right_click)
{
  enable_move_trigger_.store(enable_move);
  enable_double_click_trigger_.store(enable_double_click);
  enable_right_click_trigger_.store(enable_right_click);
}

VOID CALLBACK MouseTracker::TimerProc(PVOID lpParameter, BOOLEAN /*TimerOrWaitFired*/)
{
  auto* self = static_cast<MouseTracker*>(lpParameter);
  if (!self)
    return;

  self->Tick();
}

void MouseTracker::Tick()
{
  if (!active_)
    return;

  if (reset_pending_.exchange(false))
  {
    ResetTracking();
    return;
  }

  const bool enable_move         = enable_move_trigger_.load();
  const bool enable_double_click = enable_double_click_trigger_.load();
  const bool enable_right_click  = enable_right_click_trigger_.load();
  if (!enable_move && !enable_double_click && !enable_right_click)
    return;

  if (IsOwnerForeground())
  {
    in_background_ = false;
    ResetTracking();
    return;
  }

  POINT pt{};
  if (!::GetCursorPos(&pt))
    return;

  const ULONGLONG now = NowMs();

  const bool left_button_down  = (::GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
  const bool right_button_down = (::GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;

  if (!in_background_)
  {
    in_background_           = true;
    background_since_ms_     = now;
    has_last_cursor_pos_     = true;
    last_cursor_pos_         = pt;
    last_left_button_down_   = left_button_down;
    last_right_button_down_  = right_button_down;
    return;
  }

  if (now - background_since_ms_ < arm_delay_ms_)
  {
    has_last_cursor_pos_    = true;
    last_cursor_pos_        = pt;
    last_left_button_down_  = left_button_down;
    last_right_button_down_ = right_button_down;
    return;
  }

  if (!has_last_cursor_pos_)
  {
    last_cursor_pos_     = pt;
    has_last_cursor_pos_ = true;
  }
  else
  {
    const int dx     = std::abs(pt.x - last_cursor_pos_.x);
    const int dy     = std::abs(pt.y - last_cursor_pos_.y);
    last_cursor_pos_ = pt;

    const bool moved = (dx >= static_cast<int>(move_threshold_px_)) ||
                       (dy >= static_cast<int>(move_threshold_px_));
    if (moved && enable_move)
    {
      if (last_move_ms_ != 0 && (now - last_move_ms_) >= 500)
        moved_distance_px_ = 0;
      last_move_ms_ = now;

      moved_distance_px_ += static_cast<ULONGLONG>(dx + dy);
      if (now >= trigger_cooldown_until_ms_ &&
          moved_distance_px_ >= distance_target_px_)
      {
        if (::IsWindow(owner_wnd_))
          ::PostMessage(owner_wnd_, WM_MOUSE_TRACKER_TRIGGERED,
                        static_cast<WPARAM>(MouseTrackerTrigger::MoveDistance), 0);

        trigger_cooldown_until_ms_ = now + 1000;
        moved_distance_px_         = 0;
        has_last_click_            = false;
      }
    }
  }

  if (enable_double_click && left_button_down && !last_left_button_down_)
  {
    if (has_last_click_ &&
        (now - last_click_ms_ <= double_click_ms_) &&
        (std::abs(pt.x - last_click_pos_.x) <=
         static_cast<int>(double_click_range_px_)) &&
        (std::abs(pt.y - last_click_pos_.y) <=
         static_cast<int>(double_click_range_px_)))
    {
      if (now >= trigger_cooldown_until_ms_)
      {
        if (::IsWindow(owner_wnd_))
          ::PostMessage(owner_wnd_, WM_MOUSE_TRACKER_TRIGGERED,
                        static_cast<WPARAM>(MouseTrackerTrigger::DoubleClick), 0);

        trigger_cooldown_until_ms_ = now + 1000;
        moved_distance_px_         = 0;
      }

      has_last_click_ = false;
    }
    else
    {
      has_last_click_ = true;
      last_click_ms_  = now;
      last_click_pos_ = pt;
    }
  }

  if (enable_right_click && right_button_down && !last_right_button_down_)
  {
    if (now >= trigger_cooldown_until_ms_)
    {
      if (::IsWindow(owner_wnd_))
        ::PostMessage(owner_wnd_, WM_MOUSE_TRACKER_TRIGGERED,
                      static_cast<WPARAM>(MouseTrackerTrigger::RightClick), 0);

      trigger_cooldown_until_ms_ = now + 1000;
      moved_distance_px_         = 0;
    }
  }

  last_left_button_down_  = left_button_down;
  last_right_button_down_ = right_button_down;
}

void MouseTracker::ResetTracking()
{
  has_last_cursor_pos_       = false;
  moved_distance_px_         = 0;
  last_move_ms_              = 0;
  last_left_button_down_     = false;
  last_right_button_down_    = false;
  has_last_click_            = false;
  last_click_ms_             = 0;
  trigger_cooldown_until_ms_ = 0;
  background_since_ms_       = 0;
}

bool MouseTracker::IsOwnerForeground() const
{
  if (!::IsWindow(owner_wnd_))
    return false;

  HWND fg = ::GetForegroundWindow();
  if (!::IsWindow(fg))
    return false;

  if (fg == owner_wnd_)
    return true;

  if (::IsChild(owner_wnd_, fg))
    return true;

  return (::GetWindow(fg, GW_OWNER) == owner_wnd_);
}

ULONGLONG MouseTracker::NowMs() const
{
  return ::GetTickCount64();
}
