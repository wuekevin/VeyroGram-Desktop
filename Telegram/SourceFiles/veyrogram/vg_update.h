/*
VeyroGram — commercial Telegram client
Based on SereinGram (GPL-3.0). VeyroGram extensions are also GPL-3.0.
*/
#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QString>

namespace VeyroGram {

// Ed25519 public key embedded at build time (64 hex chars = 32 bytes).
// Set via cmake -DVG_UPDATE_PUBLIC_KEY=<hex> or defaults to the production key.
#ifndef VG_UPDATE_PUBLIC_KEY
#define VG_UPDATE_PUBLIC_KEY \
    "eb07c2b6e61db779d7daa9d9bd142277c56b1ef6b48ba162706dcca3bc92c527"
#endif

[[nodiscard]] inline QByteArray UpdatePublicKeyHex() {
    return QByteArray(VG_UPDATE_PUBLIC_KEY);
}

// Verify an update manifest signature.
//
// The server signs:  sha256("<version>:<downloadURL>:<sha256Hash>")
// with ED25519_PRIVATE_KEY_HEX and stores the hex signature in
// manifest_signature.
//
// Returns true if the signature is valid for the embedded public key.
// Returns false if the signature is invalid or the public key is wrong.
//
// NOTE: Qt does not ship Ed25519 primitives. This function requires
// linking a small implementation (see vg_update.cpp).
[[nodiscard]] bool VerifyUpdateManifest(
    const QString &version,
    const QString &downloadUrl,
    const QString &sha256Hash,
    const QString &signatureHex);

} // namespace VeyroGram
