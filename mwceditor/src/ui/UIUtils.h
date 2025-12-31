#pragma once
#include "UI.h"
#include "imgui.h"

namespace UI {
    struct Document;

    void EditFloatVar(Document* doc, const char* label, const char* key, float min = 0.0f, float max = 0.0f);
    void EditBoolVar(Document* doc, const char* label, const char* key);
    void EditIntVar(Document* doc, const char* label, const char* key, int min = 0, int max = 0);
    void EditIntAsBool(Document* doc, const char* label, const char* key);
}
