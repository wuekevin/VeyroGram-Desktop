/*
VeyroGram — commercial Telegram client
Based on SereinGram (GPL-3.0). VeyroGram extensions are also GPL-3.0.
*/
#pragma once

#include <QtCore/QString>
#include <QtCore/QObject>

namespace VeyroGram {

struct Credentials {
	QString accessToken;
	QString refreshToken;
	QString email;
	qint64 expiresAt = 0; // Unix timestamp (seconds)

	[[nodiscard]] bool valid() const;
	[[nodiscard]] bool expired() const;
};

[[nodiscard]] Credentials LoadCredentials();
void SaveCredentials(const Credentials &creds);
void ClearCredentials();
[[nodiscard]] bool IsLoggedIn();

} // namespace VeyroGram
