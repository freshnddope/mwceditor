#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "SaveSystem.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <iomanip>

namespace SaveSystem {

    static std::string WStringToString(const std::wstring& wstr) {
        if (wstr.empty()) return "";
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
        std::string strTo(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
        return strTo;
    }
    
    static std::string SanitizeKey(const std::string& raw) {
        std::string out = "";
        for (char c : raw)
            if (isalnum(c) || c == '_' || c == '.' || c == '-')
                out += c;
        return out;
    }

    Header ParseHeader(const std::string& body, uint32_t& bytesRead) {
        Header h;
        if (body.size() < 1) {
            bytesRead = 0;
            return h;
        }

        size_t offset = 1;
        
        if (body[0] != (char)-1) {
            h.containerType = (uint8_t)body[0];
            offset++; 
        } else h.containerType = EntryContainer::Null;

        if (h.containerType != EntryContainer::Null) {
             if (h.containerType == EntryContainer::Dictionary) {
                 if (offset + 4 > body.size()) {
                     bytesRead = offset;
                     return h;
                 }
                 h.keyType = *reinterpret_cast<const uint32_t*>(&body[offset]);
                 offset += 4;
             }
             
             if (offset + 4 > body.size()) {
                 bytesRead = offset;
                 return h;
             }
             h.valueType = *reinterpret_cast<const uint32_t*>(&body[offset]);
             offset += 4;

             uint32_t propSize = (h.keyType == EntryValue::Null) ? 1 : 2;
             if (offset + propSize <= body.size()) {
                 h.properties = body.substr(offset, propSize);
                 offset += propSize;
             }
        } else {
             if (offset + 4 <= body.size()) {
                h.valueType = *reinterpret_cast<const uint32_t*>(&body[offset]);
                offset += 4;
             }
        }
        
        h.headerSize = static_cast<uint32_t>(offset);
        bytesRead = h.headerSize;
        return h;
    }

    std::string Variable::GetDisplayString() const {
        if (value.empty()) return "";

        if (header.containerType == EntryContainer::Null) {
            if (header.valueType == EntryValue::String)
                return value;
            if (header.valueType == EntryValue::Bool)
                return (value[0] == 1) ? "True" : "False";
            if (header.valueType == EntryValue::Integer) {
                if (value.size() >= 4) {
                    int val = *reinterpret_cast<const int*>(value.data());
                    return std::to_string(val);
                }
            }
            if (header.valueType == EntryValue::Float) {
                if (value.size() >= 4) {
                    float val = *reinterpret_cast<const float*>(value.data());
                    return std::to_string(val);
                }
            }
            if (header.valueType == EntryValue::Transform)
                 return "Transform (Binary)";
            if (header.valueType == EntryValue::Vector3)
                 return "Vector3 (Binary)";
        }
        return "Complex Data";
    }

    static std::string ReadValue(const std::string& buffer, size_t& offset, uint32_t type) {
        if (offset >= buffer.size()) return "";
        
        switch (type) {
            case EntryValue::Bool:
                if (offset + 1 > buffer.size()) return "";
                {
                    std::string s = buffer.substr(offset, 1);
                    offset += 1;
                    return s;
                }
            case EntryValue::Integer:
            case EntryValue::Float:
                 if (offset + 4 > buffer.size()) return "";
                 {
                     std::string s = buffer.substr(offset, 4);
                     offset += 4;
                     return s;
                 }
            case EntryValue::String:
                 {
                     uint8_t len = (uint8_t)buffer[offset];
                     offset += 1;
                     if (offset + len > buffer.size()) return "";
                     std::string s = buffer.substr(offset, len);
                     offset += len;
                     return s;
                 }
            case EntryValue::Vector3:
                 if (offset + 12 > buffer.size()) return "";
                 {
                     std::string s = buffer.substr(offset, 12);
                     offset += 12;
                     return s;
                 }
            case EntryValue::Transform:
                 return "";
            default:
                return "";
        }
    }
    
    static void WriteValue(std::string& buffer, const std::string& val, uint32_t type) {
         switch (type) {
             case EntryValue::String:
                  {
                      uint8_t len = (uint8_t)val.size();
                      buffer += (char)len;
                      buffer += val;
                      break;
                  }
             default:
                  buffer += val;
                  break;
         }
    }

    void ParseContainer(Variable& var) {
        if (var.header.containerType == EntryContainer::Null) return;
        var.entries.clear();
        
        size_t offset = 0;
        if (var.value.size() < 4) return;
        uint32_t count = *reinterpret_cast<const uint32_t*>(&var.value[0]);
        offset += 4;
        
        if (count > 1000000)
            return;
        
        for (uint32_t i = 0; i < count; ++i) {
            std::string keyBytes = "";
            std::string valBytes = "";
            
            if (var.header.containerType == EntryContainer::Dictionary)
                 keyBytes = ReadValue(var.value, offset, var.header.keyType);
            valBytes = ReadValue(var.value, offset, var.header.valueType);
            
            var.entries.push_back({keyBytes, valBytes});
        }
    }
    
    void RebuildContainer(Variable& var) {
        if (var.header.containerType == EntryContainer::Null) return;
        
        std::string newBody = "";
        uint32_t count = (uint32_t)var.entries.size();
        newBody.append((char*)&count, 4);
        
        for (const auto& entry : var.entries) {
             if (var.header.containerType == EntryContainer::Dictionary)
                  WriteValue(newBody, entry.first, var.header.keyType);
             WriteValue(newBody, entry.second, var.header.valueType);
        }
        var.value = newBody;
    }

    bool SaveFile::Load(const std::filesystem::path& path) {
        currentPath = path;
        variables.clear();
        isLoaded = false;
        error = "";

        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            error = "Could not open file.";
            return false;
        }

        std::streamsize fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        if (fileSize < 1) {
            error = "File is empty.";
            return false;
        }

        char checkStart;
        file.read(&checkStart, 1);
        if (checkStart != (char)HX_STARTENTRY) {
            error = "Invalid file signature (Not an ES2 file? Missing 0x7E)";
            return false;
        }
        file.seekg(0, std::ios::beg);

        while ((std::streamsize)file.tellg() < fileSize) {
            if ((std::streamsize)file.tellg() >= fileSize) break;

            char startMarker;
            file.read(&startMarker, 1);
            if (startMarker != (char)HX_STARTENTRY)
                 break; 

            if ((std::streamsize)file.tellg() >= fileSize) break;
            uint8_t tagSize = 0;
            file.read((char*)&tagSize, 1);

            if ((std::streamsize)file.tellg() + (std::streamsize)tagSize > fileSize) break;
            std::string tagStr(tagSize, '\0');
            file.read(&tagStr[0], tagSize);

            if ((std::streamsize)file.tellg() + 4 > fileSize) break;
            uint32_t bodyLength = 0;
            file.read((char*)&bodyLength, 4);

            std::streamsize remaining = fileSize - (std::streamsize)file.tellg();
            if (bodyLength > (uint32_t)remaining) {
                error = "Corruption detected: Body length exceeds file size.";
                break;
            }

            if (bodyLength < 1)
                continue; 

            std::string body(bodyLength, '\0');
            file.read(&body[0], bodyLength);

            uint32_t processedBytes = 0;
            Header h = ParseHeader(body, processedBytes);

            size_t valueDataLen = 0;
            if (bodyLength > processedBytes + 1)
                valueDataLen = bodyLength - processedBytes - 1;

            std::string valueData = "";
            if (valueDataLen > 0)
                valueData = body.substr(processedBytes, valueDataLen);
            
            Variable v;
            v.header = h;
            v.rawKey = tagStr;
            v.key = SanitizeKey(tagStr); 
            v.value = valueData;
            v.rawValue = body; 
            
            if (v.header.containerType != EntryContainer::Null)
                ParseContainer(v);

            variables.push_back(v);
        }

        isLoaded = true;
        return true;
    }

