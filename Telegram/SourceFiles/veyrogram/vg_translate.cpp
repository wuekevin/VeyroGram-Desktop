/*
VeyroGram — commercial Telegram client
Based on SereinGram (GPL-3.0). VeyroGram extensions are also GPL-3.0.
*/
#include "veyrogram/vg_translate.h"

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
#include <QtCore/QJsonArray>
#include <QtGui/QGuiApplication>
#include <QtGui/QClipboard>
#include <QtNetwork/QNetworkReply>

namespace VeyroGram {
namespace {

// Supported target languages (code, display name).
struct LangEntry { const char *code; const char *name; };
constexpr LangEntry kLangs[] = {
	{ "zh", "Chinese (Simplified)" },
	{ "en", "English" },
	{ "ja", "Japanese" },
	{ "ko", "Korean" },
	{ "ru", "Russian" },
	{ "fr", "French" },
	{ "de", "German" },
	{ "es", "Spanish" },
	{ "pt", "Portuguese" },
	{ "ar", "Arabic" },
};

// Fetch the first available model ULID from /ai/models and call back.
void FetchFirstModel(const QString &token, Fn<void(QString ulid)> done) {
	ApiGet(u"/ai/models"_q, token, [=](ApiResponse resp) {
		if (!resp.ok()) {
			done(QString());
			return;
		}
		const auto arr = QJsonDocument::fromJson(resp.body)
			.object()[u"models"_q].toArray();
		if (arr.isEmpty()) {
			done(QString());
			return;
		}
		done(arr[0].toObject()[u"ulid"_q].toString());
	});
}

void TranslateBox(
		not_null<Ui::GenericBox*> box,
		const QString &initialText) {
	box->setTitle(rpl::single(u"AI Translate"_q));

	const auto input = box->addRow(object_ptr<Ui::InputField>(
		box,
		st::defaultInputField,
		Ui::InputField::Mode::MultiLine,
		rpl::single(u"Text to translate"_q)));
	if (!initialText.isEmpty()) {
		input->setText(initialText);
	}

	// Target language selector — simple text field for now.
	const auto langField = box->addRow(object_ptr<Ui::InputField>(
		box,
		st::defaultInputField,
		Ui::InputField::Mode::SingleLine,
		rpl::single(u"Target language (e.g. en, zh, ja)"_q)));
	langField->setText(u"zh"_q); // default to Chinese

	box->addRow(object_ptr<Ui::FlatLabel>(
		box,
		rpl::single(u"Supported: zh en ja ko ru fr de es pt ar"_q),
		st::boxDividerLabel));

	const auto result = box->addRow(object_ptr<Ui::FlatLabel>(
		box,
		rpl::single(QString()),
		st::boxLabel));

	box->setFocusCallback([=] {
		if (initialText.isEmpty()) input->setFocusFast();
		else langField->setFocusFast();
	});

	const auto busy = box->lifetime().make_state<bool>(false);
	const auto streamReply =
		box->lifetime().make_state<QPointer<QNetworkReply>>();
	const auto resultText = box->lifetime().make_state<QString>();

	const auto doTranslate = [=] {
		if (*busy) return;
		if (!IsLoggedIn()) {
			box->uiShow()->showToast(
				u"Sign in to your VeyroGram account to use AI Translate."_q);
			return;
		}
		const auto text = input->getLastText().trimmed();
		const auto lang = langField->getLastText().trimmed();
		if (text.isEmpty() || lang.isEmpty()) {
			input->showError();
			return;
		}
		*busy = true;
		result->setText(u"Translating..."_q);
		const auto token = LoadCredentials().accessToken;

		FetchFirstModel(token, crl::guard(box, [=](QString modelUlid) {
			if (modelUlid.isEmpty()) {
				*busy = false;
				result->setText(
					u"No AI model available. Contact support."_q);
				return;
			}
			const auto accumulated = box->lifetime()
				.make_state<QString>();
			*streamReply = ApiStream(
				u"/ai/translate/stream"_q,
				QJsonObject{
					{ u"model_ulid"_q, modelUlid },
					{ u"source_lang"_q, u"auto"_q },
					{ u"target_lang"_q, lang },
					{ u"text"_q, text },
				},
				token,
				crl::guard(box, [=](QString chunk) {
					// Each chunk is a partial JSON {"delta":"..."}.
					const auto obj =
						QJsonDocument::fromJson(chunk.toUtf8()).object();
					const auto delta = obj[u"delta"_q].toString();
					if (!delta.isEmpty()) {
						accumulated->append(delta);
						*resultText = *accumulated;
						result->setText(*accumulated);
					}
				}),
				crl::guard(box, [=](QString error) {
					*busy = false;
					if (!error.isEmpty()) {
						result->setText(u"Error: %1"_q.arg(error));
					}
				}));
		}));
	};

	box->addButton(rpl::single(u"Translate"_q), doTranslate);
	box->addButton(rpl::single(u"Copy Result"_q), [=] {
		if (!resultText->isEmpty()) {
			QGuiApplication::clipboard()->setText(*resultText);
			box->uiShow()->showToast(u"Copied to clipboard."_q);
		}
	});
	box->addButton(rpl::single(u"Close"_q), [=] {
		if (*streamReply) (*streamReply)->abort();
		box->closeBox();
	});
}

} // namespace

void ShowTranslateBox(std::shared_ptr<Ui::Show> show, const QString &text) {
	show->showBox(Box(TranslateBox, text));
}

} // namespace VeyroGram
