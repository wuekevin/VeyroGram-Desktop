/*
VeyroGram — commercial Telegram client
Based on SereinGram (GPL-3.0). VeyroGram extensions are also GPL-3.0.
*/
#include "veyrogram/vg_update.h"

#include <QtCore/QCryptographicHash>

// Ed25519 verification via OpenSSL EVP (available through Qt's bundled OpenSSL).
#include <openssl/evp.h>

namespace VeyroGram {
namespace {

[[nodiscard]] QByteArray FromHex(const QString &hex) {
    return QByteArray::fromHex(hex.toLatin1());
}

// Build the canonical message: sha256(version + ":" + url + ":" + hash)
[[nodiscard]] QByteArray ManifestDigest(
        const QString &version,
        const QString &downloadUrl,
        const QString &sha256Hash) {
    const auto msg = (version + u":" + downloadUrl + u":" + sha256Hash)
        .toUtf8();
    return QCryptographicHash::hash(msg, QCryptographicHash::Sha256);
}

} // namespace

bool VerifyUpdateManifest(
        const QString &version,
        const QString &downloadUrl,
        const QString &sha256Hash,
        const QString &signatureHex) {
    if (signatureHex.isEmpty()) {
        return false;
    }

    const auto pubKeyBytes = FromHex(QString::fromLatin1(VG_UPDATE_PUBLIC_KEY));
    const auto sigBytes    = FromHex(signatureHex);
    const auto digest      = ManifestDigest(version, downloadUrl, sha256Hash);

    if (pubKeyBytes.size() != 32 || sigBytes.size() != 64) {
        return false;
    }

    // Load raw Ed25519 public key into EVP_PKEY.
    EVP_PKEY *pkey = EVP_PKEY_new_raw_public_key(
        EVP_PKEY_ED25519,
        nullptr,
        reinterpret_cast<const unsigned char *>(pubKeyBytes.constData()),
        static_cast<size_t>(pubKeyBytes.size()));
    if (!pkey) {
        return false;
    }

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) {
        EVP_PKEY_free(pkey);
        return false;
    }

    // Ed25519 signs the message directly (no pre-hash in EVP).
    const bool ok =
        EVP_DigestVerifyInit(ctx, nullptr, nullptr, nullptr, pkey) == 1
        && EVP_DigestVerify(
               ctx,
               reinterpret_cast<const unsigned char *>(sigBytes.constData()),
               static_cast<size_t>(sigBytes.size()),
               reinterpret_cast<const unsigned char *>(digest.constData()),
               static_cast<size_t>(digest.size())) == 1;

    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    return ok;
}

} // namespace VeyroGram
