
#ifndef PLATFORM_ASHITA_ASHITA_UI_RENDERER_H
#define PLATFORM_ASHITA_ASHITA_UI_RENDERER_H

#include "UI/Interfaces/IUiRenderer.h"

struct IGuiManager;

namespace XIFriendList {
namespace Platform {
namespace Ashita {

class AshitaUiRenderer : public ::XIFriendList::UI::IUiRenderer {
public:
    explicit AshitaUiRenderer(IGuiManager* guiManager);
    void PushID(const char* id) override;
    void PopID() override;
    void SameLine(float offset_from_start_x = 0.0f, float spacing = -1.0f) override;
    void NewLine() override;
    void Spacing(float verticalSpacing = 0.0f) override;
    bool Button(const char* label, const ::XIFriendList::UI::ImVec2& size = ::XIFriendList::UI::ImVec2(0, 0)) override;
    bool Checkbox(const char* label, bool* v) override;
    void TextUnformatted(const char* text) override;
    void Text(const char* fmt, ...) override;
    void TextDisabled(const char* fmt, ...) override;
    void Image(void* textureId, const ::XIFriendList::UI::ImVec2& size, const ::XIFriendList::UI::ImVec2& uv0 = ::XIFriendList::UI::ImVec2(0, 0), const ::XIFriendList::UI::ImVec2& uv1 = ::XIFriendList::UI::ImVec2(1, 1), const ::XIFriendList::UI::ImVec4& tintCol = ::XIFriendList::UI::ImVec4(1, 1, 1, 1), const ::XIFriendList::UI::ImVec4& borderCol = ::XIFriendList::UI::ImVec4(0, 0, 0, 0)) override;
    bool InputText(const char* label, char* buf, size_t buf_size, int flags = 0) override;
    bool InputTextMultiline(const char* label, char* buf, size_t buf_size, const ::XIFriendList::UI::ImVec2& size = ::XIFriendList::UI::ImVec2(0, 0), int flags = 0) override;
    bool SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format = "%.3f") override;
    bool ColorEdit4(const char* label, float col[4], int flags = 0) override;
    bool MenuItem(const char* label, const char* shortcut = nullptr, bool selected = false, bool enabled = true) override;
    bool BeginCombo(const char* label, const char* preview_value, int flags = 0) override;
    void EndCombo() override;
    bool Selectable(const char* label, bool selected = false, int flags = 0, const ::XIFriendList::UI::ImVec2& size = ::XIFriendList::UI::ImVec2(0, 0)) override;
    bool BeginTable(const char* str_id, int column, int flags = 0, const ::XIFriendList::UI::ImVec2& outer_size = ::XIFriendList::UI::ImVec2(0, 0), float inner_width = 0.0f) override;
    void EndTable() override;
    void TableNextRow(int row_flags = 0, float min_row_height = 0.0f) override;
    void TableNextColumn() override;
    void TableSetColumnIndex(int column_n) override;
    void TableHeader(const char* label) override;
    void TableSetupColumn(const char* label, int flags = 0, float init_width_or_weight = 0.0f, unsigned int user_id = 0) override;
    
    // Windows
    void SetNextWindowPos(const ::XIFriendList::UI::ImVec2& pos, int cond = 0) override;
    void SetNextWindowSize(const ::XIFriendList::UI::ImVec2& size, int cond = 0) override;
    void SetNextWindowBgAlpha(float alpha) override;
    float GetWindowBgAlpha() override;
    bool Begin(const char* name, bool* p_open = nullptr, int flags = 0) override;
    void End() override;

    // Child windows
    bool BeginChild(const char* str_id, const ::XIFriendList::UI::ImVec2& size = ::XIFriendList::UI::ImVec2(0, 0), bool border = false, int flags = 0) override;
    void EndChild() override;
    
    // Popups
    void OpenPopup(const char* str_id) override;
    bool BeginPopup(const char* str_id) override;
    void EndPopup() override;
    bool BeginPopupContextWindow(const char* str_id = nullptr, int mouse_button = 1) override;
    
    // Section headers
    bool CollapsingHeader(const char* label, bool* p_open = nullptr) override;
    void Separator() override;
    
    // State queries
    bool IsItemHovered() override;
    bool IsItemActive() override;
    bool IsItemDeactivatedAfterEdit() override;
    bool IsItemClicked(int button = 0) override;
    bool IsAnyPopupOpen() override;
    ::XIFriendList::UI::ImVec2 GetContentRegionAvail() override;
    ::XIFriendList::UI::ImVec2 CalcTextSize(const char* text) override;
    void PushTextWrapPos(float wrap_pos_x = 0.0f) override;
    void PopTextWrapPos() override;

private:
    IGuiManager* guiManager_;

    // We apply a tiny global content indent for every top-level ImGui window (Begin/End)
    // to give the UI a bit of breathing room on the left edge. This does not affect the
    // window background coloring; it only shifts the content cursor region.
    int windowIndentDepth_ = 0;
    
    // Text wrapping state
    float textWrapPos_ = 0.0f;
    int textWrapStackDepth_ = 0;
};

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

#endif // PLATFORM_ASHITA_ASHITA_UI_RENDERER_H

