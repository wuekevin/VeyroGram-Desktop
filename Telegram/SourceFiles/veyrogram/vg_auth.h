/*
VeyroGram — commercial Telegram client
Based on SereinGram (GPL-3.0). VeyroGram extensions are also GPL-3.0.
*/
#pragma once

#include "base/basic_types.h"

#include <memory>

namespace Ui {
class Show;
} // namespace Ui

namespace VeyroGram {

// Open the VeyroGram login/register dialog.
void ShowLoginBox(std::shared_ptr<Ui::Show> show);

// Open the VeyroGram account info + logout dialog.
void ShowAccountBox(std::shared_ptr<Ui::Show> show);

// Log out and clear stored credentials.
// done is called (from the main thread) when complete.
void Logout(Fn<void()> done);

} // namespace VeyroGram
