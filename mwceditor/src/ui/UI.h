#pragma once
#include <string>
#include <map>
#include <vector>
#include "../system/SaveSystem.h"

namespace UI {
    void ApplyStyle();
    void Render();
    
    struct State {
        char searchBuffer[128] = "";
        std::string statusMessage = "Ready";
        std::string currentVersion = "1.0.1";
        bool showDemoWindow = false;
        bool showToolsWindow = false;
        int selectedItem = -1;
    };

    State& GetState();
}
