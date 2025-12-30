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
        bool showDemoWindow = false;
        bool showToolsWindow = false;
        int selectedItem = -1;
    };
}
