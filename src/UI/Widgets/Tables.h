// Consolidated table widgets: Table, FriendTableWidget, ContextMenu

#ifndef UI_TABLES_H
#define UI_TABLES_H

#include "WidgetSpecs.h"
#include "UI/ViewModels/FriendListViewModel.h"
#include "UI/Commands/WindowCommands.h"
#include "Core/MemoryStats.h"
#include "Core/ModelsCore.h"
#include <string>
#include <vector>
#include <functional>

namespace XIFriendList {
namespace UI {

class IUiRenderer;


void createTable(const TableSpec& spec);


struct FriendTableWidgetSpec {
    std::string tableId;            // Unique per window (prevents ImGui state collisions)
    std::string toggleRowId;        // Unique per window
    std::string sectionHeaderId;    // Unique per window
    std::string sectionHeaderLabel; // e.g. "Your Friends" / "Quick Online"
    bool showSectionHeader{ true };
    bool showColumnToggles{ true };
    std::string commandScope;       // "FriendList" or "QuickOnline" (used for preference persistence)
};

class FriendTableWidget {
public:
    FriendTableWidget();

    void setViewModel(FriendListViewModel* viewModel) { viewModel_ = viewModel; }
    void setCommandHandler(IWindowCommandHandler* handler) { commandHandler_ = handler; }
    void setIconManager(void* iconManager) { iconManager_ = iconManager; }
    void setSpec(const FriendTableWidgetSpec& spec) { spec_ = spec; }
    void setShareFriendsAcrossAlts(bool enabled) { shareFriendsAcrossAlts_ = enabled; }
    void setViewSettings(const XIFriendList::Core::FriendViewSettings& settings) { viewSettings_ = settings; cachedVisibleColumnsValid_ = false; }

    // Optional row index mapping (display row -> viewModel row).
    // Allows windows to apply filtering without duplicating the table logic.
    void render(const std::vector<size_t>* rowIndexMap = nullptr);
    
    XIFriendList::Core::MemoryStats getMemoryStats() const;

private:
    void emitCommand(WindowCommandType type, const std::string& data = "");
    void renderContextMenuItems(IUiRenderer* renderer, const std::string& friendName);
    static std::string capitalizeWords(const std::string& s);

    FriendListViewModel* viewModel_{ nullptr };
    IWindowCommandHandler* commandHandler_{ nullptr };
    void* iconManager_{ nullptr }; // Platform::Ashita::IconManager* (void* to avoid Platform dependency)
    FriendTableWidgetSpec spec_;
    bool shareFriendsAcrossAlts_{ true }; // Whether friends are shared across alts (default: true)

    // Sort state (Table widget toggles these; sorting is handled by ViewModel ordering).
    int sortColumn_{ -1 };
    bool sortAscending_{ true };

    XIFriendList::Core::FriendViewSettings viewSettings_;  // Per-window view settings
    
    // Cached visible columns (rebuilt only when visibility flags change).
    bool cachedVisibleColumnsValid_{ false };
    XIFriendList::Core::FriendViewSettings lastViewSettings_;
    std::vector<TableColumnSpec> cachedVisibleColumns_;
};

// ContextMenu widget

// Menu item specification
struct MenuItemSpec {
    std::string label;
    std::string id;
    bool enabled = true;
    bool visible = true;
    std::function<void()> onClick;  // Callback when clicked
    
    MenuItemSpec() = default;
    MenuItemSpec(const std::string& label, const std::string& id)
        : label(label), id(id) {}
};

// Context menu specification
struct ContextMenuSpec {
    std::string id;  // Unique identifier for the menu
    std::vector<MenuItemSpec> items;
    bool visible = true;
    
    ContextMenuSpec() = default;
    ContextMenuSpec(const std::string& id) : id(id) {}
};

// Returns true if menu was opened and an item was clicked
// Note: This should be called every frame after right-click is detected
// Call openContextMenu() first to trigger the popup
bool createContextMenu(const ContextMenuSpec& spec);

// Open a context menu (call this when right-click is detected)
void openContextMenu(const std::string& menuId);

} // namespace UI
} // namespace XIFriendList

#endif // UI_TABLES_H

