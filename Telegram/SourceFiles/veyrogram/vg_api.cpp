/*
VeyroGram — commercial Telegram client
Based on SereinGram (GPL-3.0). VeyroGram extensions are also GPL-3.0.
*/
#include "veyrogram/vg_api.h"

#include "serein/adapters/qtnetwork/manager.h"

#include <QtCore/QJsonDocument>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

namespace VeyroGram {
namespace {

#ifndef VG_API_BASE_URL
#define VG_API_BASE_URL "https://api.veyrogram.com/api/v1"
#endif
const auto kBaseUrl = u"" VG_API_BASE_URL ""_q;
constexpr auto kTimeout = 30000; // 30 seconds
constexpr auto kRawLimit = 4096;

[[nodiscard]] QNetworkRequest MakeRequest(
		const QString &path,
		const QString &token) {
	const auto url = QUrl(kBaseUrl + path);
	auto request = QNetworkRequest(url);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	if (!token.isEmpty()) {
		request.setRawHeader(
			"Authorization",
			("Bearer " + token).toUtf8());
	}
	request.setTransferTimeout(kTimeout);
	return request;
}

[[nodiscard]] ApiResponse ParseReply(QNetworkReply *reply) {
	ApiResponse result;
	result.status = reply->attribute(
		QNetworkRequest::HttpStatusCodeAttribute).toInt();
	if (reply->error() != QNetworkReply::NoError
		&& result.status == 0) {
		result.error = reply->errorString();
		return result;
	}
	result.body = reply->readAll();
	return result;
}

} // namespace

void ApiPost(
		const QString &path,
		const QJsonObject &json,
		const QString &token,
		Fn<void(ApiResponse)> done) {
	auto &network = Serein::Adapters::SharedNetwork();
	const auto request = MakeRequest(path, token);
	const auto payload = QJsonDocument(json).toJson(QJsonDocument::Compact);
	const auto reply = network.post(request, payload);
	QObject::connect(reply, &QNetworkReply::finished, reply, [=] {
		reply->deleteLater();
		done(ParseReply(reply));
	});
}

void ApiGet(
		const QString &path,
		const QString &token,
		Fn<void(ApiResponse)> done) {
	auto &network = Serein::Adapters::SharedNetwork();
	const auto request = MakeRequest(path, token);
	const auto reply = network.get(request);
	QObject::connect(reply, &QNetworkReply::finished, reply, [=] {
		reply->deleteLater();
		done(ParseReply(reply));
	});
}

QNetworkReply *ApiStream(
		const QString &path,
		const QJsonObject &json,
		const QString &token,
		Fn<void(QString chunk)> onChunk,
		Fn<void(QString error)> onDone) {
	auto &network = Serein::Adapters::SharedNetwork();
	const auto request = MakeRequest(path, token);
	const auto payload = QJsonDocument(json).toJson(QJsonDocument::Compact);
	const auto reply = network.post(request, payload);

	// Accumulate partial SSE frames across readyRead signals.
	const auto accum = std::make_shared<QByteArray>();
	const auto raw = std::make_shared<QByteArray>();

	QObject::connect(reply, &QNetworkReply::readyRead, reply, [=] {
		const auto chunk = reply->readAll();
		if (raw->size() < kRawLimit) {
			raw->append(chunk);
		}
		accum->append(chunk);
		// Consume complete newline-terminated SSE lines.
		while (true) {
			const int nl = accum->indexOf('\n');
			if (nl < 0) break;
			const auto line = accum->left(nl).trimmed();
			accum->remove(0, nl + 1);
			if (line.startsWith("data:")) {
				const auto data = line.mid(5).trimmed();
				if (data == "[DONE]") break;
				if (!data.isEmpty()) {
					onChunk(QString::fromUtf8(data));
				}
			}
		}
	});

	QObject::connect(reply, &QNetworkReply::finished, reply, [=] {
		reply->deleteLater();
		if (reply->error() == QNetworkReply::NoError) {
			onDone(QString());
			return;
		}
		const auto status = reply->attribute(
			QNetworkRequest::HttpStatusCodeAttribute).toInt();
		raw->append(reply->readAll());
		const auto code = QJsonDocument::fromJson(*raw).object()
			[u"error"_q].toString();
		if (!code.isEmpty()) {
			onDone(code);
		} else if (status > 0) {
			onDone(u"HTTP %1"_q.arg(status));
		} else {
			onDone(reply->errorString());
		}
	});

	return reply;
}

} // namespace VeyroGram
