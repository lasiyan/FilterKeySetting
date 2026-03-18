// clang-format off
#include "pch.h"
#include "UserPresetOSD.hpp"
#include "UserOption.hpp"
// clang-format on

namespace {
class PresetOSDWindow : public CWnd
{
 public:
  bool EnsureCreated()
  {
    if (::IsWindow(GetSafeHwnd()))
      return true;

    CString class_name = AfxRegisterWndClass(0);
    return CreateEx(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT,
                    class_name,
                    _T("PresetOSD"),
                    WS_POPUP,
                    CRect(0, 0, 1, 1),
                    nullptr,
                    0);
  }

  void ShowText(const CString&        text,
                BYTE                  alpha,
                UserPresetOSD::Corner corner,
                int                   size_percent,
                UINT                  auto_hide_ms)
  {
    if (!EnsureCreated())
      return;

    text_ = text;

    const RECT target_work = ResolveTargetWorkRect();
    const int  work_h      = target_work.bottom - target_work.top;
    const int  target_h    = max(30, (work_h * size_percent) / 100);
    const int  font_px     = max(12, (target_h * 70) / 100);

    EnsureFont(font_px);

    CClientDC dc(this);
    HGDIOBJ   old_font = dc.SelectObject(&font_);

    CRect calc(0, 0, 0, 0);
    dc.DrawText(text_, &calc, DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_CALCRECT);

    dc.SelectObject(old_font);

    const int pad_x = max(4, font_px / 5);
    const int width = calc.Width() + pad_x * 2;
    const int hgt   = target_h;
    POINT     pt    = ResolveCornerPoint(target_work, width, hgt, corner);

    SetWindowPos(&CWnd::wndTopMost, pt.x, pt.y, width, hgt,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
    ::SetLayeredWindowAttributes(GetSafeHwnd(), 0, alpha, LWA_ALPHA);

    KillTimer(1);
    if (auto_hide_ms > 0)
      SetTimer(1, auto_hide_ms, nullptr);
    Invalidate(FALSE);
    UpdateWindow();
  }

  void UpdateAlpha(BYTE alpha)
  {
    if (!::IsWindow(GetSafeHwnd()))
      return;

    ::SetLayeredWindowAttributes(GetSafeHwnd(), 0, alpha, LWA_ALPHA);
    Invalidate(FALSE);
    UpdateWindow();
  }

  void ReapplyCurrent(BYTE                  alpha,
                      UserPresetOSD::Corner corner,
                      int                   size_percent,
                      UINT                  auto_hide_ms,
                      bool                  show_if_hidden)
  {
    if (!show_if_hidden && !IsShowing())
      return;

    if (text_.IsEmpty())
      return;

    ShowText(text_, alpha, corner, size_percent, auto_hide_ms);
  }

  bool IsShowing() const
  {
    return (::IsWindow(GetSafeHwnd()) && ::IsWindowVisible(GetSafeHwnd()));
  }

 protected:
  afx_msg void OnPaint()
  {
    CPaintDC dc(this);
    CRect    rc;
    GetClientRect(&rc);

    dc.FillSolidRect(&rc, RGB(22, 22, 22));
    dc.SetBkMode(TRANSPARENT);
    dc.SetTextColor(RGB(255, 70, 70));
    dc.SelectObject(&font_);

    dc.DrawText(text_, &rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_END_ELLIPSIS);
  }

  afx_msg void OnTimer(UINT_PTR id)
  {
    if (id == 1)
    {
      KillTimer(1);
      ShowWindow(SW_HIDE);
      return;
    }

    CWnd::OnTimer(id);
  }

  afx_msg LRESULT OnNcHitTest(CPoint point)
  {
    UNREFERENCED_PARAMETER(point);
    // Pass all mouse interaction through to windows behind OSD.
    return HTTRANSPARENT;
  }

  DECLARE_MESSAGE_MAP()

 private:
  void EnsureFont(int pixel_height)
  {
    if (font_height_ == pixel_height && font_.GetSafeHandle())
      return;

    if (font_.GetSafeHandle())
      font_.DeleteObject();

    LOGFONT lf  = {};
    lf.lfHeight = -pixel_height;
    lf.lfWeight = FW_BOLD;
    _tcscpy_s(lf.lfFaceName, _T("Segoe UI"));

    font_.CreateFontIndirect(&lf);
    font_height_ = pixel_height;
  }

  static RECT ResolveTargetWorkRect()
  {
    MONITORINFO mi  = { sizeof(mi) };
    HMONITOR    mon = nullptr;

    if (auto* main_wnd = AfxGetMainWnd(); main_wnd)
    {
      const HWND main_hwnd = main_wnd->GetSafeHwnd();
      if (::IsWindow(main_hwnd) && ::IsWindowVisible(main_hwnd) && !::IsIconic(main_hwnd))
        mon = ::MonitorFromWindow(main_hwnd, MONITOR_DEFAULTTOPRIMARY);
    }

    if (!mon)
    {
      POINT cursor = {
        0,
      };
      ::GetCursorPos(&cursor);
      mon = ::MonitorFromPoint(cursor, MONITOR_DEFAULTTOPRIMARY);
    }

    GetMonitorInfo(mon, &mi);
    return mi.rcWork;
  }

  static POINT ResolveCornerPoint(const RECT& work, int width, int height, UserPresetOSD::Corner corner)
  {
    constexpr int margin = 0;

    POINT pt{};
    switch (corner)
    {
      case UserPresetOSD::Corner::TopLeft:
        pt.x = work.left + margin;
        pt.y = work.top + margin;
        break;
      case UserPresetOSD::Corner::TopRight:
        pt.x = work.right - width - margin;
        pt.y = work.top + margin;
        break;
      case UserPresetOSD::Corner::BottomLeft:
        pt.x = work.left + margin;
        pt.y = work.bottom - height - margin;
        break;
      case UserPresetOSD::Corner::BottomRight:
      default:
        pt.x = work.right - width - margin;
        pt.y = work.bottom - height - margin;
        break;
    }

    return pt;
  }

 private:
  CString text_;
  CFont   font_;
  int     font_height_ = 0;
};

BEGIN_MESSAGE_MAP(PresetOSDWindow, CWnd)
ON_WM_PAINT()
ON_WM_TIMER()
ON_WM_NCHITTEST()
END_MESSAGE_MAP()

PresetOSDWindow& OSDWindow()
{
  static PresetOSDWindow wnd;
  return wnd;
}

}  // namespace

namespace UserPresetOSD {

int ClampPosition(int value)
{
  if (value < static_cast<int>(Corner::TopLeft))
    return static_cast<int>(Corner::TopLeft);
  if (value > static_cast<int>(Corner::BottomRight))
    return static_cast<int>(Corner::BottomRight);
  return value;
}

int ClampAlpha(int value)
{
  if (value < 0)
    return 0;
  if (value > 255)
    return 255;
  return value;
}

int ClampSizePercent(int value)
{
  if (value < 1)
    return 1;
  if (value > 10)
    return 10;
  return value;
}

namespace {
struct OsdOptions
{
  bool   enabled;
  Corner corner;
  int    alpha;
  bool   keep_visible;
  UINT   hide_ms;
  int    size_percent;
};

OsdOptions LoadOsdOptions()
{
  OsdOptions opts   = {};
  opts.enabled      = (GLOBAL_OPTION.getInteger(KEY_ENABLE_PRESET_OSD, 0) != 0);
  opts.corner       = static_cast<Corner>(ClampPosition(static_cast<int>(GLOBAL_OPTION.getInteger(KEY_PRESET_OSD_CORNER, static_cast<DWORD>(Corner::BottomRight)))));
  opts.alpha        = ClampAlpha(static_cast<int>(GLOBAL_OPTION.getInteger(KEY_PRESET_OSD_ALPHA, 220)));
  opts.keep_visible = (GLOBAL_OPTION.getInteger(KEY_PRESET_OSD_KEEP_VISIBLE, 0) != 0);
  opts.hide_ms      = opts.keep_visible ? 0u : 3000u;
  opts.size_percent = ClampSizePercent(static_cast<int>(GLOBAL_OPTION.getInteger(KEY_PRESET_OSD_SIZE, 5)));
  return opts;
}
}  // namespace

void InitializeOptionDefaults()
{
  GLOBAL_OPTION.setInit(KEY_ENABLE_PRESET_OSD, false);
  GLOBAL_OPTION.setInit(KEY_PRESET_OSD_CORNER, static_cast<DWORD>(Corner::BottomRight));
  GLOBAL_OPTION.setInit(KEY_PRESET_OSD_ALPHA, static_cast<DWORD>(220));
  GLOBAL_OPTION.setInit(KEY_PRESET_OSD_KEEP_VISIBLE, false);
  GLOBAL_OPTION.setInit(KEY_PRESET_OSD_SIZE, static_cast<DWORD>(5));
}

void ShowPresetIndex(int preset_index)
{
  const auto opts = LoadOsdOptions();
  if (!opts.enabled)
    return;

  if (preset_index < 0)
    preset_index = 0;
  if (preset_index > 99)
    preset_index = 99;

  CString text;
  text.Format(_T("%d"), preset_index);

  OSDWindow().ShowText(text, static_cast<BYTE>(opts.alpha), opts.corner, opts.size_percent, opts.hide_ms);
}

void RefreshDisplay()
{
  const auto opts = LoadOsdOptions();
  if (!opts.enabled)
  {
    if (OSDWindow().IsShowing())
      OSDWindow().ShowWindow(SW_HIDE);
    return;
  }

  OSDWindow().ReapplyCurrent(static_cast<BYTE>(opts.alpha), opts.corner, opts.size_percent, opts.hide_ms, true);
}

}  // namespace UserPresetOSD
