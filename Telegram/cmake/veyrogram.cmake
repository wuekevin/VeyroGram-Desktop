set(veyrogram_sources
    veyrogram/vg_credentials.cpp
    veyrogram/vg_api.cpp
    veyrogram/vg_auth.cpp
    veyrogram/vg_translate.cpp
    veyrogram/vg_settings.cpp
    veyrogram/vg_menu.cpp
)

if (veyrogram_sources)
    nice_target_sources(Telegram ${src_loc} PRIVATE ${veyrogram_sources})
endif()
