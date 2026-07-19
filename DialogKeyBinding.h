#pragma once

class DialogKeyBinding : public CDialogEx
{
  DECLARE_DYNAMIC(DialogKeyBinding)

 public:
  DialogKeyBinding(DWORD current_hotkey, CWnd* pParent = nullptr);
  virtual ~DialogKeyBinding();

#ifdef AFX_DESIGN_TIME
  enum
  {
    IDD = IDD_KEY_BINDING
  };
#endif

 protected:
  virtual void DoDataExchange(CDataExchange* pDX);
  virtual BOOL PreTranslateMessage(MSG* pMsg);
  virtual BOOL OnInitDialog();
  virtual void OnOK();

  DECLARE_MESSAGE_MAP()

 public:
  DWORD hotkey_value_ = 0;

 private:
  CString hotkey_text_;
  void    SetHotkeyValue(UINT vk, UINT modifiers);
  void    ClearHotkey();
};
