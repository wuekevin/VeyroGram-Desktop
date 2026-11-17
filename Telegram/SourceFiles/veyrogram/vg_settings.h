/*
VeyroGram — commercial Telegram client
Based on SereinGram (GPL-3.0). VeyroGram extensions are also GPL-3.0.
*/
#pragma once

namespace Settings::Builder {
class SectionBuilder;
} // namespace Settings::Builder

namespace VeyroGram {

// Add the VeyroGram Account entry to the settings home page.
void AddSettingsEntry(Settings::Builder::SectionBuilder &builder);

} // namespace VeyroGram
