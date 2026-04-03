#pragma once

#include <array>

#include "UserOption.hpp"

class CWnd;

namespace AdminGuard {

enum class PromptResult
{
  Proceed,
  RestartRequested,
  Cancelled,
};

const std::array<LPCTSTR, 7>& GetAdminRequiredOptionList();
bool                          IsAdminRequiredOptionEnabled(const CString& option_key);
bool                          HasAnyAdminRequiredOptionEnabled();
PromptResult                  PromptAdminRestartIfNeeded(CWnd* owner, const CString* option_key = nullptr);

}  // namespace AdminGuard
