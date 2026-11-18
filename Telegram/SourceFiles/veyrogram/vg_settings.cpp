/*
VeyroGram — commercial Telegram client
Based on SereinGram (GPL-3.0). VeyroGram extensions are also GPL-3.0.
*/
#include "veyrogram/vg_settings.h"

#include "veyrogram/vg_auth.h"
#include "veyrogram/vg_credentials.h"

#include "settings/settings_builder.h"
#include "styles/style_menu_icons.h"
#include "window/window_session_controller.h"

namespace VeyroGram {

void AddSettingsEntry(Settings::Builder::SectionBuilder &builder) {
	const auto show = builder.controller()
		? builder.controller()->uiShow()
		: nullptr;

	builder.addButton({
		.id = u"veyrogram/account"_q,
		.title = rpl::single(u"VeyroGram Account"_q),
		.icon = { &st::menuIconProfile },
		.label = rpl::single(
			IsLoggedIn()
			? LoadCredentials().email
			: u"Not signed in"_q),
		.onClick = [show] {
			if (!show) return;
			if (IsLoggedIn()) {
				ShowAccountBox(show);
			} else {
				ShowLoginBox(show);
			}
		},
		.keywords = { u"veyrogram"_q, u"account"_q, u"login"_q, u"translate"_q },
	});
}

} // namespace VeyroGram
