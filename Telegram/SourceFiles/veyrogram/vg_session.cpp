/*
VeyroGram — commercial Telegram client
Based on SereinGram (GPL-3.0). VeyroGram extensions are also GPL-3.0.
*/
#include "veyrogram/vg_session.h"

#include "veyrogram/vg_credentials.h"

#include "core/application.h"
#include "core/core_settings.h"
#include "core/version.h"
#include "data/data_user.h"
#include "main/main_domain.h"
#include "main/main_session.h"

#include <QtCore/QDateTime>
#include <QtCore/QJsonDocument>
#include <QtCore/QObject>
#include <QtCore/QSysInfo>
#include <QtCore/QTimer>

#include <vector>

namespace VeyroGram {
namespace {

constexpr auto kHeartbeatInterval = 6 * 60 * 60 * 1000;
constexpr auto kRetryInterval = 5 * 60 * 1000;
constexpr auto kMaxRetries = 6;
constexpr auto kCloudAiKey = "vg_cloud_ai_enabled";
constexpr auto kShareProfileKey = "vg_share_telegram_profile";

struct TelegramProfile {
	qint64 userId = 0;
	QString username;
	QString displayName;

	[[nodiscard]] bool valid() const {
		return (userId != 0);
	}
};

struct Waiting {
	bool busy = false;
	std::vector<Fn<void(bool)>> waiters;
};

[[nodiscard]] SessionState &State() {
	static auto state = SessionState();
	return state;
}

[[nodiscard]] TelegramProfile &Profile() {
	static auto profile = TelegramProfile();
	return profile;
}

[[nodiscard]] Waiting &Queue() {
	static auto queue = Waiting();
	return queue;
}

void SetState(SessionStatus status, const QString &detail = QString()) {
	State() = SessionState{ .status = status, .detail = detail };
}

[[nodiscard]] bool ReadFlag(const char *key) {
	return Core::App().settings().readPref<bool>(key, true);
}

void WriteFlag(const char *key, bool value) {
	Core::App().settings().writePref<bool>(key, value);
	Core::App().saveSettingsDelayed();
}

[[nodiscard]] QString PlatformName() {
#if defined Q_OS_WIN
	return u"windows"_q;
#elif defined Q_OS_MAC
	return u"macos"_q;
#else
	return u"linux"_q;
#endif
}

[[nodiscard]] QString ErrorCodeOf(const ApiResponse &response) {
	return QJsonDocument::fromJson(response.body).object()
		[u"error"_q].toString();
}

[[nodiscard]] QString ErrorDetailOf(const ApiResponse &response) {
	return QJsonDocument::fromJson(response.body).object()
		[u"detail"_q].toString();
}

[[nodiscard]] QJsonObject TelegramJson() {
	// Profile only: id, username, display name. Never phone, chats or session.
	const auto &profile = Profile();
	if (!ShareTelegramProfile() || !profile.valid()) {
		return QJsonObject();
	}
	return QJsonObject{
		{ u"user_id"_q, profile.userId },
		{ u"username"_q, profile.username },
		{ u"display_name"_q, profile.displayName },
	};
}

[[nodiscard]] QJsonObject VersionJson() {
	return QJsonObject{
		{ u"client_version"_q, QString::fromLatin1(AppVersionStr) },
		{ u"os_version"_q, QSysInfo::prettyProductName() },
	};
}

void ApplyErrorState(const ApiResponse &response) {
	if (response.status == 0) {
		SetState(SessionStatus::Offline, response.error);
		return;
	}
	const auto code = ErrorCodeOf(response);
	if (code == u"ERR_REGISTRATION_CLOSED"_q) {
		SetState(SessionStatus::RegistrationClosed);
	} else if (code == u"ERR_ACCOUNT_DISABLED"_q) {
		SetState(SessionStatus::AccountDisabled);
	} else if (code == u"ERR_DEVICE_DISABLED"_q) {
		SetState(SessionStatus::DeviceDisabled);
	} else if (code == u"ERR_MAINTENANCE"_q || response.status == 503) {
		SetState(SessionStatus::Maintenance, ErrorDetailOf(response));
	} else if (code == u"ERR_RATE_LIMITED"_q || response.status == 429) {
		SetState(SessionStatus::RateLimited);
	} else if (code == u"ERR_INSTALL_SECRET_MISMATCH"_q) {
		SetState(SessionStatus::IdentityRejected);
	} else {
		SetState(SessionStatus::Offline, u"HTTP %1"_q.arg(response.status));
	}
}

[[nodiscard]] bool ShouldRetry() {
	switch (State().status) {
	case SessionStatus::Offline:
	case SessionStatus::Maintenance:
	case SessionStatus::RateLimited:
		return true;
	default:
		return false;
	}
}

class SessionKeeper final : public QObject {
public:
	explicit SessionKeeper(QObject *parent);
	~SessionKeeper();

