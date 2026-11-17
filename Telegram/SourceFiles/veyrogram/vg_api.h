/*
VeyroGram — commercial Telegram client
Based on SereinGram (GPL-3.0). VeyroGram extensions are also GPL-3.0.
*/
#pragma once

#include "base/basic_types.h"

#include <QtCore/QByteArray>
#include <QtCore/QJsonObject>
#include <QtCore/QString>

class QNetworkReply;

namespace VeyroGram {

struct ApiResponse {
	int status = 0;
	QByteArray body;
	QString error;

	[[nodiscard]] bool ok() const { return status >= 200 && status < 300; }
};

// POST JSON to /api/v1/<path> with optional Bearer token.
void ApiPost(
	const QString &path,
	const QJsonObject &json,
	const QString &token,
	Fn<void(ApiResponse)> done);

// GET /api/v1/<path> with Bearer token.
void ApiGet(
	const QString &path,
	const QString &token,
	Fn<void(ApiResponse)> done);

// POST /api/v1/<path> as SSE stream.
// onChunk is called for each SSE "data:" line.
// onDone is called when the stream ends (error is empty on success).
QNetworkReply *ApiStream(
	const QString &path,
	const QJsonObject &json,
	const QString &token,
	Fn<void(QString chunk)> onChunk,
	Fn<void(QString error)> onDone);

} // namespace VeyroGram
