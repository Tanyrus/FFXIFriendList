// Consolidated table widgets implementation

#include "UI/Widgets/Tables.h"
#include "UI/Widgets/Layout.h"
#include "UI/Interfaces/IUiRenderer.h"
#include "Platform/Ashita/IconManager.h"
#include "Core/MemoryStats.h"
#include "Protocol/JsonUtils.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <sstream>

namespace XIFriendList {
namespace UI {


void createTable(const TableSpec& spec) {
    if (!spec.visible || spec.rowRenderer == nullptr) {
        return;
    }
    
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr) {
        return;
    }
    
    // ImGuiTableFlags_Borders = 0x00000002 (all borders)
    // ImGuiTableFlags_RowBg = 0x00000010 (alternating row background)
    int flags = 0x00000002 | 0x00000010;  // Borders | RowBg
    
    // Begin table - use stable ID for consistent layout
    std::string tableId = "##" + spec.id;
    const std::string headerPopupId = tableId + "_header_ctx";
    int columnCount = static_cast<int>(spec.columns.size());
    // Use ImVec2(0, 0) for auto-size, prevents layout shifts
    ImVec2 outerSize(0, 0);
    
    // Calling BeginTable with same ID and structure maintains position
    bool tableStarted = renderer->BeginTable(tableId.c_str(), columnCount, flags, outerSize, 0.0f);
    
    if (!tableStarted) {
        return;
    }
    
