
#include "UI/Windows/NoteEditorWindow.h"
#include "UI/Widgets/Controls.h"
#include "UI/Widgets/Inputs.h"
#include "UI/Widgets/Indicators.h"
#include "UI/Widgets/Layout.h"
#include "UI/UiConstants.h"
#include "UI/Interfaces/IUiRenderer.h"
#include "Protocol/JsonUtils.h"
#include "Platform/Ashita/AshitaPreferencesStore.h"
#include "Platform/Ashita/AshitaThemeHelper.h"
#include "Platform/Ashita/ImGuiBridge.h"
#include "Core/MemoryStats.h"
#include "UI/Windows/UiCloseCoordinator.h"
#include "UI/Helpers/WindowHelper.h"
#ifndef BUILDING_TESTS
#include <Ashita.h>
#include <imgui.h>
#endif
#include <sstream>

namespace XIFriendList {
namespace UI {

NoteEditorWindow::NoteEditorWindow()
    : commandHandler_(nullptr)
    , viewModel_(nullptr)
    , visible_(false)
    , title_("Edit Note")
    , windowId_("NoteEditor")
    , locked_(false)
{
    noteInputBuffer_.reserve(MAX_NOTE_SIZE + 1);
}

void NoteEditorWindow::setFriendName(const std::string& name) {
    if (viewModel_) {
        viewModel_->openEditor(name);
        noteInputBuffer_ = viewModel_->getCurrentNoteText();
        // Reset loading state tracking for new friend
        wasLoading_ = viewModel_->isLoading();
    }
}

const std::string& NoteEditorWindow::getFriendName() const {
    if (viewModel_) {
        return viewModel_->getCurrentFriendName();
    }
    static const std::string empty;
    return empty;
}

void NoteEditorWindow::render() {
    if (!visible_ || !viewModel_) {
        return;
    }
    
    if (!viewModel_->isEditorOpen()) {
        return;
    }
    
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr) {
        return;
    }

    if (pendingClose_ && isUiMenuCleanForClose(renderer)) {
        pendingClose_ = false;
        autoSaveIfNeeded();  // Auto-save before closing
        visible_ = false;
        viewModel_->closeEditor();
        return;
    }
    