    bool SaveFile::Save(const std::filesystem::path& path) {
        std::ofstream file(path, std::ios::binary);
        if (!file.is_open()) {
            error = "Could not open file for writing.";
            return false;
        }

        for (auto& var : variables) {
            char start = (char)HX_STARTENTRY;
            file.write(&start, 1);

            uint8_t tagSize = (uint8_t)var.rawKey.size();
            file.write((char*)&tagSize, 1);
            file.write(var.rawKey.c_str(), tagSize);

            if (var.header.containerType != EntryContainer::Null)
                RebuildContainer(var);

            std::string headerBytes = "";
            
            if (var.header.containerType != EntryContainer::Null)
                headerBytes += (char)var.header.containerType;
            
            headerBytes += (char)-1;
            
            if (var.header.containerType == EntryContainer::Dictionary)
                 headerBytes.append((char*)&var.header.keyType, 4);

            headerBytes.append((char*)&var.header.valueType, 4);
            
            if (var.header.containerType != EntryContainer::Null)
                headerBytes += var.header.properties;
            
            std::string newBody = headerBytes + var.value;
            newBody += (char)HX_ENDENTRY;
            
            uint32_t bodySize = (uint32_t)newBody.size();
            file.write((char*)&bodySize, 4);
            
            file.write(newBody.c_str(), bodySize);
        }

        return true;
    }

    Variable* SaveFile::GetVariable(const std::string& key) {
        for (auto& v : variables)
            if (v.key == key) return &v;
        return nullptr;
    }

}