    {
        // Setup columns - MUST be called every frame before first row
        // ImGui uses this to maintain column state, but calling it consistently prevents layout shifts
        // Use WidthStretch for all columns
        int colFlags = 0x00000001;  // ImGuiTableColumnFlags_WidthStretch for all columns
        for (size_t i = 0; i < spec.columns.size(); ++i) {
            const auto& col = spec.columns[i];
            // Use column header as ID (ImGui uses this to restore column widths)
            // The unique table ID + column header allows ImGui to persist widths per table
            renderer->TableSetupColumn(col.header.c_str(), colFlags, 0.0f, static_cast<unsigned int>(i));
        }
        
        if (spec.showHeaders) {
            // The header row is NEVER affected by sorting - it always appears as the first row
            // ImGuiTableRowFlags_Headers (0x00000001) marks this as a header row, not a data row
            renderer->TableNextRow(0x00000001);  // ImGuiTableRowFlags_Headers - header row (always first)
            for (size_t i = 0; i < spec.columns.size(); ++i) {
                renderer->TableSetColumnIndex(static_cast<int>(i));
                // Add left padding for first column header
                std::string headerText = spec.columns[i].header;
                if (i == 0) {
                    headerText = " " + headerText;  // 2 spaces for padding
                }
                renderer->TableHeader(headerText.c_str());

                // Right-click on any header opens the header context menu (if provided).
                if (spec.headerContextMenu && renderer->IsItemClicked(1)) {
                    renderer->OpenPopup(headerPopupId.c_str());
                }
                
                // Note: Clicking header updates sort state, but header row itself never moves
                // Sorting only affects data rows rendered below
                if (spec.sortable && spec.sortColumn != nullptr && spec.sortAscending != nullptr) {
                    if (renderer->IsItemClicked(0)) {
                        // Toggle sort column/direction
                        if (*spec.sortColumn == static_cast<int>(i)) {
                            // Same column - toggle direction
                            *spec.sortAscending = !*spec.sortAscending;
                        } else {
                            // New column - set to ascending
                            *spec.sortColumn = static_cast<int>(i);
                            *spec.sortAscending = true;
                        }
                    }
                }
            }

            // Render header context menu popup (if open).
            if (spec.headerContextMenu && renderer->BeginPopup(headerPopupId.c_str())) {
                spec.headerContextMenu();
                renderer->EndPopup();
            }
        }
        
        // Render data rows (header row is already rendered above, this is only for data)
        for (size_t rowIndex = 0; rowIndex < spec.rowCount; ++rowIndex) {
            // Push row ID for stable widget IDs (prevents flickering when data changes)
            char rowIdStr[32];
            snprintf(rowIdStr, sizeof(rowIdStr), "%zu", rowIndex);
            renderer->PushID(rowIdStr);
            
            renderer->TableNextRow();  // Data row (not header - header was rendered above)
            
            // Row data is generated lazily only if a cell falls back to default rendering.
            std::vector<std::string> rowData;
            bool rowDataGenerated = false;
            
            // Track if row was clicked for callbacks
            bool rowLeftClicked = false;
            bool rowRightClicked = false;
            
            // Render cells - MUST render ALL columns, even if rowData is incomplete
            for (size_t colIndex = 0; colIndex < spec.columns.size(); ++colIndex) {
                renderer->TableSetColumnIndex(static_cast<int>(colIndex));
                
                // Try custom cell renderer first (for icons, etc.)
                bool customRendered = false;
                if (spec.cellRenderer) {
                    customRendered = spec.cellRenderer(rowIndex, colIndex, spec.columns[colIndex].id);
                }
                
                if (!customRendered) {
                    if (!rowDataGenerated) {
                        rowData = spec.rowRenderer(rowIndex);
                        rowDataGenerated = true;
                    }
                    std::string cellText;
                    if (colIndex < rowData.size()) {
                        cellText = rowData[colIndex];
                    }
                    
                    // Render cell text (empty string is fine - ImGui will handle it)
                    renderer->TextUnformatted(cellText.c_str());
                }
                
                if (renderer->IsItemClicked(0)) {  // Left mouse button
                    rowLeftClicked = true;
                    if (spec.onCellClick) {
                        spec.onCellClick(rowIndex, static_cast<int>(colIndex));
                    }
                }
                if (renderer->IsItemClicked(1)) {  // Right mouse button
                    rowRightClicked = true;
                }
            }
            
            if (rowLeftClicked && spec.onRowClick) {
                spec.onRowClick(rowIndex);
            }
            if (rowRightClicked && spec.onRowRightClick) {
                spec.onRowRightClick(rowIndex);
            }
            
            renderer->PopID();  // Pop row ID
        }
        
        renderer->EndTable();
    }
}


FriendTableWidget::FriendTableWidget() 
    : viewSettings_()  // Default: showJob=true, others=false
{
    // Provide stable defaults; caller should override IDs per window.
    spec_.tableId = "friend_table";
    spec_.toggleRowId = "friend_table_columns";
    spec_.sectionHeaderId = "friend_table_header";
    spec_.sectionHeaderLabel = "Friends";
    spec_.commandScope = "FriendList";
}

std::string FriendTableWidget::capitalizeWords(const std::string& s) {
    std::string out = s;
    if (!out.empty()) {
        out[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(out[0])));
        for (size_t i = 1; i < out.length(); ++i) {
            if (out[i - 1] == ' ') {
                out[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(out[i])));
            } else {
                out[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(out[i])));
            }
        }
    }
    return out;
}

void FriendTableWidget::emitCommand(WindowCommandType type, const std::string& data) {
    if (commandHandler_ != nullptr) {
        WindowCommand command(type, data);
        commandHandler_->handleCommand(command);
    }
}