    static bool sizeSet = false;
    if (!sizeSet) {
        ImVec2 windowSize(500.0f, 400.0f);
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
    
    // Track friend name to detect changes (for potential future position tracking)
    std::string currentFriendName = viewModel_->getCurrentFriendName();
    lastFriendName_ = currentFriendName;
    
    // Use static window title to ensure position persistence
    // The title never changes, so ImGui preserves position automatically
    std::string windowTitle = title_ + "##" + windowId_;
    bool windowOpen = visible_;
    bool beginResult = renderer->Begin(windowTitle.c_str(), &windowOpen, windowFlags);
    
    if (!beginResult) {
        // Window collapsed or not visible
        // Must call End() exactly once for every Begin() call, regardless of Begin() return value
        renderer->End();
        applyWindowCloseGating(renderer, windowId_, !windowOpen, visible_, pendingClose_);
        if (!visible_) {
            autoSaveIfNeeded();  // Auto-save before closing
            viewModel_->closeEditor();
        }
        return;
    }
    
    visible_ = windowOpen;
    applyWindowCloseGating(renderer, windowId_, !windowOpen, visible_, pendingClose_);
    if (!visible_) {
        autoSaveIfNeeded();  // Auto-save before closing
        viewModel_->closeEditor();
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
    renderer->BeginChild("##note_editor_body", childSize, false, WINDOW_BODY_CHILD_FLAGS);
    // Header
    TextSpec headerSpec;
    headerSpec.text = "Note for " + viewModel_->getCurrentFriendName();
    headerSpec.id = "note_header";
    headerSpec.visible = true;
    createText(headerSpec);
    
    // Storage mode indicator
    TextSpec modeSpec;
    modeSpec.text = "Storage: " + viewModel_->getStorageModeText();
    modeSpec.id = "note_storage_mode";
    modeSpec.visible = true;
    createText(modeSpec);
    
    // Timestamp
    renderTimestamp();
    
    // Error message
    renderError();
    
    renderStatus();
    
    // Action status
    renderActionStatus();
    
    // Loading indicator
    if (viewModel_->isLoading()) {
        TextSpec loadingSpec;
        loadingSpec.text = "Loading...";
        loadingSpec.id = "note_loading";
        loadingSpec.visible = true;
        createText(loadingSpec);
    }
    
    renderer->Separator();
    
    // Sync note buffer from ViewModel when note finishes loading
    // Input is disabled while loading, so we can safely sync when loading finishes
    bool currentlyLoading = viewModel_->isLoading();
    if (wasLoading_ && !currentlyLoading) {
        // Just finished loading - sync buffer from ViewModel
        // Since input is disabled during loading, user couldn't have typed anything
        noteInputBuffer_ = viewModel_->getCurrentNoteText();
    }
    wasLoading_ = currentlyLoading;
    
    // Also sync if buffer is empty but ViewModel has text (handles case where loading finished before we checked)
    if (noteInputBuffer_.empty() && !viewModel_->getCurrentNoteText().empty() && !currentlyLoading) {
        noteInputBuffer_ = viewModel_->getCurrentNoteText();
    }
    
    // Sync buffer when note is deleted (clear editor immediately, but keep window open)
    if (!viewModel_->getCurrentFriendName().empty() && lastFriendName_ != viewModel_->getCurrentFriendName()) {
        noteInputBuffer_ = viewModel_->getCurrentNoteText();
    }
    
    // Sync buffer when note text is cleared (after deletion)
    if (viewModel_->getCurrentNoteText().empty() && !noteInputBuffer_.empty() && !viewModel_->isLoading()) {
        noteInputBuffer_.clear();
    }
    
    InputTextMultilineSpec noteInputSpec;
    noteInputSpec.label = std::string(Constants::LABEL_NOTE);
    noteInputSpec.id = "note_input";
    noteInputSpec.buffer = &noteInputBuffer_;
    noteInputSpec.bufferSize = MAX_NOTE_SIZE + 1;
    noteInputSpec.width = 0.0f;  // Auto-width (full available width)
    noteInputSpec.height = 200.0f;  // Fixed height for multiline area
    noteInputSpec.enabled = !viewModel_->isLoading();
    noteInputSpec.visible = true;
    noteInputSpec.onChange = [this](const std::string& newText) {
        if (viewModel_) {
            viewModel_->setCurrentNoteText(newText);
        }
    };
    bool textChanged = createInputTextMultiline(noteInputSpec);
    if (textChanged) {
        if (viewModel_) {
            viewModel_->setCurrentNoteText(noteInputBuffer_);
        }
    }
    
    bool isInputActive = renderer->IsItemActive();
    if (wasInputActive_ && !isInputActive && renderer->IsItemDeactivatedAfterEdit()) {
        // Input lost focus - auto-save
        autoSaveIfNeeded();
    }
    wasInputActive_ = isInputActive;
    
    // Character count
    size_t currentLength = noteInputBuffer_.length();
    TextSpec charCountSpec;
    charCountSpec.text = "Characters: " + std::to_string(currentLength) + " / " + std::to_string(MAX_NOTE_SIZE);
    charCountSpec.id = "note_char_count";
    charCountSpec.visible = true;
    createText(charCountSpec);
    
    renderer->Separator();
    
    // Buttons (Save button removed - auto-save on close/blur)
    ButtonSpec deleteButtonSpec;
    deleteButtonSpec.label = std::string(Constants::BUTTON_DELETE_NOTE);
    deleteButtonSpec.id = "note_delete_button";
    deleteButtonSpec.enabled = !viewModel_->isLoading() && !viewModel_->getCurrentFriendName().empty();
    deleteButtonSpec.visible = true;
    deleteButtonSpec.onClick = [this]() {
        if (viewModel_ && !viewModel_->getCurrentFriendName().empty()) {
            emitCommand(WindowCommandType::DeleteNote, viewModel_->getCurrentFriendName());
        }
    };
    createButton(deleteButtonSpec);
    
    renderer->SameLine();
    
    // Upload and Download buttons removed - notes upload to server is disabled
    // ButtonSpec uploadButtonSpec;
    // uploadButtonSpec.label = "Upload";
    // uploadButtonSpec.id = "note_upload_button";
    // uploadButtonSpec.enabled = !viewModel_->isLoading() && !viewModel_->isServerMode();
    // uploadButtonSpec.visible = true;
    // uploadButtonSpec.onClick = [this]() {
    //     emitCommand(WindowCommandType::UploadNotes);
    // };
    // createButton(uploadButtonSpec);
    // 
    // renderer->SameLine();
    // 
    // ButtonSpec downloadButtonSpec;
    // downloadButtonSpec.label = "Download";
    // downloadButtonSpec.id = "note_download_button";
    // downloadButtonSpec.enabled = !viewModel_->isLoading() && viewModel_->isServerMode();
    // downloadButtonSpec.visible = true;
    // downloadButtonSpec.onClick = [this]() {
    //     emitCommand(WindowCommandType::DownloadNotes);
    // };
    // createButton(downloadButtonSpec);
    // 
    // renderer->SameLine();
    
    ButtonSpec cancelButtonSpec;
    cancelButtonSpec.label = std::string(Constants::BUTTON_CLOSE);
    cancelButtonSpec.id = "note_close_button";
    cancelButtonSpec.enabled = true;
    cancelButtonSpec.visible = true;
    cancelButtonSpec.onClick = [this]() {
        autoSaveIfNeeded();  // Auto-save before closing
        visible_ = false;
        if (viewModel_) {
            viewModel_->closeEditor();
        }
    };
    createButton(cancelButtonSpec);
    renderer->EndChild();
    
    renderLockButton(renderer, windowId_, locked_, iconManager_, commandHandler_);
    
    renderer->End();
}

void NoteEditorWindow::renderError() {
    if (!viewModel_ || !viewModel_->hasError()) {
        return;
    }
    
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr) {
        return;
    }
    
    TextSpec errorSpec;
    errorSpec.text = "Error: " + viewModel_->getError();
    errorSpec.id = "note_error";
    errorSpec.visible = true;
    createText(errorSpec);
}

void NoteEditorWindow::renderStatus() {
    if (!viewModel_ || !viewModel_->hasStatus()) {
        return;
    }
    
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr) {
        return;
    }
    
