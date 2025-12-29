// UI rendering interface - abstracts ImGui operations from Platform layer

#ifndef UI_INTERFACES_I_UI_RENDERER_H
#define UI_INTERFACES_I_UI_RENDERER_H

#include <string>
#include <cstddef>

namespace XIFriendList {
namespace UI {

// Forward declarations for ImGui types (avoid including ImGui headers in interface)
struct ImVec2 {
    float x, y;
    ImVec2() : x(0), y(0) {}
    ImVec2(float _x, float _y) : x(_x), y(_y) {}
};

struct ImVec4 {
    float x, y, z, w;
    ImVec4() : x(0), y(0), z(0), w(0) {}
    ImVec4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
};

// UI rendering interface
// Abstracts ImGui operations so UI layer doesn't depend on Platform/Ashita
class IUiRenderer {
public:
    virtual ~IUiRenderer() = default;
    
    // ID management
    virtual void PushID(const char* id) = 0;
    virtual void PopID() = 0;
    
    // Layout
    virtual void SameLine(float offset_from_start_x = 0.0f, float spacing = -1.0f) = 0;
    virtual void NewLine() = 0;
    virtual void Spacing(float verticalSpacing = 0.0f) = 0;  // Add vertical spacing (in pixels)
    
    // Widgets
    virtual bool Button(const char* label, const ImVec2& size = ImVec2(0, 0)) = 0;
    virtual bool Checkbox(const char* label, bool* v) = 0;
    virtual void TextUnformatted(const char* text) = 0;
    virtual void Text(const char* fmt, ...) = 0;
    virtual void TextDisabled(const char* fmt, ...) = 0;
    virtual void Image(void* textureId, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tintCol = ImVec4(1, 1, 1, 1), const ImVec4& borderCol = ImVec4(0, 0, 0, 0)) = 0;
    virtual bool InputText(const char* label, char* buf, size_t buf_size, int flags = 0) = 0;
    virtual bool InputTextMultiline(const char* label, char* buf, size_t buf_size, const ImVec2& size = ImVec2(0, 0), int flags = 0) = 0;
    virtual bool SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format = "%.3f") = 0;
    virtual bool ColorEdit4(const char* label, float col[4], int flags = 0) = 0;
    virtual bool MenuItem(const char* label, const char* shortcut = nullptr, bool selected = false, bool enabled = true) = 0;
    
    // Combo/Dropdown
    virtual bool BeginCombo(const char* label, const char* preview_value, int flags = 0) = 0;
    virtual void EndCombo() = 0;
    virtual bool Selectable(const char* label, bool selected = false, int flags = 0, const ImVec2& size = ImVec2(0, 0)) = 0;
    
    virtual bool BeginTable(const char* str_id, int column, int flags = 0, const ImVec2& outer_size = ImVec2(0, 0), float inner_width = 0.0f) = 0;
    virtual void EndTable() = 0;
    virtual void TableNextRow(int row_flags = 0, float min_row_height = 0.0f) = 0;
    virtual void TableNextColumn() = 0;
    virtual void TableSetColumnIndex(int column_n) = 0;
    virtual void TableHeader(const char* label) = 0;
    virtual void TableSetupColumn(const char* label, int flags = 0, float init_width_or_weight = 0.0f, unsigned int user_id = 0) = 0;
    
    // Windows
    virtual void SetNextWindowPos(const ImVec2& pos, int cond = 0) = 0;
    virtual void SetNextWindowSize(const ImVec2& size, int cond = 0) = 0;
    virtual void SetNextWindowBgAlpha(float alpha) = 0;
    virtual float GetWindowBgAlpha() = 0;  // Get current window background alpha
    virtual bool Begin(const char* name, bool* p_open = nullptr, int flags = 0) = 0;
    virtual void End() = 0;

    // Child windows (used for pinned footer layouts)
    virtual bool BeginChild(const char* str_id, const ImVec2& size = ImVec2(0, 0), bool border = false, int flags = 0) = 0;
    virtual void EndChild() = 0;
    
    // Popups
    virtual void OpenPopup(const char* str_id) = 0;
    virtual bool BeginPopup(const char* str_id) = 0;
    virtual void EndPopup() = 0;
    virtual bool BeginPopupContextWindow(const char* str_id = nullptr, int mouse_button = 1) = 0;  // Context menu for window (right-click on title bar or empty area)
    
    // Section headers
    virtual bool CollapsingHeader(const char* label, bool* p_open = nullptr) = 0;
    virtual void Separator() = 0;
    
    // State queries
    virtual bool IsItemHovered() = 0;
    virtual bool IsItemActive() = 0;  // Check if item is currently active (focused)
    virtual bool IsItemDeactivatedAfterEdit() = 0;
    virtual bool IsItemClicked(int button = 0) = 0;

    // Global popup/menu state
    // Used to prevent closing plugin windows while context menus/popups are open.
    virtual bool IsAnyPopupOpen() = 0;
    virtual ImVec2 GetContentRegionAvail() = 0;
    virtual ImVec2 CalcTextSize(const char* text) = 0;
    
    // Text wrapping
    virtual void PushTextWrapPos(float wrap_pos_x = 0.0f) = 0;  // 0.0f = wrap to end of window
    virtual void PopTextWrapPos() = 0;
};

// Global renderer access (set by Platform layer, used by UI widgets)
// This allows widgets to access renderer without changing function signatures
IUiRenderer* GetUiRenderer();
void SetUiRenderer(IUiRenderer* renderer);

} // namespace UI
} // namespace XIFriendList

#endif // UI_INTERFACES_I_UI_RENDERER_H