void FriendTableWidget::renderContextMenuItems(IUiRenderer* renderer, const std::string& friendName) {
    if (!renderer) {
        return;
    }

    bool hasOutgoingRequest = false;
    std::string requestIdForFriend;
    std::string friendedAsValue;
    if (viewModel_) {
        const auto& outgoing = viewModel_->getOutgoingRequests();
        for (const auto& req : outgoing) {
            std::string reqToLower = req.toCharacterName;
            std::transform(reqToLower.begin(), reqToLower.end(), reqToLower.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            std::string friendNameLower = friendName;
            std::transform(friendNameLower.begin(), friendNameLower.end(), friendNameLower.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (reqToLower == friendNameLower) {
                requestIdForFriend = req.requestId;
                hasOutgoingRequest = true;
                break;
            }
        }
        
        const auto& rows = viewModel_->getFriendRows();
        for (const auto& row : rows) {
            if (row.name == friendName || row.friendedAs == friendName) {
                friendedAsValue = row.friendedAs;
                break;
            }
        }
    }

    if (!friendedAsValue.empty()) {
        renderer->TextUnformatted(("Friended As: " + capitalizeWords(friendedAsValue)).c_str());
        renderer->Separator();
    }
    
    if (hasOutgoingRequest && !requestIdForFriend.empty()) {
        if (renderer->MenuItem("Cancel Request")) {
            emitCommand(WindowCommandType::CancelFriendRequest, requestIdForFriend);
        }
    } else {
        // Always show "Remove Friend" (deletes entire friendship)
        if (renderer->MenuItem("Remove Friend")) {
            emitCommand(WindowCommandType::RemoveFriend, friendName);
        }
        
        // Show "Remove From Alt View" only when shareFriendsAcrossAlts is disabled
        // This option only removes the friend from current character's view
        if (!shareFriendsAcrossAlts_) {
            if (renderer->MenuItem("Remove From Alt View")) {
                emitCommand(WindowCommandType::RemoveFriendVisibility, friendName);
            }
        }
    }
    

    if (renderer->MenuItem("View Details")) {
        emitCommand(WindowCommandType::ViewFriendDetails, friendName);
    }
    
    renderer->Separator();
    
    if (renderer->MenuItem("Edit Note")) {
        emitCommand(WindowCommandType::OpenNoteEditor, friendName);
    }
    
    if (renderer->MenuItem("Delete Note")) {
        emitCommand(WindowCommandType::DeleteNote, friendName);
    }
    
    renderer->Separator();
    
    if (renderer->MenuItem("Show Friended As", nullptr, viewModel_->getShowFriendedAsColumn())) {
        viewModel_->setShowFriendedAsColumn(!viewModel_->getShowFriendedAsColumn());
        std::vector<std::pair<std::string, std::string>> fields;
        fields.push_back({ "scope", XIFriendList::Protocol::JsonUtils::encodeString(spec_.commandScope) });
        fields.push_back({ "column", XIFriendList::Protocol::JsonUtils::encodeString("friended_as") });
        emitCommand(WindowCommandType::ToggleColumnVisibility, XIFriendList::Protocol::JsonUtils::encodeObject(fields));
    }
}

void FriendTableWidget::render(const std::vector<size_t>* rowIndexMap) {
    if (viewModel_ == nullptr) {
        return;
    }

    if (spec_.showSectionHeader) {
        SectionHeaderSpec headerSpec;
        headerSpec.label = spec_.sectionHeaderLabel;
        headerSpec.id = spec_.sectionHeaderId;
        headerSpec.visible = true;
        createSectionHeader(headerSpec);
    }


    const bool flagsChanged = !cachedVisibleColumnsValid_ ||
        viewSettings_.showJob != lastViewSettings_.showJob ||
        viewSettings_.showZone != lastViewSettings_.showZone ||
        viewSettings_.showNationRank != lastViewSettings_.showNationRank ||
        viewSettings_.showLastSeen != lastViewSettings_.showLastSeen;

    if (flagsChanged) {
        cachedVisibleColumns_.clear();
        cachedVisibleColumns_.reserve(6);
        cachedVisibleColumns_.push_back(TableColumnSpec("Name", "name"));
        if (viewSettings_.showJob) {
            cachedVisibleColumns_.push_back(TableColumnSpec("Job", "job"));
        }
        if (viewSettings_.showZone) {
            cachedVisibleColumns_.push_back(TableColumnSpec("Zone", "zone"));
        }
        if (viewSettings_.showNationRank) {
            cachedVisibleColumns_.push_back(TableColumnSpec("Nation/Rank", "nation_rank"));
        }
        if (viewSettings_.showLastSeen) {
            cachedVisibleColumns_.push_back(TableColumnSpec("Last Seen", "last_seen"));
        }

        lastViewSettings_ = viewSettings_;
        cachedVisibleColumnsValid_ = true;
    }

    const std::vector<TableColumnSpec>& visibleColumns = cachedVisibleColumns_;
    const auto& rows = viewModel_->getFriendRows();

    const size_t rowCount = rowIndexMap ? rowIndexMap->size() : rows.size();

    // We render all table cells via the cellRenderer below, so the default rowRenderer should
    // never be used. Keep a minimal implementation as a fallback for safety.
    TableRowRenderer rowRenderer = [](size_t /*displayRow*/) -> std::vector<std::string> {
        return {};
    };

    TableCellRenderer cellRenderer = [this, &rows, rowIndexMap](size_t displayRow, size_t /*colIndex*/, const std::string& colId) -> bool {
        if (!viewModel_) {
            return false;
        }
        const size_t rowIndex = rowIndexMap ? (*rowIndexMap)[displayRow] : displayRow;
        if (rowIndex >= rows.size()) {
            return false;
        }

        const auto& row = rows[rowIndex];
        IUiRenderer* renderer = GetUiRenderer();
        if (!renderer) {
            return false;
        }

        const bool isOffline = (!row.isOnline && !row.isPending);

        if (colId == "name") {
            void* iconHandle = nullptr;
            if (iconManager_) {
                auto* iconMgr = static_cast<XIFriendList::Platform::Ashita::IconManager*>(iconManager_);
                if (row.isPending) {
                    iconHandle = iconMgr->getIcon(XIFriendList::Platform::Ashita::IconType::Pending);
                } else if (row.isOnline) {
                    iconHandle = iconMgr->getIcon(XIFriendList::Platform::Ashita::IconType::Online);
                } else {
                    iconHandle = iconMgr->getIcon(XIFriendList::Platform::Ashita::IconType::Offline);
                }
            }

            // Render icon if available (dim when offline)
            // Add a single leading space before the icon for visual padding/alignment.
            renderer->TextUnformatted(" ");
            renderer->SameLine(0.0f, 0.0f);
            if (iconHandle) {
                ImVec2 size(12.0f, 12.0f);
                ImVec4 tint(1.0f, 1.0f, 1.0f, 1.0f);
                if (isOffline) {
                    // Offline icon is now grayscale (created in IconManager); keep it visible but clearly "disabled".
                    tint = ImVec4(0.70f, 0.70f, 0.70f, 1.0f);
                }
                renderer->Image(iconHandle, size, ImVec2(0, 0), ImVec2(1, 1), tint);
                renderer->SameLine(0.0f, 6.0f);
            } else {
                renderer->TextUnformatted(" "); // basic padding alignment
                renderer->SameLine(0.0f, 6.0f);
            }

            // Stable ID scope for popup behavior
            std::string friendNameId = "friend_" + row.name;
            renderer->PushID(friendNameId.c_str());

            const std::string capitalizedName = capitalizeWords(row.name);
            if (isOffline) {
                renderer->TextDisabled("%s", capitalizedName.c_str());
            } else {
                renderer->TextUnformatted(capitalizedName.c_str());
            }

            const bool nameLeftClicked = renderer->IsItemClicked(0);
            const bool nameRightClicked = renderer->IsItemClicked(1);

            if (nameLeftClicked) {
                emitCommand(WindowCommandType::OpenNoteEditor, row.name);
            }

            std::string contextMenuId = "##context_" + row.name;
            if (nameRightClicked) {
                renderer->OpenPopup(contextMenuId.c_str());
            }

            if (renderer->BeginPopup(contextMenuId.c_str())) {
                renderContextMenuItems(renderer, row.name);
                renderer->EndPopup();
            }

            renderer->PopID();
            return true;
        }

        std::string text;
        if (colId == "friended_as") {
            text = capitalizeWords(row.friendedAs);
        } else if (colId == "job") {
            text = row.jobText;
        } else if (colId == "zone") {
            text = row.zoneText;
        } else if (colId == "nation_rank") {
            if (row.nation >= 0 && row.nation <= 3 && iconManager_) {
                XIFriendList::Platform::Ashita::IconManager* iconMgr = static_cast<XIFriendList::Platform::Ashita::IconManager*>(iconManager_);
                void* nationIcon = nullptr;
                XIFriendList::Platform::Ashita::IconType iconType;
                
                switch (row.nation) {
                    case 0: iconType = XIFriendList::Platform::Ashita::IconType::NationSandy; break;
                    case 1: iconType = XIFriendList::Platform::Ashita::IconType::NationBastok; break;
                    case 2: iconType = XIFriendList::Platform::Ashita::IconType::NationWindurst; break;
                    case 3: iconType = XIFriendList::Platform::Ashita::IconType::NationJeuno; break;
                    default: iconType = XIFriendList::Platform::Ashita::IconType::NationSandy; break;
                }
                
                nationIcon = iconMgr->getIcon(iconType);
                
                if (nationIcon && row.nation != 3) {
                    UI::ImVec2 iconSize(13.0f, 13.0f);
                    renderer->Image(nationIcon, iconSize);
                    renderer->SameLine(0.0f, 4.0f);
                    if (!row.nationRankText.empty() && row.nationRankText != "Hidden") {
                        size_t spacePos = row.nationRankText.find(' ');
                        if (spacePos != std::string::npos && spacePos + 1 < row.nationRankText.length()) {
                            text = row.nationRankText.substr(spacePos + 1);
                        } else {
                            text = row.nationRankText;
                        }
                    } else if (!row.rankText.empty() && row.rankText != "Hidden") {
                        text = row.rankText;
                    } else {
                        text = "Hidden";
                    }
                } else {
                    text = row.nationRankText;
                }
            } else {
                text = row.nationRankText;
            }
        } else if (colId == "last_seen") {
            text = row.lastSeenText;
        } else {
            // Not handled here; fall back to default table rendering.
            return false;
        }

        if (isOffline) {
            renderer->TextDisabled("%s", text.c_str());
        } else {
            renderer->TextUnformatted(text.c_str());
        }
        return true;
    };

    TableSpec tableSpec;
    tableSpec.id = spec_.tableId;
    tableSpec.visible = true;
    tableSpec.columns = visibleColumns;
    tableSpec.rowCount = rowCount;
    tableSpec.rowRenderer = rowRenderer;
    tableSpec.cellRenderer = cellRenderer;
    tableSpec.sortable = true;
    tableSpec.sortColumn = &sortColumn_;
    tableSpec.sortAscending = &sortAscending_;
    tableSpec.showHeaders = false;

    createTable(tableSpec);
}

// ContextMenu widget implementation

bool createContextMenu(const ContextMenuSpec& spec) {
    if (!spec.visible || spec.items.empty()) {
        return false;
    }
    
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr) {
        return false;
    }
    
    // Use PushID for stable widget IDs
    renderer->PushID(spec.id.c_str());
    
    bool itemClicked = false;
    
    // Begin popup (must be called after OpenPopup)
    if (renderer->BeginPopup(spec.id.c_str())) {
        // Render menu items
        for (const auto& item : spec.items) {
            if (!item.visible) {
                continue;
            }
            
            renderer->PushID(item.id.c_str());
            
            // Render menu item
            if (renderer->MenuItem(item.label.c_str(), nullptr, false, item.enabled)) {
                if (item.enabled && item.onClick) {
                    item.onClick();
                    itemClicked = true;
                }
            }
            
            renderer->PopID();
        }
        
        renderer->EndPopup();
    }
    
    renderer->PopID();
    
    return itemClicked;
}

// Helper to open context menu (call this when right-click is detected)
void openContextMenu(const std::string& menuId) {
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer != nullptr) {
        renderer->OpenPopup(menuId.c_str());
    }
}

XIFriendList::Core::MemoryStats FriendTableWidget::getMemoryStats() const {
    size_t bytes = sizeof(FriendTableWidget);
    
    bytes += spec_.tableId.capacity();
    bytes += spec_.toggleRowId.capacity();
    bytes += spec_.sectionHeaderId.capacity();
    bytes += spec_.sectionHeaderLabel.capacity();
    bytes += spec_.commandScope.capacity();
    
    bytes += cachedVisibleColumns_.capacity() * sizeof(TableColumnSpec);
    for (const auto& col : cachedVisibleColumns_) {
        bytes += col.id.capacity();
    }
    
    return XIFriendList::Core::MemoryStats(cachedVisibleColumns_.size(), bytes, "FriendTableWidget");
}

} // namespace UI
} // namespace XIFriendList

