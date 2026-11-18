/*
VeyroGram — commercial Telegram client
Based on SereinGram (GPL-3.0). VeyroGram extensions are also GPL-3.0.
*/
#pragma once

#include "base/basic_types.h"

#include <QtCore/QString>
#include <memory>

namespace Ui {
class Show;
} // namespace Ui

namespace VeyroGram {

// Open a dialog to translate the given text.
// If text is empty the dialog shows a text-entry field.
void ShowTranslateBox(
	std::shared_ptr<Ui::Show> show,
	const QString &text = QString());

} // namespace VeyroGram
