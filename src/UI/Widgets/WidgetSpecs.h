// Widget specification structures for declarative UI construction

#ifndef UI_WIDGET_SPECS_H
#define UI_WIDGET_SPECS_H

#include <string>
#include <functional>
#include <vector>
#include <cstdint>

namespace XIFriendList {
namespace UI {

// Button specification
struct ButtonSpec {
    std::string label;
    std::string id;  // Unique identifier for the button
    float width = 0.0f;  // 0 = auto
    float height = 0.0f;  // 0 = auto
    bool enabled = true;
    bool visible = true;
    std::function<void()> onClick;  // Callback when clicked
    
    ButtonSpec() = default;
    ButtonSpec(const std::string& label, const std::string& id)
        : label(label), id(id) {}
};

// Toggle specification
struct ToggleSpec {
    std::string label;
    std::string id;
    bool* value;  // Pointer to boolean value (must remain valid)
    bool enabled = true;
    bool visible = true;
    std::function<void()> onChange;  // Callback when toggled
    
    ToggleSpec() : value(nullptr) {}
    ToggleSpec(const std::string& label, const std::string& id, bool* value)
        : label(label), id(id), value(value) {}
};

// Input text specification
struct InputTextSpec {
    std::string label;
    std::string id;
    std::string* buffer;  // Pointer to string buffer (must remain valid, must have capacity)
    size_t bufferSize = 256;  // Buffer capacity
    bool enabled = true;
    bool visible = true;
    bool readOnly = false;
    // Placeholder text support removed: Windows API SetCursorPos macro conflicts with ImGui::SetCursorPos
    // SDK doesn't expose InputTextWithHint, and overlay approach requires SetCursorPos which conflicts
    // std::string placeholder;  // Deferred until SDK exposes InputTextWithHint or workaround found
    std::function<void(const std::string&)> onChange;  // Callback when text changes
    std::function<void(const std::string&)> onEnter;  // Callback when Enter is pressed
    
    InputTextSpec() : buffer(nullptr) {}
    InputTextSpec(const std::string& label, const std::string& id, std::string* buffer)
        : label(label), id(id), buffer(buffer) {}
};

// Input text multiline specification
struct InputTextMultilineSpec {
    std::string label;
    std::string id;
    std::string* buffer;  // Pointer to string buffer (must remain valid, must have capacity)
    size_t bufferSize = 256;  // Buffer capacity
    float width = 0.0f;  // Width of text area (0 = auto/full width)
    float height = 0.0f;  // Height of text area (0 = auto)
    bool enabled = true;
    bool visible = true;
    bool readOnly = false;
    std::function<void(const std::string&)> onChange;  // Callback when text changes
    
    InputTextMultilineSpec() : buffer(nullptr) {}
    InputTextMultilineSpec(const std::string& label, const std::string& id, std::string* buffer)
        : label(label), id(id), buffer(buffer) {}
};

struct TableColumnSpec {
    std::string header;
    std::string id;
    float width = 0.0f;  // 0 = auto
    bool sortable = false;
    
    TableColumnSpec() = default;
    TableColumnSpec(const std::string& header, const std::string& id)
        : header(header), id(id) {}
};

// Takes row index and returns vector of cell strings
// Note: Returned strings should be references to cached data when possible
using TableRowRenderer = std::function<std::vector<std::string>(size_t rowIndex)>;

// Custom cell renderer function type
// Takes row index, column index, and column ID
// Returns true if cell was rendered, false to use default text rendering
using TableCellRenderer = std::function<bool(size_t rowIndex, size_t colIndex, const std::string& colId)>;

struct TableSpec {
    std::string id;
    std::vector<TableColumnSpec> columns;
    size_t rowCount = 0;
    TableRowRenderer rowRenderer;
    bool sortable = false;
    int* sortColumn = nullptr;  // Pointer to column index for sorting (-1 = none)
    bool* sortAscending = nullptr;  // Pointer to sort direction
    bool enabled = true;
    bool visible = true;
    bool showHeaders = true;  // Whether to render header row (default: true)
    std::function<void(size_t rowIndex)> onRowClick;  // Callback when row is clicked (left-click)
    std::function<void(size_t rowIndex, int columnIndex)> onCellClick;  // Callback when cell is clicked (left-click)
    std::function<void(size_t rowIndex)> onRowRightClick;  // Callback when row is right-clicked (for context menu)
    std::function<void()> headerContextMenu;  // Optional header right-click menu (rendered inside a popup)
    TableCellRenderer cellRenderer;  // Optional custom cell renderer (returns true if rendered, false to use default)
    
    TableSpec() : sortColumn(nullptr), sortAscending(nullptr) {}
    TableSpec(const std::string& id, const std::vector<TableColumnSpec>& columns)
        : id(id), columns(columns) {}
};

// Section header specification
struct SectionHeaderSpec {
    std::string label;
    std::string id;
    bool collapsible = false;
    bool* collapsed = nullptr;  // Pointer to collapsed state (if collapsible)
    bool visible = true;
    
    SectionHeaderSpec() : collapsed(nullptr) {}
    SectionHeaderSpec(const std::string& label, const std::string& id)
        : label(label), id(id) {}
};

// Slider specification (float)
struct SliderSpec {
    std::string label;
    std::string id;
    float* value;  // Pointer to float value (must remain valid)
    float min = 0.0f;
    float max = 1.0f;
    const char* format = "%.2f";  // Format string for display
    bool enabled = true;
    bool visible = true;
    std::function<void(float)> onChange;  // Callback when value changes (called immediately on change)
    std::function<void(float)> onDeactivated;  // Callback when slider is deactivated after edit (for debounced save)
    
    SliderSpec() : value(nullptr) {}
    SliderSpec(const std::string& label, const std::string& id, float* value)
        : label(label), id(id), value(value) {}
};

// Color picker specification (RGBA)
struct ColorPickerSpec {
    std::string label;
    std::string id;
    float* color;  // Pointer to float[4] array (R, G, B, A) (must remain valid)
    bool showAlpha = true;  // Show alpha channel
    bool enabled = true;
    bool visible = true;
    std::function<void()> onChange;  // Callback when color changes
    
    ColorPickerSpec() : color(nullptr) {}
    ColorPickerSpec(const std::string& label, const std::string& id, float* color)
        : label(label), id(id), color(color) {}
};

} // namespace UI
} // namespace XIFriendList

#endif // UI_WIDGET_SPECS_H

