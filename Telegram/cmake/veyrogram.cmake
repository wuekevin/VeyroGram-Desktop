set(veyrogram_sources
    veyrogram/vg_credentials.cpp
    veyrogram/vg_api.cpp
    veyrogram/vg_session.cpp
    veyrogram/vg_translate.cpp
    veyrogram/vg_settings.cpp
    veyrogram/vg_menu.cpp
    veyrogram/vg_update.cpp
)

if (veyrogram_sources)
    nice_target_sources(Telegram ${src_loc} PRIVATE ${veyrogram_sources})
endif()

# --------------------------------------------------------------------------
# VeyroGram build-time constants
# Override at cmake time:
#   cmake -DVG_API_BASE_URL="https://api.example.com/api/v1" ...
#   cmake -DVG_UPDATE_PUBLIC_KEY="<64-hex-ed25519-pubkey>" ...
# --------------------------------------------------------------------------
if (NOT DEFINED VG_API_BASE_URL)
    set(VG_API_BASE_URL "https://api.veyrogram.com/api/v1")
endif()

if (NOT DEFINED VG_UPDATE_PUBLIC_KEY)
    # Production Ed25519 public key (derived from ED25519_PRIVATE_KEY_HEX via
    # `go run ./cmd/tools/pubkey` in the backend repo).
    set(VG_UPDATE_PUBLIC_KEY "eb07c2b6e61db779d7daa9d9bd142277c56b1ef6b48ba162706dcca3bc92c527")
endif()

target_compile_definitions(Telegram PRIVATE
    VG_API_BASE_URL="${VG_API_BASE_URL}"
    VG_UPDATE_PUBLIC_KEY="${VG_UPDATE_PUBLIC_KEY}"
)