    TextSpec statusSpec;
    statusSpec.text = viewModel_->getStatus();
    statusSpec.id = "note_status";
    statusSpec.visible = true;
    createText(statusSpec);
}

void NoteEditorWindow::renderActionStatus() {
    if (!viewModel_) {
        return;
    }
    
    const NotesViewModel::ActionStatus& actionStatus = viewModel_->getActionStatus();
    if (!actionStatus.visible) {
        return;
    }
    
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr) {
        return;
    }
    
    TextSpec actionSpec;
    actionSpec.text = (actionStatus.success ? "[OK] " : "[ERROR] ") + actionStatus.message;
    actionSpec.id = "note_action_status";
    actionSpec.visible = true;
    createText(actionSpec);
}

void NoteEditorWindow::renderTimestamp() {
    if (!viewModel_) {
        return;
    }
    
    uint64_t lastSaved = viewModel_->getLastSavedAt();
    if (lastSaved == 0) {
        return;
    }
    
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr) {
        return;
    }
    
    TextSpec timestampSpec;
    timestampSpec.text = "Last saved: " + NotesViewModel::formatTimestamp(lastSaved) + " (" + viewModel_->getStorageModeText() + ")";
    timestampSpec.id = "note_timestamp";
    timestampSpec.visible = true;
    createText(timestampSpec);
}

void NoteEditorWindow::emitCommand(WindowCommandType type, const std::string& data) {
    if (commandHandler_ != nullptr) {
        WindowCommand command(type, data);
        commandHandler_->handleCommand(command);
    }
}

void NoteEditorWindow::autoSaveIfNeeded() {
    // 1. ViewModel is available
    // 2. Friend name is not empty
    // 3. Not currently loading
    // 4. There are unsaved changes
    if (!viewModel_ || viewModel_->getCurrentFriendName().empty() || viewModel_->isLoading()) {
        return;
    }
    
    // Ensure ViewModel has the latest text from buffer before checking
    viewModel_->setCurrentNoteText(noteInputBuffer_);
    
    if (viewModel_->hasUnsavedChanges()) {
        // There are unsaved changes - save them
        emitCommand(WindowCommandType::SaveNote, viewModel_->getCurrentFriendName());
    }
}

XIFriendList::Core::MemoryStats NoteEditorWindow::getMemoryStats() const {
    size_t bytes = sizeof(NoteEditorWindow);
    
    bytes += title_.capacity();
    bytes += windowId_.capacity();
    bytes += noteInputBuffer_.capacity();
    bytes += lastFriendName_.capacity();
    
    return XIFriendList::Core::MemoryStats(1, bytes, "NoteEditor Window");
}

} // namespace UI
} // namespace XIFriendList
