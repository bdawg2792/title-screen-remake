// Flow Title - v0.1 "discovery build"
// Hooks the main menu (title screen) and logs its object hierarchy so the
// real restyle can be written against actual object names in 1.40.8_7379.
// Replace the template's src/main.cpp with this file.

#include "scotland2/shared/loader.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"
#include "paper2_scotland2/shared/logger.hpp"

#include "GlobalNamespace/MainMenuViewController.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/Vector3.hpp"

#include <algorithm>
#include <cctype>
#include <string>

static modloader::ModInfo modInfo{MOD_ID, VERSION, 0};
static constexpr auto PaperLogger = Paper::ConstLoggerContext("FlowTitle");

static std::string Lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

static std::string PathOf(UnityEngine::Transform* t) {
    std::string p = static_cast<std::string>(t->get_name());
    for (auto parent = t->get_parent(); parent; parent = parent->get_parent()) {
        p = static_cast<std::string>(parent->get_name()) + "/" + p;
    }
    return p;
}

// Logs a transform and everything under it (name, active state, position, scale).
static void DumpTree(UnityEngine::Transform* t, int depth) {
    if (!t) return;
    auto pos = t->get_localPosition();
    auto scl = t->get_localScale();
    PaperLogger.info("{}{} [active={}] pos=({:.2f},{:.2f},{:.2f}) scale=({:.2f},{:.2f},{:.2f})",
                     std::string(depth * 2, ' '),
                     static_cast<std::string>(t->get_name()),
                     t->get_gameObject()->get_activeSelf(),
                     pos.x, pos.y, pos.z, scl.x, scl.y, scl.z);
    for (int i = 0, n = t->get_childCount(); i < n; i++) {
        DumpTree(t->GetChild(i), depth + 1);
    }
}

// Finds the logo and background/environment objects (they live outside the view controller).
static void ScanSceneForBackground() {
    static const char* keys[] = {"logo", "environment", "star", "floor", "fog", "menu"};
    auto all = UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::Transform*>();
    int logged = 0;
    for (auto t : all) {
        if (!t) continue;
        std::string name = Lower(static_cast<std::string>(t->get_name()));
        for (auto key : keys) {
            if (name.find(key) != std::string::npos) {
                PaperLogger.info("MATCH '{}' -> {}", key, PathOf(t));
                logged++;
                break;
            }
        }
        if (logged >= 300) break;
    }
    PaperLogger.info("Scene scan done, {} matches logged", logged);
}

MAKE_HOOK_MATCH(MainMenu_DidActivate,
                &GlobalNamespace::MainMenuViewController::DidActivate,
                void,
                GlobalNamespace::MainMenuViewController* self,
                bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    MainMenu_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);

    if (firstActivation) {
        PaperLogger.info("=== FlowTitle: MainMenuViewController hierarchy ===");
        DumpTree(self->get_transform(), 0);
        PaperLogger.info("=== FlowTitle: scene scan (logo / background) ===");
        ScanSceneForBackground();
    }
}

MOD_EXPORT_FUNC void setup(CModInfo& info) {
    info.id = MOD_ID;
    info.version = VERSION;
    modInfo.assign(info);
    PaperLogger.info("FlowTitle setup");
}

MOD_EXPORT_FUNC void late_load() {
    il2cpp_functions::Init();
    INSTALL_HOOK(PaperLogger, MainMenu_DidActivate);
    PaperLogger.info("FlowTitle hooks installed");
}
