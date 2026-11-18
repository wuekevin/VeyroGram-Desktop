/*
VeyroGram — commercial Telegram client
Based on SereinGram (GPL-3.0). VeyroGram extensions are also GPL-3.0.
*/
#pragma once

#include <QtCore/QString>

namespace VeyroGram {

struct Credentials {
	QString accessToken;
	QString refreshToken;
	qint64 expiresAt = 0;

	[[nodiscard]] bool valid() const;
	[[nodiscard]] bool expired() const;
};

[[nodiscard]] Credentials LoadCredentials();
bool SaveCredentials(const Credentials &creds);
bool ClearCredentials();

struct InstallIdentity {
	QString installId;
	QString installSecret;
	bool persisted = false;
};

[[nodiscard]] InstallIdentity LoadOrCreateInstallIdentity();

} // namespace VeyroGram
