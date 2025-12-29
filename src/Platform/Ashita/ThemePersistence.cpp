#include "Platform/Ashita/ThemePersistence.h"
#include "Platform/Ashita/PathUtils.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <windows.h>
#include <map>

namespace XIFriendList {
namespace Platform {
namespace Ashita {

std::string ThemePersistence::getConfigPath() {
    char dllPath[MAX_PATH] = {0};
    
    HMODULE hModule = GetModuleHandleA(nullptr);
    if (hModule != nullptr) {
        if (GetModuleFileNameA(hModule, dllPath, MAX_PATH) > 0) {
            std::string path(dllPath);
            size_t lastSlash = path.find_last_of("\\/");
            if (lastSlash != std::string::npos) {
                std::string gameDir = path.substr(0, lastSlash);
                lastSlash = gameDir.find_last_of("\\/");
                if (lastSlash != std::string::npos) {
                    gameDir = gameDir.substr(0, lastSlash + 1);
                    return gameDir + "config\\FFXIFriendList\\ffxifriendlist.ini";
                }
            }
        }
    }
    
    std::string defaultPath = ::XIFriendList::Platform::Ashita::PathUtils::getDefaultIniPath();
    return defaultPath.empty() ? "C:\\HorizonXI\\HorizonXI\\Game\\config\\FFXIFriendList\\ffxifriendlist.ini" : defaultPath;
}

std::string ThemePersistence::getCustomThemesPath() {
    char dllPath[MAX_PATH] = {0};
    
    HMODULE hModule = GetModuleHandleA(nullptr);
    if (hModule != nullptr) {
        if (GetModuleFileNameA(hModule, dllPath, MAX_PATH) > 0) {
            std::string path(dllPath);
            size_t lastSlash = path.find_last_of("\\/");
            if (lastSlash != std::string::npos) {
                std::string gameDir = path.substr(0, lastSlash);
                lastSlash = gameDir.find_last_of("\\/");
                if (lastSlash != std::string::npos) {
                    gameDir = gameDir.substr(0, lastSlash + 1);
                    return gameDir + "config\\FFXIFriendList\\CustomThemes.ini";
                }
            }
        }
    }
    
    std::string defaultPath = ::XIFriendList::Platform::Ashita::PathUtils::getDefaultConfigPath("CustomThemes.ini");
    return defaultPath.empty() ? "C:\\HorizonXI\\HorizonXI\\Game\\config\\FFXIFriendList\\CustomThemes.ini" : defaultPath;
}

void ThemePersistence::ensureConfigDirectory(const std::string& filePath) {
    size_t lastSlash = filePath.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        std::string dir = filePath.substr(0, lastSlash);
        CreateDirectoryA(dir.c_str(), nullptr);
    }
}

void ThemePersistence::trimString(std::string& str) {
    str.erase(0, str.find_first_not_of(" \t\r\n"));
    str.erase(str.find_last_not_of(" \t\r\n") + 1);
}

::XIFriendList::Core::Color ThemePersistence::parseColor(const std::string& colorStr) {
    std::istringstream stream(colorStr);
    std::string token;
    std::vector<float> components;
    
    while (std::getline(stream, token, ',')) {
        try {
            std::string trimmed = token;
            trimString(trimmed);
            components.push_back(std::stof(trimmed));
        } catch (...) {
            break;
        }
    }
    
    if (components.size() >= 3) {
        float r = components[0];
        float g = components.size() > 1 ? components[1] : 0.0f;
        float b = components.size() > 2 ? components[2] : 0.0f;
        float a = components.size() > 3 ? components[3] : 1.0f;
        return ::XIFriendList::Core::Color(r, g, b, a);
    }
    
    return ::XIFriendList::Core::Color(0.0f, 0.0f, 0.0f, 1.0f);
}

std::string ThemePersistence::formatColor(const ::XIFriendList::Core::Color& color) {
    std::ostringstream stream;
    stream << color.r << "," << color.g << "," << color.b << "," << color.a;
    return stream.str();
}

std::string ThemePersistence::readIniValue(const std::string& filePath, const std::string& key) {
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return "";
        }
        
        std::string line;
        bool isInSettingsSection = false;
        
