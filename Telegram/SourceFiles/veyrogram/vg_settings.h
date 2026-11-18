/*
VeyroGram — commercial Telegram client
Based on SereinGram (GPL-3.0). VeyroGram extensions are also GPL-3.0.
*/
#pragma once

#include <memory>

namespace Settings::Builder {
class SectionBuilder;
} // namespace Settings::Builder

namespace Ui {
class Show;
} // namespace Ui

namespace VeyroGram {

void AddSettingsEntry(Settings::Builder::SectionBuilder &builder);
void ShowServicesBox(std::shared_ptr<Ui::Show> show);

} // namespace VeyroGram
