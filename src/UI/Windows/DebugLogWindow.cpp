
#include "UI/Windows/DebugLogWindow.h"
#include "Debug/DebugLog.h"
#include "UI/Widgets/Layout.h"
#include "UI/Windows/UiCloseCoordinator.h"
#include "UI/Commands/WindowCommands.h"
#include "UI/Interfaces/IUiRenderer.h"
#include "UI/UiConstants.h"
#include "Protocol/JsonUtils.h"
#include "Platform/Ashita/AshitaPreferencesStore.h"
#include "Platform/Ashita/AshitaThemeHelper.h"
#include "Platform/Ashita/ImGuiBridge.h"
#include "UI/Helpers/WindowHelper.h"
#ifndef BUILDING_TESTS
#include <Ashita.h>
#include <imgui.h>
#endif
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <ctime>
#include <windows.h>

namespace XIFriendList {
namespace UI {

DebugLogWindow::DebugLogWindow()
    : commandHandler_(nullptr)
    , visible_(false)
    , title_("FriendList Debug")
    , windowId_("DebugLog")
    , locked_(false)
    , filterText_("")
    , autoScroll_(true)
    , lastLogSize_(0)
{
}

void DebugLogWindow::render() {
    if (!visible_) {
        return;
    }
    
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr) {
        return;
    }

    if (pendingClose_ && isUiMenuCleanForClose(renderer)) {
        pendingClose_ = false;
        visible_ = false;
        return;
    }
    
    static bool sizeSet = false;
    if (!sizeSet) {
        ImVec2 windowSize(800.0f, 600.0f);
        renderer->SetNextWindowSize(windowSize, 0x00000002);  // ImGuiCond_Once
        sizeSet = true;
    }
    
    std::optional<XIFriendList::Platform::Ashita::ScopedThemeGuard> themeGuard;
    if (commandHandler_) {
        auto themeTokens = commandHandler_->getCurrentThemeTokens();
        if (themeTokens.has_value()) {
            themeGuard.emplace(themeTokens.value());
        }
    }
    
    locked_ = XIFriendList::Platform::Ashita::AshitaPreferencesStore::loadWindowLockState(windowId_);
    
    // G2: Set window flags to prevent move/resize when locked
    int windowFlags = 0;
    if (locked_) {
        windowFlags = 0x0004 | 0x0002;  // ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
    }
    
    // Begin window
    bool windowOpen = visible_;
    bool beginResult = renderer->Begin(title_.c_str(), &windowOpen, windowFlags);
    
    if (!beginResult) {
        // Window collapsed or not visible
        // Must call End() exactly once for every Begin() call, regardless of Begin() return value
        renderer->End();
        applyWindowCloseGating(renderer, windowId_, !windowOpen, visible_, pendingClose_);
        return;
    }
    
    visible_ = windowOpen;
    applyWindowCloseGating(renderer, windowId_, !windowOpen, visible_, pendingClose_);
    if (!visible_) {
        // Must call End() exactly once for every Begin() call, regardless of Begin() return value
        renderer->End();
        return;
    }
    
    UI::ImVec2 contentAvail = renderer->GetContentRegionAvail();
#ifndef BUILDING_TESTS
    float reserve = calculateLockButtonReserve();
    UI::ImVec2 childSize(0.0f, contentAvail.y - reserve);
    if (childSize.y < 0.0f) {
        childSize.y = 0.0f;
    }
#else
    UI::ImVec2 childSize(0.0f, contentAvail.y);
#endif
    renderer->BeginChild("##debug_log_body", childSize, false, WINDOW_BODY_CHILD_FLAGS);
    // Render toolbar
    renderToolbar();
    // Render log content
    renderLogContent();
    renderer->EndChild();
    
    renderLockButton(renderer, windowId_, locked_, nullptr, commandHandler_);
    
    renderer->End();
}

void DebugLogWindow::renderToolbar() {
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr) {
        return;
    }
    
    // Filter input
    renderer->TextUnformatted("Filter:");
    renderer->SameLine();
    
    // Use InputText widget if available, otherwise fallback to raw ImGui
    char filterBuf[256];
    strncpy_s(filterBuf, filterText_.c_str(), sizeof(filterBuf) - 1);
    filterBuf[sizeof(filterBuf) - 1] = '\0';
    
    if (renderer->InputText("##filter", filterBuf, sizeof(filterBuf), 0)) {
        filterText_ = std::string(filterBuf);
    }
    
    renderer->SameLine();
    
    // Auto-scroll checkbox
    bool autoScroll = autoScroll_;
    if (renderer->Checkbox("Auto-scroll", &autoScroll)) {
        autoScroll_ = autoScroll;
    }
    
    renderer->SameLine();
    
    // Copy All button
    if (renderer->Button("Copy All")) {
        copyAllToClipboard();
    }
    
    renderer->SameLine();
    
