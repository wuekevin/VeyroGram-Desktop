/*
VeyroGram — commercial Telegram client
Based on SereinGram (GPL-3.0). VeyroGram extensions are also GPL-3.0.
*/
#include "veyrogram/vg_credentials.h"

#include "serein/ports/credentials.h"

#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QRandomGenerator>
#include <QtCore/QStandardPaths>
#include <QtCore/QUuid>

#include <cstring>
#include <optional>

namespace VeyroGram {
namespace {

constexpr auto kSecretBytes = 32;
constexpr auto kSecretHexSize = kSecretBytes * 2;

const auto kSessionAccount = u"veyrogram/session"_q;
const auto kInstallAccount = u"veyrogram/install"_q;

struct SessionCache {
	bool loaded = false;
	Credentials value;
};

[[nodiscard]] SessionCache &Cache() {
	static auto cache = SessionCache();
	return cache;
}

void RemoveLegacyFile() {
	const auto base = QStandardPaths::writableLocation(
		QStandardPaths::AppDataLocation);
	QFile::remove(base + u"/VeyroGram/vg_session.dat"_q);
}

[[nodiscard]] QString RandomSecretHex() {
	auto bytes = QByteArray(kSecretBytes, '\0');
	for (auto i = 0; i != kSecretBytes / 4; ++i) {
		const auto value = quint32(QRandomGenerator::system()->generate());
		std::memcpy(bytes.data() + i * 4, &value, sizeof(value));
	}
	return QString::fromLatin1(bytes.toHex());
}

} // namespace

bool Credentials::valid() const {
	return !accessToken.isEmpty();
}

bool Credentials::expired() const {
	if (expiresAt <= 0) {
		return false;
	}
	return (QDateTime::currentSecsSinceEpoch() >= expiresAt - 60);
}

Credentials LoadCredentials() {
	auto &cache = Cache();
	if (cache.loaded) {
		return cache.value;
	}
	cache.loaded = true;
	RemoveLegacyFile();

	const auto result = Serein::Ports::SecretStore::Read(kSessionAccount);
	if (result.error != Serein::CredentialError::None) {
		return cache.value;
	}
	const auto object = QJsonDocument::fromJson(result.secret).object();
	cache.value.accessToken = object[u"access"_q].toString();
	cache.value.refreshToken = object[u"refresh"_q].toString();
	cache.value.expiresAt = qint64(object[u"expires_at"_q].toDouble());
	return cache.value;
}

bool SaveCredentials(const Credentials &creds) {
	auto &cache = Cache();
	cache.loaded = true;
	cache.value = creds;
	const auto object = QJsonObject{
		{ u"access"_q, creds.accessToken },
		{ u"refresh"_q, creds.refreshToken },
		{ u"expires_at"_q, creds.expiresAt },
	};
	return (Serein::Ports::SecretStore::Write(
		kSessionAccount,
		QJsonDocument(object).toJson(QJsonDocument::Compact)
	) == Serein::CredentialError::None);
}

bool ClearCredentials() {
	auto &cache = Cache();
	cache.loaded = true;
	cache.value = Credentials();
	return (Serein::Ports::SecretStore::Remove(kSessionAccount)
		== Serein::CredentialError::None);
}

InstallIdentity LoadOrCreateInstallIdentity() {
	// Cached so an unavailable secret store cannot mint an identity per call.
	static auto identity = std::optional<InstallIdentity>();
	if (identity) {
		return *identity;
	}

	const auto result = Serein::Ports::SecretStore::Read(kInstallAccount);
	if (result.error == Serein::CredentialError::None) {
		const auto object = QJsonDocument::fromJson(result.secret).object();
		auto stored = InstallIdentity();
		stored.installId = object[u"id"_q].toString();
		stored.installSecret = object[u"secret"_q].toString();
		if (!stored.installId.isEmpty()
			&& stored.installSecret.size() == kSecretHexSize) {
			stored.persisted = true;
			identity = stored;
			return *identity;
		}
	}

	auto created = InstallIdentity();
	created.installId = QUuid::createUuid().toString(QUuid::WithoutBraces);
	created.installSecret = RandomSecretHex();
	const auto object = QJsonObject{
		{ u"id"_q, created.installId },
		{ u"secret"_q, created.installSecret },
	};
	created.persisted = (Serein::Ports::SecretStore::Write(
		kInstallAccount,
		QJsonDocument(object).toJson(QJsonDocument::Compact)
	) == Serein::CredentialError::None);
	identity = created;
	return *identity;
}

} // namespace VeyroGram
