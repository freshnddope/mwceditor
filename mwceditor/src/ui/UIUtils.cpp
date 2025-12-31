#include "UIUtils.h"
#include "UI.h"
#include "imgui.h"
#include <vector>
#include <string>

namespace UI {

    struct Document {
        std::string filepath;
        std::string name;
        SaveSystem::SaveFile file;
        State state;
    };

    void EditFloatVar(Document* doc, const char* label, const char* key, float min, float max) {
        if (!doc) return;

        auto* var = doc->file.GetVariable(key);
        if (!var) {
            ImGui::TextDisabled("%s: Not found", label);
            return;
        }

        if (var->header.valueType != SaveSystem::EntryValue::Float) {
            ImGui::TextDisabled("%s: Not a float", label);
            return;
        }

        float val = 0.0f;
        if (var->value.size() >= 4)
            memcpy(&val, var->value.data(), 4);

        bool changed = false;
        if (min == max)
            changed = ImGui::InputFloat(label, &val);
        else
            changed = ImGui::SliderFloat(label, &val, min, max);

        if (changed) {
            var->value.resize(4);
            memcpy(var->value.data(), &val, 4);
        }
    }

    void EditBoolVar(Document* doc, const char* label, const char* key) {
        if (!doc) return;

        auto* var = doc->file.GetVariable(key);
        if (!var) {
            ImGui::TextDisabled("%s: Not found", label);
            return;
        }

        if (var->header.valueType != SaveSystem::EntryValue::Bool) {
            ImGui::TextDisabled("%s: Not a bool", label);
            return;
        }

        bool val = (!var->value.empty() && var->value[0] == 1);
        if (ImGui::Checkbox(label, &val)) {
            if (var->value.empty())
                var->value.resize(1);
            var->value[0] = val ? 1 : 0;
        }
    }

    void EditIntVar(Document* doc, const char* label, const char* key, int min, int max) {
        if (!doc) return;

        auto* var = doc->file.GetVariable(key);
        if (!var) {
            ImGui::TextDisabled("%s: Not found", label);
            return;
        }

        if (var->header.valueType != SaveSystem::EntryValue::Integer) {
            ImGui::TextDisabled("%s: Not an int", label);
            return;
        }

        int val = 0;
        if (var->value.size() >= 4)
            memcpy(&val, var->value.data(), 4);

        bool changed = false;
        if (min == max)
            changed = ImGui::InputInt(label, &val);
        else
            changed = ImGui::SliderInt(label, &val, min, max);

        if (changed) {
            var->value.resize(4);
            memcpy(var->value.data(), &val, 4);
        }
    }

    void EditIntAsBool(Document* doc, const char* label, const char* key) {
        if (!doc) return;

        auto* var = doc->file.GetVariable(key);
        if (!var) {
            ImGui::TextDisabled("%s: Not found", label);
            return;
        }

        if (var->header.valueType != SaveSystem::EntryValue::Integer) {
            ImGui::TextDisabled("%s: Not an int", label);
            return;
        }

        int val = 0;
        if (var->value.size() >= 4)
            memcpy(&val, var->value.data(), 4);

        bool checked = (val != 0);
        if (ImGui::Checkbox(label, &checked)) {
            val = checked ? 1 : 0;
            var->value.resize(4);
            memcpy(var->value.data(), &val, 4);
        }
    }
}
