#pragma once
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <iostream>

namespace SaveSystem {

    namespace EntryValue {
        enum Type : uint32_t {
            Null = 0,
            Transform = 0x097AFA76,
            Float = 0x6E3ED76B,
            String = 0xFDE9F1EE,
            Bool = 0xAD4D7C9C,
            Color = 0x32CF4B31,
            Integer = 0xE2A80856,
            Vector3 = 0xEC66DC46,
            Unknown = 0xFFFFFFFF
        };
    }

    namespace EntryContainer {
        enum Type : uint8_t {
            Null = 0x00,
            NativeArray = 0x51,
            Dictionary = 0x52,
            List = 0x53,
            HashSet = 0x54,
            Queue = 0x55,
            Stack = 0x56,
            Unknown = 0xFF 
        };
    }

    struct Header {
        uint8_t containerType = EntryContainer::Null;
        uint32_t keyType = EntryValue::Null;
        uint32_t valueType = EntryValue::Null;
        std::string properties = "";
        uint32_t headerSize = 0; 
    };

    struct Variable {
        Header header;
        std::string key;
        std::string rawKey;
        std::string value;
        std::string rawValue;
        std::vector<std::pair<std::string, std::string>> entries; 
        std::string GetDisplayString() const; 
    };

    class SaveFile {
    public:
        std::filesystem::path currentPath;
        std::vector<Variable> variables;
        bool isLoaded = false;
        std::string error;

        bool Load(const std::filesystem::path& path);
        bool Save(const std::filesystem::path& path);
        Variable* GetVariable(const std::string& key);

        static const uint8_t HX_STARTENTRY = 0x7E;
        static const uint8_t HX_ENDENTRY = 0x7B;
    };

}
