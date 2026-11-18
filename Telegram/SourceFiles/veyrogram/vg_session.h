/*
VeyroGram — commercial Telegram client
Based on SereinGram (GPL-3.0). VeyroGram extensions are also GPL-3.0.
*/
#pragma once

#include "base/basic_types.h"
#include "veyrogram/vg_api.h"

#include <QtCore/QJsonObject>
#include <QtCore/QString>

namespace VeyroGram {

enum class SessionStatus {
	Idle,
	Connecting,
	Connected,
	Offline,
	RegistrationClosed,
	AccountDisabled,
	DeviceDisabled,
	Maintenance,
	RateLimited,
	IdentityRejected,
};

struct SessionState {
	SessionStatus status = SessionStatus::Idle;
	QString detail;
};

[[nodiscard]] SessionState CurrentSessionState();
[[nodiscard]] QString SessionStatusText(const SessionState &state);
[[nodiscard]] QString ApiErrorText(const QString &code);

[[nodiscard]] bool CloudAiEnabled();
void SetCloudAiEnabled(bool enabled);
[[nodiscard]] bool ShareTelegramProfile();
void SetShareTelegramProfile(bool enabled);

[[nodiscard]] QString DeviceShortId();

void StartSession();
void EnsureSession(Fn<void(bool ok)> done);
void WithToken(Fn<void(QString token)> done);
void AuthedGet(const QString &path, Fn<void(ApiResponse)> done);
void AuthedPost(
	const QString &path,
	const QJsonObject &json,
	Fn<void(ApiResponse)> done);
void InvalidateSession();

} // namespace VeyroGram