	void syncNow();
	void scheduleRetry();

private:
	QTimer _heartbeat;
	int _retries = 0;
	bool _hasTelegramSession = false;
	rpl::lifetime _lifetime;

};

SessionKeeper *Keeper = nullptr;

[[nodiscard]] bool ApplyLogin(const ApiResponse &response) {
	if (!response.ok()) {
		ApplyErrorState(response);
		return false;
	}
	const auto object = QJsonDocument::fromJson(response.body).object();
	auto credentials = Credentials();
	credentials.accessToken = object[u"access_token"_q].toString();
	credentials.refreshToken = object[u"refresh_token"_q].toString();
	credentials.expiresAt = QDateTime::currentSecsSinceEpoch()
		+ object[u"expires_in"_q].toInt(900);
	if (credentials.accessToken.isEmpty()) {
		SetState(SessionStatus::Offline, u"empty token"_q);
		return false;
	}
	SaveCredentials(credentials);
	SetState(SessionStatus::Connected);
	return true;
}

void DeviceLogin(Fn<void(bool)> done) {
	const auto identity = LoadOrCreateInstallIdentity();
	auto body = VersionJson();
	body.insert(u"platform"_q, PlatformName());
	body.insert(u"install_id"_q, identity.installId);
	body.insert(u"install_secret"_q, identity.installSecret);
	body.insert(u"device_name"_q, QSysInfo::machineHostName());
	const auto telegram = TelegramJson();
	if (!telegram.isEmpty()) {
		body.insert(u"telegram"_q, telegram);
	}
	ApiPost(
		u"/auth/device-login"_q,
		body,
		QString(),
		[=](ApiResponse response) {
			done(ApplyLogin(response));
		});
}

void RefreshLogin(const Credentials &credentials, Fn<void(bool)> done) {
	ApiPost(
		u"/auth/refresh"_q,
		QJsonObject{ { u"refresh_token"_q, credentials.refreshToken } },
		QString(),
		[=](ApiResponse response) {
			if (response.ok()) {
				done(ApplyLogin(response));
			} else if (response.status == 0) {
				ApplyErrorState(response);
				done(false);
			} else {
				DeviceLogin(done);
			}
		});
}

void FinishEnsure(bool ok) {
	auto &queue = Queue();
	queue.busy = false;
	auto waiters = std::move(queue.waiters);
	queue.waiters.clear();
	if (!ok && Keeper && ShouldRetry()) {
		Keeper->scheduleRetry();
	}
	for (const auto &waiter : waiters) {
		waiter(ok);
	}
}

void Authed(
		Fn<void(QString token, Fn<void(ApiResponse)> done)> send,
		Fn<void(ApiResponse)> done) {
	const auto attempt = [=](bool retried, auto self) -> void {
		WithToken([=](QString token) {
			if (token.isEmpty()) {
				done(ApiResponse{ .status = 0, .error = u"no session"_q });
				return;
			}
			send(token, [=](ApiResponse response) {
				if (response.status == 401 && !retried) {
					InvalidateSession();
					self(true, self);
					return;
				}
				done(response);
			});
		});
	};
	attempt(false, attempt);
}

void SendHeartbeat() {
	auto body = VersionJson();
	const auto telegram = TelegramJson();
	if (!telegram.isEmpty()) {
		body.insert(u"telegram"_q, telegram);
	}
	AuthedPost(u"/client/heartbeat"_q, body, [](ApiResponse response) {
		if (!response.ok() && response.status != 0) {
			ApplyErrorState(response);
		}
	});
}

void CaptureProfile(Main::Session *session) {
	auto &profile = Profile();
	profile = TelegramProfile();
	if (!session) {
		return;
	}
	const auto user = session->user();
	profile.userId = qint64(session->userId().bare);
	profile.username = user->username();
	profile.displayName = user->name();
}

SessionKeeper::SessionKeeper(QObject *parent)
: QObject(parent) {
	Keeper = this;
	connect(&_heartbeat, &QTimer::timeout, this, [=] {
		if (_hasTelegramSession) {
			syncNow();
		}
	});
	_heartbeat.start(kHeartbeatInterval);

	Core::App().domain().activeSessionValue(
	) | rpl::on_next([=](Main::Session *session) {
		CaptureProfile(session);
		_hasTelegramSession = (session != nullptr);
		if (_hasTelegramSession) {
			_retries = 0;
			syncNow();
		}
	}, _lifetime);
}

SessionKeeper::~SessionKeeper() {
	if (Keeper == this) {
		Keeper = nullptr;
	}
}

void SessionKeeper::syncNow() {
	EnsureSession([](bool ok) {
		if (ok) {
			SendHeartbeat();
		}
	});
}

void SessionKeeper::scheduleRetry() {
	if (++_retries > kMaxRetries) {
		return;
	}
	QTimer::singleShot(kRetryInterval, this, [=] {
		if (_hasTelegramSession) {
			syncNow();
		}
	});
}

} // namespace

SessionState CurrentSessionState() {
	return State();
}

QString SessionStatusText(const SessionState &state) {
	switch (state.status) {
	case SessionStatus::Idle:
		return u"未连接"_q;
	case SessionStatus::Connecting:
		return u"连接中…"_q;
	case SessionStatus::Connected:
		return u"已连接"_q;
	case SessionStatus::Offline:
		return u"无法连接服务器，稍后将自动重试"_q;
	case SessionStatus::RegistrationClosed:
		return u"暂未开放新设备注册"_q;
	case SessionStatus::AccountDisabled:
		return u"账号已被禁用，请联系支持"_q;
	case SessionStatus::DeviceDisabled:
		return u"此设备已被禁用，请联系支持"_q;
	case SessionStatus::Maintenance:
		return state.detail.isEmpty()
			? u"服务维护中，请稍后再试"_q
			: state.detail;
	case SessionStatus::RateLimited:
		return u"请求过于频繁，请稍后再试"_q;
	case SessionStatus::IdentityRejected:
		return u"设备身份校验失败，请联系支持"_q;
	}
	return QString();
}

QString ApiErrorText(const QString &code) {
	if (code == u"ERR_AI_DISABLED"_q) {
		return u"AI 功能已被管理员暂时关闭"_q;
	} else if (code == u"ERR_QUOTA_EXCEEDED"_q) {
		return u"AI 额度已用完，请联系管理员或升级套餐"_q;
	} else if (code == u"ERR_MAINTENANCE"_q) {
		return u"服务维护中，请稍后再试"_q;
	} else if (code == u"ERR_MODEL_NOT_FOUND"_q) {
		return u"当前没有可用的 AI 模型"_q;
	} else if (code == u"ERR_PROVIDER_ERROR"_q) {
		return u"AI 服务暂时不可用，请稍后重试"_q;
	} else if (code == u"ERR_RATE_LIMITED"_q) {
		return u"请求过于频繁，请稍后再试"_q;
	} else if (code == u"ERR_ACCOUNT_DISABLED"_q) {
		return u"账号已被禁用，请联系支持"_q;
	} else if (code == u"ERR_DEVICE_DISABLED"_q) {
		return u"此设备已被禁用，请联系支持"_q;
	} else if (code == u"ERR_TOKEN_INVALID"_q
		|| code == u"ERR_UNAUTHORIZED"_q) {
		return u"登录状态已刷新，请重试一次"_q;
	} else if (code == u"ERR_VALIDATION"_q
		|| code == u"ERR_INVALID_REQUEST"_q) {
		return u"请求内容无效（文本可能过长）"_q;
	}
	return QString();
}

bool CloudAiEnabled() {
	return ReadFlag(kCloudAiKey);
}

void SetCloudAiEnabled(bool enabled) {
	WriteFlag(kCloudAiKey, enabled);
}

bool ShareTelegramProfile() {
	return ReadFlag(kShareProfileKey);
}

void SetShareTelegramProfile(bool enabled) {
	WriteFlag(kShareProfileKey, enabled);
	if (enabled && Keeper) {
		Keeper->syncNow();
	}
}

QString DeviceShortId() {
	return LoadOrCreateInstallIdentity().installId.left(8);
}

void StartSession() {
	if (!Keeper) {
		new SessionKeeper(&Core::App());
	}
}

void EnsureSession(Fn<void(bool ok)> done) {
	const auto credentials = LoadCredentials();
	if (credentials.valid() && !credentials.expired()) {
		if (done) {
			done(true);
		}
		return;
	}
	auto &queue = Queue();
	if (done) {
		queue.waiters.push_back(std::move(done));
	}
	if (queue.busy) {
		return;
	}
	queue.busy = true;
	SetState(SessionStatus::Connecting);
	if (!credentials.refreshToken.isEmpty()) {
		RefreshLogin(credentials, FinishEnsure);
	} else {
		DeviceLogin(FinishEnsure);
	}
}

void WithToken(Fn<void(QString token)> done) {
	EnsureSession([=](bool ok) {
		done(ok ? LoadCredentials().accessToken : QString());
	});
}

void AuthedGet(const QString &path, Fn<void(ApiResponse)> done) {
	Authed([=](QString token, Fn<void(ApiResponse)> callback) {
		ApiGet(path, token, std::move(callback));
	}, std::move(done));
}

void AuthedPost(
		const QString &path,
		const QJsonObject &json,
		Fn<void(ApiResponse)> done) {
	Authed([=](QString token, Fn<void(ApiResponse)> callback) {
		ApiPost(path, json, token, std::move(callback));
	}, std::move(done));
}

void InvalidateSession() {
	auto credentials = LoadCredentials();
	credentials.accessToken = QString();
	credentials.expiresAt = 0;
	SaveCredentials(credentials);
}

} // namespace VeyroGram
