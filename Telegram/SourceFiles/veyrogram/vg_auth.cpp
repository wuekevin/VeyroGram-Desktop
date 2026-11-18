/*
VeyroGram — commercial Telegram client
Based on SereinGram (GPL-3.0). VeyroGram extensions are also GPL-3.0.
*/
#include "veyrogram/vg_auth.h"

#include "veyrogram/vg_api.h"
#include "veyrogram/vg_credentials.h"

#include "ui/layers/generic_box.h"
#include "ui/widgets/fields/input_field.h"
#include "ui/widgets/labels.h"
#include "ui/toast/toast.h"
#include "styles/style_layers.h"
#include "styles/style_boxes.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QDateTime>

namespace VeyroGram {
namespace {

void HandleAuthResponse(
		not_null<Ui::GenericBox*> box,
		const ApiResponse &resp,
		const QString &email) {
	if (!resp.ok()) {
		const auto err = QJsonDocument::fromJson(resp.body)
			.object()[u"error"_q].toString();
		QString msg;
		if (err == u"ERR_INVALID_CREDENTIALS"_q) {
			msg = u"Email or password incorrect."_q;
		} else if (err == u"ERR_EMAIL_TAKEN"_q) {
			msg = u"Email already registered."_q;
		} else if (err == u"ERR_ACCOUNT_DISABLED"_q) {
			msg = u"Account disabled. Contact support."_q;
		} else if (err == u"ERR_VALIDATION"_q) {
			msg = u"Invalid email or password (min 8 chars)."_q;
		} else {
			msg = u"Server error (%1). Try again."_q.arg(resp.status);
		}
		box->uiShow()->showToast(msg);
		return;
	}
	const auto obj = QJsonDocument::fromJson(resp.body).object();
	Credentials creds;
	creds.email = email;
	creds.accessToken = obj[u"access_token"_q].toString();
	creds.refreshToken = obj[u"refresh_token"_q].toString();
	const auto expiresIn = obj[u"expires_in"_q].toInt(3600);
	creds.expiresAt = QDateTime::currentSecsSinceEpoch() + expiresIn;
	SaveCredentials(creds);
	box->closeBox();
	box->uiShow()->showToast(u"Signed in as %1"_q.arg(email));
}

void LoginBox(not_null<Ui::GenericBox*> box) {
	box->setTitle(rpl::single(u"VeyroGram Account"_q));

	const auto email = box->addRow(object_ptr<Ui::InputField>(
		box,
		st::defaultInputField,
		Ui::InputField::Mode::SingleLine,
		rpl::single(u"Email"_q)));

	const auto password = box->addRow(object_ptr<Ui::InputField>(
		box,
		st::defaultInputField,
		Ui::InputField::Mode::SingleLine,
		rpl::single(u"Password"_q)));

	box->addRow(object_ptr<Ui::FlatLabel>(
		box,
		rpl::single(u"Sign in to your VeyroGram account to unlock AI translation and premium features."_q),
		st::boxLabel));

	box->setFocusCallback([=] { email->setFocusFast(); });

	const auto busy = box->lifetime().make_state<bool>(false);

	const auto doLogin = [=] {
		if (*busy) return;
		const auto mail = email->getLastText().trimmed();
		const auto pass = password->getLastText();
		if (mail.isEmpty() || pass.isEmpty()) {
			email->showError();
			return;
		}
		*busy = true;
		ApiPost(
			u"/auth/login"_q,
			QJsonObject{
				{ u"email"_q, mail },
				{ u"password"_q, pass },
			},
			QString(),
			crl::guard(box, [=](ApiResponse resp) {
				*busy = false;
				HandleAuthResponse(box, resp, mail);
			}));
	};

	box->addButton(rpl::single(u"Sign In"_q), doLogin);

	const auto doRegister = [=] {
		if (*busy) return;
		const auto mail = email->getLastText().trimmed();
		const auto pass = password->getLastText();
		if (mail.isEmpty() || pass.size() < 8) {
			password->showError();
			box->uiShow()->showToast(u"Password must be at least 8 characters."_q);
			return;
		}
		*busy = true;
		ApiPost(
			u"/auth/register"_q,
			QJsonObject{
				{ u"email"_q, mail },
				{ u"password"_q, pass },
			},
			QString(),
			crl::guard(box, [=](ApiResponse resp) {
				*busy = false;
				HandleAuthResponse(box, resp, mail);
			}));
	};

	box->addButton(rpl::single(u"Create Account"_q), doRegister);
	box->addButton(rpl::single(u"Cancel"_q), [=] { box->closeBox(); });
}

void AccountBox(not_null<Ui::GenericBox*> box) {
	box->setTitle(rpl::single(u"VeyroGram Account"_q));

	const auto creds = LoadCredentials();
	box->addRow(object_ptr<Ui::FlatLabel>(
		box,
		rpl::single(u"Signed in as: %1"_q.arg(creds.email)),
		st::boxLabel));

	const auto busy = box->lifetime().make_state<bool>(false);

	box->addButton(rpl::single(u"Sign Out"_q), [=] {
		if (*busy) return;
		*busy = true;
		const auto token = LoadCredentials().accessToken;
		ApiPost(
			u"/auth/logout"_q,
			QJsonObject{},
			token,
			crl::guard(box, [=](ApiResponse) {
				ClearCredentials();
				box->closeBox();
				box->uiShow()->showToast(u"Signed out."_q);
			}));
	});

	box->addButton(rpl::single(u"Close"_q), [=] { box->closeBox(); });
}

} // namespace

void ShowLoginBox(std::shared_ptr<Ui::Show> show) {
	show->showBox(Box(LoginBox));
}

void ShowAccountBox(std::shared_ptr<Ui::Show> show) {
	show->showBox(Box(AccountBox));
}

void Logout(Fn<void()> done) {
	const auto token = LoadCredentials().accessToken;
	ApiPost(
		u"/auth/logout"_q,
		QJsonObject{},
		token,
		[=](ApiResponse) {
			ClearCredentials();
			if (done) done();
		});
}

} // namespace VeyroGram