        while (std::getline(file, line)) {
            std::string trimmed = line;
            trimString(trimmed);
            
            if (trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '#') {
                continue;
            }
            
            if (trimmed[0] == '[' && trimmed.back() == ']') {
                std::string section = trimmed.substr(1, trimmed.length() - 2);
                isInSettingsSection = (section == "Settings");
                continue;
            }
            
            if (!isInSettingsSection) {
                continue;
            }
            
            size_t equalsPos = trimmed.find('=');
            if (equalsPos == std::string::npos) {
                continue;
            }
            
            std::string lineKey = trimmed.substr(0, equalsPos);
            std::string value = trimmed.substr(equalsPos + 1);
            trimString(lineKey);
            trimString(value);
            
            // Case-insensitive comparison
            std::string lowerKey = lineKey;
            std::string lowerSearchKey = key;
            std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), 
                          [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            std::transform(lowerSearchKey.begin(), lowerSearchKey.end(), lowerSearchKey.begin(), 
                          [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            
            if (lowerKey == lowerSearchKey) {
                return value;
            }
        }
    } catch (...) {
    }
    
    return "";
}

bool ThemePersistence::writeIniValue(const std::string& filePath, const std::string& key, const std::string& value) {
    try {
        ensureConfigDirectory(filePath);
        
        std::vector<std::string> lines;
        std::ifstream inFile(filePath);
        if (inFile.is_open()) {
            std::string line;
            while (std::getline(inFile, line)) {
                lines.push_back(line);
            }
            inFile.close();
        }
        
        bool foundSettings = false;
        bool foundKey = false;
        size_t settingsIndex = 0;
        
        for (size_t i = 0; i < lines.size(); ++i) {
            std::string trimmed = lines[i];
            trimString(trimmed);
            if (trimmed == "[Settings]") {
                foundSettings = true;
                settingsIndex = i;
                for (size_t j = i + 1; j < lines.size(); ++j) {
                    std::string nextLine = lines[j];
                    std::string nextTrimmed = nextLine;
                    trimString(nextTrimmed);
                    if (nextTrimmed.empty() || nextTrimmed[0] == '[') {
                        break;
                    }
                    size_t equalsPos = nextTrimmed.find('=');
                    if (equalsPos != std::string::npos) {
                        std::string lineKey = nextTrimmed.substr(0, equalsPos);
                        trimString(lineKey);
                        std::string lowerKey = lineKey;
                        std::string lowerSearchKey = key;
                        std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), 
                                      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                        std::transform(lowerSearchKey.begin(), lowerSearchKey.end(), lowerSearchKey.begin(), 
                                      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                        if (lowerKey == lowerSearchKey) {
                            lines[j] = key + "=" + value;
                            foundKey = true;
                            break;
                        }
                    }
                }
                break;
            }
        }
        
        if (!foundSettings) {
            if (!lines.empty() && !lines.back().empty()) {
                lines.push_back("");
            }
            lines.push_back("[Settings]");
            lines.push_back(key + "=" + value);
        } else if (!foundKey) {
            lines.insert(lines.begin() + settingsIndex + 1, key + "=" + value);
        }
        
        std::ofstream outFile(filePath);
        if (!outFile.is_open()) {
            return false;
        }
        
        for (const auto& line : lines) {
            outFile << line << "\n";
        }
        outFile.close();
        return true;
    } catch (...) {
        return false;
    }
}

bool ThemePersistence::loadFromFile(::XIFriendList::App::State::ThemeState& state) {
    try {
        std::string configPath = getConfigPath();
        
        std::string themeIndexStr = readIniValue(configPath, "Theme");
        if (!themeIndexStr.empty()) {
            try {
                int themeIndex = std::stoi(themeIndexStr);
                if (themeIndex >= -2 && themeIndex <= 3) {
                    state.themeIndex = themeIndex;
                }
            } catch (...) {
            }
        }
        
        state.presetName = readIniValue(configPath, "ThemePreset");
        if (state.presetName.empty()) {
            state.presetName = readIniValue(configPath, "themePreset");
        }
        if (state.presetName.empty()) {
            state.presetName = readIniValue(configPath, "theme_preset");
        }
        
        state.customThemeName = readIniValue(configPath, "CustomThemeName");
        
        std::string bgAlphaStr = readIniValue(configPath, "BackgroundAlpha");
        if (!bgAlphaStr.empty()) {
            try {
                float alpha = std::stof(bgAlphaStr);
                if (alpha >= 0.0f && alpha <= 1.0f) {
                    state.backgroundAlpha = alpha;
                }
            } catch (...) {
            }
        }
        
        std::string textAlphaStr = readIniValue(configPath, "TextAlpha");
        if (!textAlphaStr.empty()) {
            try {
                float alpha = std::stof(textAlphaStr);
                if (alpha >= 0.0f && alpha <= 1.0f) {
                    state.textAlpha = alpha;
                }
            } catch (...) {
            }
        }
        
        std::string customThemesPath = getCustomThemesPath();
        std::ifstream file(customThemesPath);
        if (file.is_open()) {
            std::string line;
            std::string currentThemeName;
            ::XIFriendList::Core::CustomTheme currentTheme;
            bool isInThemeSection = false;
            
            typedef ::XIFriendList::Core::Color (::XIFriendList::Core::CustomTheme::*ColorMemberPtr);
            static const std::map<std::string, ColorMemberPtr> colorMap = {
                {"WindowBg", &::XIFriendList::Core::CustomTheme::windowBgColor},
                {"ChildBg", &::XIFriendList::Core::CustomTheme::childBgColor},
                {"FrameBg", &::XIFriendList::Core::CustomTheme::frameBgColor},
                {"FrameBgHovered", &::XIFriendList::Core::CustomTheme::frameBgHovered},
                {"FrameBgActive", &::XIFriendList::Core::CustomTheme::frameBgActive},
                {"TitleBg", &::XIFriendList::Core::CustomTheme::titleBg},
                {"TitleBgActive", &::XIFriendList::Core::CustomTheme::titleBgActive},
                {"TitleBgCollapsed", &::XIFriendList::Core::CustomTheme::titleBgCollapsed},
                {"Button", &::XIFriendList::Core::CustomTheme::buttonColor},
                {"ButtonHovered", &::XIFriendList::Core::CustomTheme::buttonHoverColor},
                {"ButtonActive", &::XIFriendList::Core::CustomTheme::buttonActiveColor},
                {"Separator", &::XIFriendList::Core::CustomTheme::separatorColor},
                {"SeparatorHovered", &::XIFriendList::Core::CustomTheme::separatorHovered},
                {"SeparatorActive", &::XIFriendList::Core::CustomTheme::separatorActive},
                {"ScrollbarBg", &::XIFriendList::Core::CustomTheme::scrollbarBg},
                {"ScrollbarGrab", &::XIFriendList::Core::CustomTheme::scrollbarGrab},
                {"ScrollbarGrabHovered", &::XIFriendList::Core::CustomTheme::scrollbarGrabHovered},
                {"ScrollbarGrabActive", &::XIFriendList::Core::CustomTheme::scrollbarGrabActive},
                {"CheckMark", &::XIFriendList::Core::CustomTheme::checkMark},
                {"SliderGrab", &::XIFriendList::Core::CustomTheme::sliderGrab},
                {"SliderGrabActive", &::XIFriendList::Core::CustomTheme::sliderGrabActive},
                {"Header", &::XIFriendList::Core::CustomTheme::header},
                {"HeaderHovered", &::XIFriendList::Core::CustomTheme::headerHovered},
                {"HeaderActive", &::XIFriendList::Core::CustomTheme::headerActive},
                {"Text", &::XIFriendList::Core::CustomTheme::textColor},
                {"TextDisabled", &::XIFriendList::Core::CustomTheme::textDisabled},
            };
            
            while (std::getline(file, line)) {
                std::string trimmed = line;
                trimString(trimmed);
                
                if (trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '#') {
                    continue;
                }
                
                if (trimmed[0] == '[' && trimmed.back() == ']') {
                    if (isInThemeSection && !currentThemeName.empty()) {
                        state.customThemes.push_back(currentTheme);
                    }
                    
                    currentThemeName = trimmed.substr(1, trimmed.length() - 2);
                    currentTheme = ::XIFriendList::Core::CustomTheme();
                    currentTheme.name = currentThemeName;
                    isInThemeSection = true;
                    continue;
                }
                
                if (!isInThemeSection) {
                    continue;
                }
                
                size_t equalsPos = trimmed.find('=');
                if (equalsPos != std::string::npos) {
                    std::string lineKey = trimmed.substr(0, equalsPos);
                    std::string value = trimmed.substr(equalsPos + 1);
                    trimString(lineKey);
                    trimString(value);
                    
                    auto it = colorMap.find(lineKey);
                    if (it != colorMap.end()) {
                        ::XIFriendList::Core::Color color = parseColor(value);
                        currentTheme.*(it->second) = color;
                    }
                }
            }
            
            if (isInThemeSection && !currentThemeName.empty()) {
                state.customThemes.push_back(currentTheme);
            }
            
            file.close();
        }
        
        return true;
    } catch (...) {
        return false;
    }
}

bool ThemePersistence::saveToFile(const ::XIFriendList::App::State::ThemeState& state) {
    try {
        std::string configPath = getConfigPath();
        
        writeIniValue(configPath, "Theme", std::to_string(state.themeIndex));
        
        if (!state.presetName.empty()) {
            writeIniValue(configPath, "ThemePreset", state.presetName);
        }
        
        writeIniValue(configPath, "CustomThemeName", state.customThemeName);
        
        std::ostringstream bgAlphaStream;
        bgAlphaStream << state.backgroundAlpha;
        writeIniValue(configPath, "BackgroundAlpha", bgAlphaStream.str());
        
        std::ostringstream textAlphaStream;
        textAlphaStream << state.textAlpha;
        writeIniValue(configPath, "TextAlpha", textAlphaStream.str());
        
        std::string customThemesPath = getCustomThemesPath();
        ensureConfigDirectory(customThemesPath);
        
        std::ofstream file(customThemesPath);
        if (!file.is_open()) {
            return false;
        }
        
        file << "; Custom Themes for XIFriendList\n";
        file << "; Format: [ThemeName]\n";
        file << "; Key=Red,Green,Blue,Alpha\n\n";
        
        for (const auto& theme : state.customThemes) {
            file << "[" << theme.name << "]\n";
            file << "WindowBg=" << formatColor(theme.windowBgColor) << "\n";
            file << "ChildBg=" << formatColor(theme.childBgColor) << "\n";
            file << "FrameBg=" << formatColor(theme.frameBgColor) << "\n";
            file << "FrameBgHovered=" << formatColor(theme.frameBgHovered) << "\n";
            file << "FrameBgActive=" << formatColor(theme.frameBgActive) << "\n";
            file << "TitleBg=" << formatColor(theme.titleBg) << "\n";
            file << "TitleBgActive=" << formatColor(theme.titleBgActive) << "\n";
            file << "TitleBgCollapsed=" << formatColor(theme.titleBgCollapsed) << "\n";
            file << "Button=" << formatColor(theme.buttonColor) << "\n";
            file << "ButtonHovered=" << formatColor(theme.buttonHoverColor) << "\n";
            file << "ButtonActive=" << formatColor(theme.buttonActiveColor) << "\n";
            file << "Separator=" << formatColor(theme.separatorColor) << "\n";
            file << "SeparatorHovered=" << formatColor(theme.separatorHovered) << "\n";
            file << "SeparatorActive=" << formatColor(theme.separatorActive) << "\n";
            file << "ScrollbarBg=" << formatColor(theme.scrollbarBg) << "\n";
            file << "ScrollbarGrab=" << formatColor(theme.scrollbarGrab) << "\n";
            file << "ScrollbarGrabHovered=" << formatColor(theme.scrollbarGrabHovered) << "\n";
            file << "ScrollbarGrabActive=" << formatColor(theme.scrollbarGrabActive) << "\n";
            file << "CheckMark=" << formatColor(theme.checkMark) << "\n";
            file << "SliderGrab=" << formatColor(theme.sliderGrab) << "\n";
            file << "SliderGrabActive=" << formatColor(theme.sliderGrabActive) << "\n";
            file << "Header=" << formatColor(theme.header) << "\n";
            file << "HeaderHovered=" << formatColor(theme.headerHovered) << "\n";
            file << "HeaderActive=" << formatColor(theme.headerActive) << "\n";
            file << "Text=" << formatColor(theme.textColor) << "\n";
            file << "TextDisabled=" << formatColor(theme.textDisabled) << "\n";
            file << "\n";
        }
        
        file.close();
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

