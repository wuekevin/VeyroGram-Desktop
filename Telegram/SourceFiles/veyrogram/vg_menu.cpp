/*
VeyroGram — commercial Telegram client
Based on SereinGram (GPL-3.0). VeyroGram extensions are also GPL-3.0.
*/
#include "veyrogram/vg_menu.h"

#include "veyrogram/vg_translate.h"

#include "serein/menu/contributors.h"
#include "history/history_item.h"
#include "ui/widgets/menu/menu_action.h"
#include "ui/widgets/menu/menu.h"
#include "ui/widgets/popup_menu.h"
#include "window/window_session_controller.h"
#include "styles/style_menu_icons.h"

namespace VeyroGram {

void RegisterTranslateMenuAction() {
	Serein::Menu::AddContributor([](const Serein::Menu::Context &ctx) {
		if (!ctx.item) return;

		const auto text = ctx.item->originalText().text.trimmed();
		if (text.isEmpty()) return;

		const auto controller = ctx.controller;
		const auto action = Ui::Menu::CreateAction(
			ctx.menu,
			u"AI 翻译"_q,
			crl::guard(controller, [show = controller->uiShow(), text] {
				ShowTranslateBox(show, text);
			}));
		auto widget = base::make_unique_q<Ui::Menu::Action>(
			ctx.menu->menu(),
			ctx.menu->menu()->st(),
			action,
			&st::menuIconTranslate,
			&st::menuIconTranslate);
		ctx.menu->insertAction(
			int(ctx.menu->actions().size()),
			std::move(widget));
	});
}

} // namespace VeyroGram
