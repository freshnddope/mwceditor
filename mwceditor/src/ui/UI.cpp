#include "UI.h"
#include "UIUtils.h"
#include "imgui.h"
#include <algorithm>
#include <Windows.h>
#include <string>
#include <cmath>
#include <vector>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <shellapi.h>

namespace UI {

    struct Document {
        std::string filepath;
        std::string name;
        SaveSystem::SaveFile file;
        State state;
    };

    static std::vector<Document> documents;
    static std::string globalStatus = "Ready";
    static int activeTabIndex = -1;
    static bool forceTabSelection = false;
    static bool showToolsWindow = false;
    static bool showHelpWindow = false;
    static State globalState;

    State& GetState() {
        return globalState;
    }

    int FindDocumentByPath(const std::string& path) {
        for (size_t i = 0; i < documents.size(); ++i)
            if (documents[i].filepath == path)
                return static_cast<int>(i);
        return -1;
    }

    Document* GetActiveDocument() {
        if (activeTabIndex >= 0 && activeTabIndex < (int)documents.size())
            return &documents[activeTabIndex];
        return nullptr;
    }

    void ApplyStyle() {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;
        colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg]               = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg]                = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
        colors[ImGuiCol_Border]                 = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
        colors[ImGuiCol_FrameBg]                = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
        colors[ImGuiCol_TitleBg]                = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_CheckMark]              = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
        colors[ImGuiCol_Button]                 = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
        colors[ImGuiCol_ButtonHovered]          = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
        colors[ImGuiCol_ButtonActive]           = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
        colors[ImGuiCol_Header]                 = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_HeaderHovered]          = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
        colors[ImGuiCol_HeaderActive]           = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
        colors[ImGuiCol_Separator]              = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
        colors[ImGuiCol_ResizeGrip]             = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
        colors[ImGuiCol_Tab]                    = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_TabHovered]             = ImVec4(0.14f, 0.14f, 0.14f, 0.80f);
        colors[ImGuiCol_TabActive]              = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);

        style.WindowRounding = 6.0f;
        style.FrameRounding = 3.0f;
    }

    void RenderValueEditor(const char* label, std::string& buffer, uint32_t type) {
        ImGui::PushID(label);
        switch (type) {
            case SaveSystem::EntryValue::Bool: {
                bool b = (buffer.size() > 0 && buffer[0] == 1);
                if (ImGui::Checkbox("##bool", &b)) {
                    if (buffer.empty()) buffer.resize(1);
                    buffer[0] = b ? 1 : 0;
                }
                break;
            }
            case SaveSystem::EntryValue::Integer: {
                 int val = 0;
                 if (buffer.size() >= 4) val = *reinterpret_cast<const int*>(buffer.data());
                 if (ImGui::InputInt("##int", &val)) {
                     if (buffer.size() < 4) buffer.resize(4);
                     memcpy(buffer.data(), &val, 4);
                 }
                 break;
            }
            case SaveSystem::EntryValue::Float: {
                 float val = 0.0f;
                 if (buffer.size() >= 4) val = *reinterpret_cast<const float*>(buffer.data());
                 if (ImGui::InputFloat("##float", &val)) {
                     if (buffer.size() < 4) buffer.resize(4);
                     memcpy(buffer.data(), &val, 4);
                 }
                 break;
            }
            case SaveSystem::EntryValue::String: {
                 char buf[1024] = "";
                 if (buffer.size() < 1024) strncpy_s(buf, buffer.c_str(), buffer.size());
                 if (ImGui::InputText("##str", buf, 1024))
                     buffer = std::string(buf);
                 break;
            }
            case SaveSystem::EntryValue::Vector3: {
                 float vec[3] = {0.0f, 0.0f, 0.0f};
                 if (buffer.size() >= 12) memcpy(vec, buffer.data(), 12);
                 if (ImGui::DragFloat3("##vec3", vec, 0.1f)) {
                     if (buffer.size() < 12) buffer.resize(12);
                     memcpy(buffer.data(), vec, 12);
                 }
                 break;
            }
             case SaveSystem::EntryValue::Transform: {
                 if (buffer.size() >= 12) {
                     int numFloats = (int)buffer.size() / 4;
                     if (numFloats >= 3) {
                         float* rawFloats = reinterpret_cast<float*>(buffer.data());
                         std::vector<float> vec(rawFloats, rawFloats + numFloats);
                         
                         bool changed = false;
                         if (numFloats == 3) changed = ImGui::DragFloat3("Pos", vec.data(), 0.1f);
                         else if (numFloats == 4) changed = ImGui::DragFloat4("Quat", vec.data(), 0.01f);
                         else if (numFloats == 7) {
                             changed |= ImGui::DragFloat3("Pos", vec.data(), 0.1f);
                             changed |= ImGui::DragFloat4("Rot", vec.data() + 3, 0.01f);
                         } else
                             ImGui::Text("Raw Floats [%d]", numFloats);

                         if (changed)
                              memcpy(buffer.data(), vec.data(), buffer.size());
                     }
                 } else {
                     ImGui::TextDisabled("Empty Transform");
                 }
                 break;
             }
             case SaveSystem::EntryValue::Color: {
                 float col[4] = {0.0f, 0.0f, 0.0f, 1.0f};
                 if (buffer.size() >= 16) memcpy(col, buffer.data(), 16);
                 if (ImGui::ColorEdit4("##color", col)) {
                     if (buffer.size() < 16) buffer.resize(16);
                     memcpy(buffer.data(), col, 16);
                 }
                 break;
            }
            default:
                 ImGui::TextDisabled("Binary Data (%zu bytes)", buffer.size());
                 break;
        }
        ImGui::PopID();
    }

    void RenderToolsWindow() {
        if (!showToolsWindow)
            return;

        Document* doc = GetActiveDocument();
        if (!doc)
            return;

        if (!ImGui::Begin("Tools", &showToolsWindow)) {
            ImGui::End();
            return;
        }

        if (ImGui::CollapsingHeader("Player", ImGuiTreeNodeFlags_DefaultOpen)) {
            EditFloatVar(doc, "Set Player Money", "PlayerMoney");
            EditFloatVar(doc, "Set Player Bank Money", "PlayerMoneyBank");
            EditIntVar(doc, "Set Cigarettes", "PlayerCigarettes", 0, 20);
            EditBoolVar(doc, "Player Tattoo", "PlayerTattoo");
        }

        ImGui::Separator();

        if (ImGui::CollapsingHeader("Misc", ImGuiTreeNodeFlags_DefaultOpen)) {
            EditFloatVar(doc, "Fuel Oil Price", "FuelPriceFuelOil");
            EditFloatVar(doc, "Fuel Diesel Price", "FuelPriceDiesel");
            EditFloatVar(doc, "Fuel Gasoline Price", "FuelPriceGasoline");

            ImGui::Separator();

            if (ImGui::CollapsingHeader("Keys")) {
                EditIntAsBool(doc, "Satsuma Key", "KeySatsuma");
                EditIntAsBool(doc, "Home Key", "KeyHome");
                EditIntAsBool(doc, "Ferndale Key", "KeyFerndale");
                EditBoolVar(doc, "Gifu Available", "GifuAvailable");
            }
        }

        ImGui::End();
    }

    void RenderHelpWindow() {
        if (!showHelpWindow) return;

        ImGui::Begin("Help", &showHelpWindow, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::TextWrapped("Welcome to the MWC Save Editor!");
        ImGui::Separator();

        if (ImGui::CollapsingHeader("How to open save file", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::BulletText("Go to File -> Open Save...");
            ImGui::BulletText("Select your save file from the game directory");
            ImGui::BulletText("The file will open in a new tab");
        }

        if (ImGui::CollapsingHeader("Editing values")) {
            ImGui::BulletText("Use search bar to find variables quickly");
            ImGui::BulletText("Click on values to edit them");
            ImGui::BulletText("Changes are applied instantly in memory");
            ImGui::BulletText("Remember to save the file!");
        }

        if (ImGui::CollapsingHeader("Containers (List / Dictionary)")) {
            ImGui::BulletText("Lists and dictionaries can be expanded");
            ImGui::BulletText("Use '+' button to add new entries");
            ImGui::BulletText("Use 'x' button to remove entries");
        }

        if (ImGui::CollapsingHeader("Restore points")) {
            ImGui::BulletText("Create restore points from Restore menu");
            ImGui::BulletText("All save files are backed up");
            ImGui::BulletText("You can restore previous state anytime");
        }

        if (ImGui::CollapsingHeader("Tools Window")) {
            ImGui::BulletText("Tools window allows quick edits");
            ImGui::BulletText("Requires savefile.txt to be loaded");
            ImGui::BulletText("Useful for money, fuel prices, etc.");
        }

        ImGui::Separator();
        ImGui::TextDisabled("MWC Editor by polaris");

        ImGui::End();
    }


    void Render() {
        static bool opt_fullscreen = true;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        if (opt_fullscreen) {
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }

        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace", nullptr, window_flags);
        ImGui::PopStyleVar();

        if (opt_fullscreen)
            ImGui::PopStyleVar(2);

        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }

        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Open Save...")) {
                     char filename[MAX_PATH] = "";
                     char initialDir[MAX_PATH] = "";
                     if (GetEnvironmentVariableA("USERPROFILE", initialDir, MAX_PATH))
                         strcat_s(initialDir, "\\AppData\\LocalLow\\Amistech\\My Winter Car");

                     OPENFILENAMEA ofn;
                     ZeroMemory(&ofn, sizeof(ofn));
                     ofn.lStructSize = sizeof(ofn);
                     ofn.hwndOwner = NULL; 
                     ofn.lpstrFilter = "All Files\0*.*\0";
                     ofn.lpstrFile = filename;
                     ofn.nMaxFile = MAX_PATH;
                     ofn.lpstrInitialDir = initialDir;
                     ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
                     
                     if (GetOpenFileNameA(&ofn)) {
                         int existingIdx = FindDocumentByPath(filename);
                         if (existingIdx >= 0) {
                             globalStatus = "File already open, switching to tab: " + documents[existingIdx].name;
                             activeTabIndex = existingIdx;
                             forceTabSelection = true;
                         } else {
                             Document newDoc;
                             newDoc.filepath = filename;
                             std::filesystem::path p(filename);
                             newDoc.name = p.filename().string();
                             
                             if (newDoc.file.Load(filename)) {
                                 globalStatus = "Loaded: " + newDoc.name;
                                 documents.push_back(newDoc);
                                 activeTabIndex = (int)documents.size() - 1;
                                 forceTabSelection = true;
                             } else
                                 globalStatus = "Error: " + newDoc.file.error;
                         }
                     }
                }
                
                if (ImGui::MenuItem("Open in Explorer")) {
                     char initialDir[MAX_PATH] = "";
                     if (GetEnvironmentVariableA("USERPROFILE", initialDir, MAX_PATH)) {
                         strcat_s(initialDir, "\\AppData\\LocalLow\\Amistech\\My Winter Car");
                         ShellExecuteA(NULL, "open", initialDir, NULL, NULL, SW_SHOW);
                     }
                }
                
                ImGui::Separator();
                if (ImGui::MenuItem("Save All", nullptr, false, !documents.empty())) {
                    int savedCount = 0;
                    int failedCount = 0;
                    for (auto& doc : documents) {
                        if (doc.file.isLoaded) {
                            if (doc.file.Save(doc.filepath))
                                savedCount++;
                            else
                                failedCount++;
                        }
                    }
                    if (failedCount > 0)
                        globalStatus = "Saved " + std::to_string(savedCount) + " files, " + std::to_string(failedCount) + " failed";
                    else
                        globalStatus = "Saved " + std::to_string(savedCount) + " files successfully";
                }
                
                ImGui::Separator();
                if (ImGui::MenuItem("Exit"))
                    PostQuitMessage(0);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Tools")) {
                if (ImGui::MenuItem("Open Tools Window")) {
                    showToolsWindow = true;
                    
                    char savefilePath[MAX_PATH] = "";
                    if (GetEnvironmentVariableA("USERPROFILE", savefilePath, MAX_PATH)) {
                        strcat_s(savefilePath, "\\AppData\\LocalLow\\Amistech\\My Winter Car\\savefile.txt");
                        int existingIdx = FindDocumentByPath(savefilePath);
                        if (existingIdx >= 0) {
                            activeTabIndex = existingIdx;
                            forceTabSelection = true;
                        } else if (std::filesystem::exists(savefilePath)) {
                            Document newDoc;
                            newDoc.filepath = savefilePath;
                            newDoc.name = "savefile.txt";
                            if (newDoc.file.Load(savefilePath)) {
                                documents.push_back(newDoc);
                                activeTabIndex = (int)documents.size() - 1;
                                forceTabSelection = true;
                                globalStatus = "Automatically loaded savefile.txt";
                            }
                        }
                    }
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Restore")) {
                bool hasDoc = !documents.empty();
                
                if (ImGui::MenuItem("Create Restore Point", nullptr, false, hasDoc)) {
                     auto& doc = documents[0];
                     std::filesystem::path currentFile(doc.filepath);
                     std::filesystem::path saveDir = currentFile.parent_path();
                     std::filesystem::path backupRoot = saveDir / "backup";
                     
                     if (!std::filesystem::exists(backupRoot))
                         std::filesystem::create_directory(backupRoot);

                    auto now = std::chrono::system_clock::now();
                    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
                    std::stringstream ss;
                    ss << std::put_time(std::localtime(&now_c), "restore_%Y-%m-%d_%H-%M-%S");
                    
                    std::filesystem::path destFolder = backupRoot / ss.str();
                    
                    try {
                        std::filesystem::create_directory(destFolder);
                        int copyCount = 0;
                        for (const auto& entry : std::filesystem::directory_iterator(saveDir)) {
                            if (entry.is_regular_file()) {
                                std::filesystem::copy_file(entry.path(), destFolder / entry.path().filename());
                                copyCount++;
                            }
                        }
                        globalStatus = "Restore point created: " + ss.str() + " (" + std::to_string(copyCount) + " files)";
                    } catch (...) {
                        globalStatus = "Failed to create restore point";
                    }
                }
                
                ImGui::Separator();
                
                if (ImGui::BeginMenu("Load Restore Point", hasDoc)) {
                    auto& doc = documents[0];
                    std::filesystem::path currentFile(doc.filepath);
                    std::filesystem::path saveDir = currentFile.parent_path();
                    std::filesystem::path backupDir = saveDir / "backup";
                    
                    if (std::filesystem::exists(backupDir)) {
                         static std::vector<std::filesystem::directory_entry> restores;
                         restores.clear();
                         for (const auto& entry : std::filesystem::directory_iterator(backupDir))
                             if (entry.is_directory()) restores.push_back(entry);
                         std::sort(restores.begin(), restores.end(), [](const auto& a, const auto& b) {
                              return a.last_write_time() > b.last_write_time();
                         });
                         
                         int count = 0;
                         for (const auto& entry : restores) {
                             std::string name = entry.path().filename().string();
                             if (ImGui::MenuItem(name.c_str())) {
                                  try {
                                      int restored = 0;
                                      for (const auto& f : std::filesystem::directory_iterator(entry.path())) {
                                          if (f.is_regular_file()) {
                                              std::filesystem::copy_file(f.path(), saveDir / f.path().filename(), std::filesystem::copy_options::overwrite_existing);
                                              restored++;
                                          }
                                      }
                                      
                                      for (auto& d : documents)
                                          d.file.Load(d.filepath);
                                      globalStatus = "Restored " + std::to_string(restored) + " files from " + name;
                                  } catch (...) {
                                      globalStatus = "Restore failed";
                                  }
                             }
                             if (++count > 15) break;
                         }
                         
                         if (count == 0) ImGui::TextDisabled("(No restore points)");
                    } else
                         ImGui::TextDisabled("No backup folder found");
                    ImGui::EndMenu();
                }
                
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem("Open Help Window"))
                    showHelpWindow = true;
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        ImGui::Begin("Editor", nullptr);
        
        if (documents.empty())
            ImGui::TextDisabled("No file loaded. File -> Open Save...");
        else {
            if (ImGui::BeginTabBar("FileTabs", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_FittingPolicyScroll)) {
                for (int i = 0; i < documents.size();) {
                    auto& doc = documents[i];
                    bool keepOpen = true;
                    
                    ImGuiTabItemFlags flags = ImGuiTabItemFlags_None;
                    if (forceTabSelection && activeTabIndex == i)
                        flags |= ImGuiTabItemFlags_SetSelected;
                    
                    if (ImGui::BeginTabItem(doc.name.c_str(), &keepOpen, flags)) {
                        activeTabIndex = i;
                        if (ImGui::Button("Save")) {
                             if (doc.file.Save(doc.filepath))
                                 globalStatus = "Saved " + doc.name;
                             else
                                 globalStatus = "Error: " + doc.file.error;
                        }
                        ImGui::SameLine();
                        ImGui::Text("|");
                        ImGui::SameLine();
                        
                        if (!doc.file.isLoaded)
                            ImGui::TextColored(ImVec4(1,0,0,1), "File not loaded");
                        else {
                            ImGui::InputTextWithHint("##Search", "Search...", doc.state.searchBuffer, IM_ARRAYSIZE(doc.state.searchBuffer));
                            ImGui::SameLine();
                            ImGui::Text("Items: %zu", doc.file.variables.size());
                            ImGui::Separator();

                            if (ImGui::BeginTable("Variables", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY)) {
                                ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                                ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 200.0f);
                                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
                                ImGui::TableHeadersRow();

                                std::string search = doc.state.searchBuffer;
                                std::transform(search.begin(), search.end(), search.begin(), ::tolower);

                                std::vector<size_t> filteredIndices;
                                filteredIndices.reserve(doc.file.variables.size());
                                for (size_t j = 0; j < doc.file.variables.size(); ++j) {
                                     if (search.empty())
                                         filteredIndices.push_back(j);
                                     else {
                                         std::string keyLower = doc.file.variables[j].key;
                                         std::transform(keyLower.begin(), keyLower.end(), keyLower.begin(), ::tolower);
                                         if (keyLower.find(search) != std::string::npos)
                                             filteredIndices.push_back(j);
                                     }
                                }

                                ImGuiListClipper clipper;
                                clipper.Begin((int)filteredIndices.size());
                                
                                while (clipper.Step()) {
                                    for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
                                        size_t j = filteredIndices[row];
                                        auto& var = doc.file.variables[j];

                                        ImGui::PushID((int)j); 
                                        ImGui::TableNextRow();
                                        
                                        ImGui::TableSetColumnIndex(0);
                                        const char* typeName = "Data";
                                        switch(var.header.valueType) {
                                            case SaveSystem::EntryValue::Bool: typeName = "Bool"; break;
                                            case SaveSystem::EntryValue::Integer: typeName = "Int"; break;
                                            case SaveSystem::EntryValue::Float: typeName = "Float"; break;
                                            case SaveSystem::EntryValue::String: typeName = "String"; break;
                                            case SaveSystem::EntryValue::Transform: typeName = "Trans"; break;
                                            case SaveSystem::EntryValue::Vector3: typeName = "Vec3"; break;
                                            case SaveSystem::EntryValue::Color: typeName = "Color"; break;
                                            default: typeName = "Data"; break;
                                        }
                                        if (var.header.containerType != SaveSystem::EntryContainer::Null) {
                                             if (var.header.containerType == SaveSystem::EntryContainer::List) typeName = "List";
                                             else if (var.header.containerType == SaveSystem::EntryContainer::Dictionary) typeName = "Dict";
                                             else if (var.header.containerType == SaveSystem::EntryContainer::NativeArray) typeName = "Array";
                                             else typeName = "Container";
                                        }
                                        ImGui::TextDisabled("%s", typeName);

                                        ImGui::TableSetColumnIndex(1);
                                        bool isContainer = (var.header.containerType != SaveSystem::EntryContainer::Null);
                                        
                                        if (isContainer) {
                                             std::string label = var.key + " [" + std::to_string(var.entries.size()) + "]";
                                             bool open = ImGui::TreeNode(label.c_str());
                                             
                                             ImGui::SameLine();
                                             if (ImGui::SmallButton("+")) {
                                                 var.entries.push_back({"", ""}); 
                                                 std::string defVal = "";
                                                 if (var.header.valueType == SaveSystem::EntryValue::Integer || var.header.valueType == SaveSystem::EntryValue::Float) defVal.resize(4, 0);
                                                 if (var.header.valueType == SaveSystem::EntryValue::Bool) defVal.resize(1, 0);
                                                 if (var.header.valueType == SaveSystem::EntryValue::Vector3) defVal.resize(12, 0);
                                                 if (var.header.valueType == SaveSystem::EntryValue::Color) defVal.resize(16, 0);
                                                 var.entries.back().second = defVal;
                                                 
                                                 if (var.header.containerType == SaveSystem::EntryContainer::Dictionary) {
                                                     std::string defKey = "";
                                                     if (var.header.keyType == SaveSystem::EntryValue::Integer || var.header.keyType == SaveSystem::EntryValue::Float) defKey.resize(4, 0);
                                                     if (var.header.keyType == SaveSystem::EntryValue::String) defKey = "NewKey"; 
                                                     var.entries.back().first = defKey;
                                                 }
                                             }

                                             ImGui::TableSetColumnIndex(2);
                                             ImGui::TextDisabled("Container with %zu items", var.entries.size());
                                             
                                             if (open) {
                                                 for (size_t k = 0; k < var.entries.size(); ++k) {
                                                     ImGui::TableNextRow();
                                                     ImGui::TableSetColumnIndex(1);
                                                     ImGui::PushID((int)k);
                                                     
                                                     if (ImGui::SmallButton("X")) {
                                                         var.entries.erase(var.entries.begin() + k);
                                                         ImGui::PopID();
                                                         k--;
                                                         continue;
                                                     }
                                                     ImGui::SameLine();

                                                     ImGui::Indent();
                                                     if (var.header.containerType == SaveSystem::EntryContainer::Dictionary)
                                                          RenderValueEditor("##key", var.entries[k].first, var.header.keyType);
                                                     else
                                                          ImGui::Text("[%zu]", k);
                                                     ImGui::Unindent();
                                                     
                                                     ImGui::TableSetColumnIndex(2);
                                                     RenderValueEditor("##val", var.entries[k].second, var.header.valueType);
                                                     
                                                     ImGui::PopID();
                                                 }
                                                 ImGui::TreePop(); 
                                             }
                                        } else {
                                            ImGui::TextUnformatted(var.key.c_str());
                                            ImGui::TableSetColumnIndex(2);
                                            ImGui::SetNextItemWidth(-FLT_MIN);
                                            RenderValueEditor("##scalar", var.value, var.header.valueType);
                                        }
                                        
                                        ImGui::PopID();
                                    }
                                }
                                ImGui::EndTable();
                            }
                        }
                        
                        ImGui::EndTabItem();
                    }
                    
                    if (!keepOpen)
                        documents.erase(documents.begin() + i);
                    else
                        i++;
                }
                ImGui::EndTabBar();
                forceTabSelection = false;
            }
        }
        
        ImGui::End();

        RenderToolsWindow();
        RenderHelpWindow();

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 workPos = viewport->WorkPos;
        ImVec2 workSize = viewport->WorkSize;
        float padding = 10.0f;
        float windowHeight = 30.0f;

        const char* githubLabel = "MWC Editor @ github.com/freshnddope/mwceditor";
        float githubWidth = ImGui::CalcTextSize(githubLabel).x + 20.0f;
        
        float statusWidth = ImGui::CalcTextSize(globalStatus.c_str()).x + 20.0f;
        if (statusWidth < 150.0f) statusWidth = 150.0f;

        ImVec2 statusPos;
        statusPos.x = workPos.x + workSize.x - statusWidth - padding;
        statusPos.y = workPos.y + workSize.y - windowHeight - padding;

        ImVec2 githubPos;
        githubPos.x = statusPos.x - githubWidth - padding;
        githubPos.y = statusPos.y;

        float time = (float)ImGui::GetTime();
        float alpha = (sinf(time * 2.0f) * 0.5f + 0.5f) * 0.5f + 0.5f;

        ImGui::SetNextWindowPos(githubPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(githubWidth, windowHeight));
        ImGui::Begin("GithubOverlay", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration);
        ImGui::SetCursorPosY((windowHeight - ImGui::GetTextLineHeight()) * 0.5f);
        ImGui::SetCursorPosX(10.0f);
        ImGui::TextColored(ImVec4(0.33f, 0.67f, 0.86f, alpha), "%s", githubLabel);
        ImGui::End();

        ImGui::SetNextWindowPos(statusPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(statusWidth, windowHeight));
        ImGui::Begin("StatusOverlay", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration);
        ImGui::SetCursorPosY((windowHeight - ImGui::GetTextLineHeight()) * 0.5f);
        ImGui::SetCursorPosX(10.0f);
        ImGui::TextUnformatted(globalStatus.c_str());
        ImGui::End();
        
        ImGui::End();
    }
}
