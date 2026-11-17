/*
VeyroGram — commercial Telegram client
Based on SereinGram (GPL-3.0). VeyroGram extensions are also GPL-3.0.
*/
#pragma once

namespace VeyroGram {

// Register the AI Translate action with Serein's message menu system.
// Call once during application init (from RegisterMessageMenu or modules.cpp).
void RegisterTranslateMenuAction();

} // namespace VeyroGram
