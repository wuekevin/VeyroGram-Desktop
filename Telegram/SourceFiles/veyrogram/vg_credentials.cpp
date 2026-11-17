/*
VeyroGram — commercial Telegram client
Based on SereinGram (GPL-3.0). VeyroGram extensions are also GPL-3.0.
*/
#include "veyrogram/vg_credentials.h"

#include <QtCore/QDateTime>
#include <QtCore/QDataStream>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>

namespace VeyroGram {
namespace {

constexpr quint32 kMagic = 0x56475345; // "VGSE"
constexpr quint32 kVersion = 1;

// 32-byte XOR key for simple obfuscation (not crypto-secure).
// Prevents casual reading of the session file; tokens are still
// retrievable from process memory.
constexpr unsigned char kXorKey[32] = {
	0x4B, 0x2A, 0x9F, 0x13, 0xE7, 0x5C, 0x81, 0x3D,
	0xB0, 0x67, 0x24, 0xC9, 0x52, 0x0E, 0xAA, 0x76,
	0x1F, 0x93, 0x45, 0xD8, 0x6B, 0x20, 0xF4, 0x87,
	0x3C, 0xA1, 0x59, 0xE2, 0x0D, 0x74, 0xC8, 0x36,
};

[[nodiscard]] QByteArray Obfuscate(const QByteArray &data) {
	QByteArray out = data;
	for (int i = 0; i < out.size(); ++i) {
		out[i] = out[i] ^ static_cast<char>(kXorKey[i % 32]);
	}
	return out;
}

[[nodiscard]] QString SessionFilePath() {
	const auto base = QStandardPaths::writableLocation(
		QStandardPaths::AppDataLocation);
	return base + u"/VeyroGram/vg_session.dat"_q;
}

} // namespace

bool Credentials::valid() const {
	return !accessToken.isEmpty() && !email.isEmpty();
}

bool Credentials::expired() const {
	if (expiresAt <= 0) {
		return false;
	}
	const auto now = QDateTime::currentSecsSinceEpoch();
	return now >= expiresAt - 60; // 60-second buffer
}

Credentials LoadCredentials() {
	const auto path = SessionFilePath();
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		return {};
	}
	const auto raw = Obfuscate(file.readAll());
	file.close();

	QDataStream ds(raw);
	ds.setByteOrder(QDataStream::BigEndian);

	quint32 magic = 0, version = 0;
	ds >> magic >> version;
	if (magic != kMagic || version != kVersion) {
		return {};
	}

	Credentials c;
	ds >> c.email >> c.accessToken >> c.refreshToken >> c.expiresAt;
	if (ds.status() != QDataStream::Ok) {
		return {};
	}
	return c;
}

void SaveCredentials(const Credentials &creds) {
	QByteArray raw;
	QDataStream ds(&raw, QIODevice::WriteOnly);
	ds.setByteOrder(QDataStream::BigEndian);
	ds << kMagic << kVersion;
	ds << creds.email << creds.accessToken << creds.refreshToken << creds.expiresAt;

	const auto path = SessionFilePath();
	QDir().mkpath(QFileInfo(path).absolutePath());
	QFile file(path);
	if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		file.write(Obfuscate(raw));
	}
}

void ClearCredentials() {
	QFile::remove(SessionFilePath());
}

bool IsLoggedIn() {
	const auto c = LoadCredentials();
	return c.valid() && !c.expired();
}

} // namespace VeyroGram
