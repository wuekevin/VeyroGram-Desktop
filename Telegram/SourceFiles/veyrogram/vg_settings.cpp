/*
VeyroGram — commercial Telegram client
Based on SereinGram (GPL-3.0). VeyroGram extensions are also GPL-3.0.
*/
#include "veyrogram/vg_settings.h"

#include "veyrogram/vg_credentials.h"
#include "veyrogram/vg_session.h"

#include "core/version.h"
#include "settings/settings_builder.h"
#include "styles/style_boxes.h"
#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"
#include "ui/layers/generic_box.h"
#include "ui/toast/toast.h"
#include "ui/widgets/checkbox.h"
#include "ui/widgets/labels.h"
#include "window/window_session_controller.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace VeyroGram {
namespace {

[[nodiscard]] QString StatusLine() {
	return u"连接状态：%1"_q.arg(SessionStatusText(CurrentSessionState()));
}

void ServicesBox(not_null<Ui::GenericBox*> box) {
	box->setTitle(rpl::single(u"VeyroGram 服务"_q));

	const auto status = box->addRow(object_ptr<Ui::FlatLabel>(
		box,
		rpl::single(StatusLine()),
		st::boxLabel));
	const auto plan = box->addRow(object_ptr<Ui::FlatLabel>(
		box,
		rpl::single(u"套餐与额度：加载中…"_q),
		st::boxLabel));
	box->addRow(object_ptr<Ui::FlatLabel>(
		box,
		rpl::single(u"设备 ID：%1　客户端版本：%2"_q.arg(
			DeviceShortId(),
			QString::fromLatin1(AppVersionStr))),
		st::boxLabel));
	if (!LoadOrCreateInstallIdentity().persisted) {
		box->addRow(object_ptr<Ui::FlatLabel>(
			box,
			rpl::single(
				u"无法安全保存设备凭据，重启后可能被识别为新设备。"_q),
			st::boxLabel));
	}

	const auto cloudAi = box->addRow(object_ptr<Ui::Checkbox>(
		box,
		u"启用云端 AI 翻译"_q,
		CloudAiEnabled(),
		st::defaultBoxCheckbox));
	const auto shareProfile = box->addRow(object_ptr<Ui::Checkbox>(
		box,
		u"向 VeyroGram 同步 Telegram 资料（ID / 用户名 / 昵称）"_q,
		ShareTelegramProfile(),
		st::defaultBoxCheckbox));

	box->addRow(object_ptr<Ui::FlatLabel>(
		box,
		rpl::single(
			u"会上传：设备名称、系统与客户端版本；开启同步后还包括 "
			"Telegram 用户 ID、用户名和昵称；使用 AI 翻译时，你选择翻译的"
			"那段文字会发送到服务器并交给 AI 服务商处理。"_q),
		st::boxDividerLabel));
	box->addRow(object_ptr<Ui::FlatLabel>(
		box,
		rpl::single(
			u"不会上传：聊天记录、联系人、手机号、登录验证码、两步验证"
			"密码和 Telegram 登录会话。关闭同步只会停止之后的上传。"_q),
		st::boxDividerLabel));

	const auto refresh = [=] {
		status->setText(StatusLine());
		plan->setText(u"套餐与额度：加载中…"_q);
		EnsureSession(crl::guard(box, [=](bool) {
			status->setText(StatusLine());
			AuthedGet(u"/entitlements"_q, crl::guard(box, [=](
					ApiResponse entitlements) {
				if (!entitlements.ok()) {
					plan->setText(u"套餐与额度：暂时无法获取"_q);
					return;
				}
				const auto name = QJsonDocument::fromJson(entitlements.body)
					.object()[u"plan_name"_q].toString();
				AuthedGet(u"/ai/quota"_q, crl::guard(box, [=](
						ApiResponse quota) {
					const auto balance = qint64(QJsonDocument::fromJson(
						quota.body).object()[u"token_balance"_q].toDouble());
					plan->setText(quota.ok()
						? u"套餐：%1　AI 剩余额度：%2 tokens"_q.arg(
							name,
							QString::number(balance))
						: u"套餐：%1"_q.arg(name));
				}));
			}));
		}));
	};
	refresh();

	box->addButton(rpl::single(u"保存"_q), [=] {
		SetCloudAiEnabled(cloudAi->checked());
		SetShareTelegramProfile(shareProfile->checked());
		box->closeBox();
		box->uiShow()->showToast(u"已保存"_q);
	});
	box->addButton(rpl::single(u"刷新"_q), refresh);
	box->addButton(rpl::single(u"关闭"_q), [=] { box->closeBox(); });
}

} // namespace

void ShowServicesBox(std::shared_ptr<Ui::Show> show) {
	show->showBox(Box(ServicesBox));
}

void AddSettingsEntry(Settings::Builder::SectionBuilder &builder) {
	const auto show = builder.controller()
		? builder.controller()->uiShow()
		: nullptr;

	builder.addButton({
		.id = u"veyrogram/services"_q,
		.title = rpl::single(u"VeyroGram 服务"_q),
		.icon = { &st::menuIconProfile },
		.label = rpl::single(SessionStatusText(CurrentSessionState())),
		.onClick = [show] {
			if (show) {
				ShowServicesBox(show);
			}
		},
		.keywords = {
			u"veyrogram"_q,
			u"services"_q,
			u"translate"_q,
			u"服务"_q,
			u"翻译"_q,
		},
	});
}

} // namespace VeyroGram