    // Clear button
    if (renderer->Button("Clear")) {
        clearLog();
    }
    
    // Line count and last updated time
    XIFriendList::Debug::DebugLog& log = XIFriendList::Debug::DebugLog::getInstance();
    size_t logSize = log.size();
    
    renderer->SameLine();
    std::stringstream ss;
    ss << "Lines: " << logSize << " / " << log.maxLines();
    renderer->TextUnformatted(ss.str().c_str());
    
    renderer->Separator();
}

void DebugLogWindow::renderLogContent() {
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr) {
        return;
    }
    
    std::vector<std::string> logLines = getFilteredLogLines();
    
    XIFriendList::Debug::DebugLog& log = XIFriendList::Debug::DebugLog::getInstance();
    size_t currentLogSize = log.size();
    if (currentLogSize != lastLogSize_) {
        cachedLogLines_ = logLines;
        lastLogSize_ = currentLogSize;
    } else {
        // Use cached lines if filter hasn't changed
        logLines = cachedLogLines_;
    }
    
    // Render each log line
    for (const auto& line : logLines) {
        renderer->TextUnformatted(line.c_str());
    }
    
    // Note: Auto-scroll would require SetScrollHereY which isn't in IUiRenderer
}

void DebugLogWindow::copyAllToClipboard() {
    // Copy the currently displayed (filtered) log lines to the Windows clipboard.
    std::vector<std::string> lines = getFilteredLogLines();
    if (lines.empty()) {
        XIFriendList::Debug::DebugLog::getInstance().push("[DebugLog] Copy All: no lines to copy.");
        return;
    }
    
    std::string text;
    // Pre-allocate roughly to reduce reallocations (average ~100 chars per line is typical)
    text.reserve(lines.size() * 100);
    for (size_t i = 0; i < lines.size(); ++i) {
        text += lines[i];
        if (i + 1 < lines.size()) {
            text += "\r\n";
        }
    }
    
    if (text.empty()) {
        XIFriendList::Debug::DebugLog::getInstance().push("[DebugLog] Copy All: nothing to copy.");
        return;
    }
    
    if (!OpenClipboard(nullptr)) {
        XIFriendList::Debug::DebugLog::getInstance().push("[DebugLog] Copy All: failed to open clipboard.");
        return;
    }
    
    EmptyClipboard();
    
    // Convert UTF-8 to wide string for CF_UNICODETEXT.
    const int wlen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (wlen <= 0) {
        CloseClipboard();
        XIFriendList::Debug::DebugLog::getInstance().push("[DebugLog] Copy All: UTF-8 conversion failed.");
        return;
    }
    
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, static_cast<SIZE_T>(wlen) * sizeof(wchar_t));
    if (hMem == nullptr) {
        CloseClipboard();
        XIFriendList::Debug::DebugLog::getInstance().push("[DebugLog] Copy All: clipboard allocation failed.");
        return;
    }
    
    wchar_t* pMem = static_cast<wchar_t*>(GlobalLock(hMem));
    if (pMem == nullptr) {
        GlobalFree(hMem);
        CloseClipboard();
        XIFriendList::Debug::DebugLog::getInstance().push("[DebugLog] Copy All: clipboard lock failed.");
        return;
    }
    
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, pMem, wlen);
    GlobalUnlock(hMem);
    
    SetClipboardData(CF_UNICODETEXT, hMem);
    CloseClipboard();
    
    std::ostringstream msg;
    msg << "[DebugLog] Copied " << lines.size() << " lines to clipboard.";
    XIFriendList::Debug::DebugLog::getInstance().push(msg.str());
}

void DebugLogWindow::clearLog() {
    XIFriendList::Debug::DebugLog::getInstance().clear();
    cachedLogLines_.clear();
    lastLogSize_ = 0;
}

std::vector<std::string> DebugLogWindow::getFilteredLogLines() const {
    XIFriendList::Debug::DebugLog& log = XIFriendList::Debug::DebugLog::getInstance();
    std::vector<XIFriendList::Debug::LogEntry> entries = log.snapshot();
    
    std::vector<std::string> result;
    result.reserve(entries.size());
    
    // Convert entries to strings and apply filter
    for (const auto& entry : entries) {
        std::string line = entry.message;
        
        // Apply filter if set
        if (!filterText_.empty()) {
            // Case-insensitive substring search
            std::string lineLower = line;
            std::string filterLower = filterText_;
            std::transform(lineLower.begin(), lineLower.end(), lineLower.begin(), 
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(), 
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            
            if (lineLower.find(filterLower) == std::string::npos) {
                continue;
            }
        }
        
        result.push_back(line);
    }
    
    return result;
}

void DebugLogWindow::emitCommand(WindowCommandType type, const std::string& data) {
    if (commandHandler_ != nullptr) {
        WindowCommand command(type, data);
        commandHandler_->handleCommand(command);
    }
}

} // namespace UI
} // namespace XIFriendList

