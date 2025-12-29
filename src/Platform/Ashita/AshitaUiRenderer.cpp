#include "AshitaUiRenderer.h"
#include <Ashita.h>
#include <cstdarg>
#include <vector>
#include <string>
#ifndef BUILDING_TESTS
#include <imgui.h>
#endif

namespace XIFriendList {
namespace Platform {
namespace Ashita {

AshitaUiRenderer::AshitaUiRenderer(IGuiManager* guiManager)
    : guiManager_(guiManager)
    , textWrapPos_(0.0f)
    , textWrapStackDepth_(0)
{
}

void AshitaUiRenderer::PushID(const char* id) {
    if (guiManager_) {
        guiManager_->PushID(id);
    }
}

void AshitaUiRenderer::PopID() {
    if (guiManager_) {
        guiManager_->PopID();
    }
}

void AshitaUiRenderer::SameLine(float offset_from_start_x, float spacing) {
    if (guiManager_) {
        guiManager_->SameLine(offset_from_start_x, spacing);
    }
}

void AshitaUiRenderer::NewLine() {
}

void AshitaUiRenderer::Spacing(float verticalSpacing) {
    if (guiManager_ && verticalSpacing > 0.0f) {
#ifndef BUILDING_TESTS
        guiManager_->TextUnformatted("");
        if (verticalSpacing > 5.0f) {
            guiManager_->TextUnformatted("");
        }
#else
        (void)verticalSpacing;
#endif
    }
}

bool AshitaUiRenderer::Button(const char* label, const XIFriendList::UI::ImVec2& size) {
    if (!guiManager_) {
        return false;
    }
    ImVec2 imguiSize(size.x, size.y);
    return guiManager_->Button(label, imguiSize);
}

bool AshitaUiRenderer::Checkbox(const char* label, bool* v) {
    if (!guiManager_) {
        return false;
    }
    return guiManager_->Checkbox(label, v);
}

void AshitaUiRenderer::TextUnformatted(const char* text) {
    if (!guiManager_ || !text) {
        return;
    }
    
    // If text wrapping is enabled, manually wrap the text
    if (textWrapStackDepth_ > 0 && textWrapPos_ > 0.0f) {
        std::string textStr(text);
        float wrapWidth = textWrapPos_;
        
        // Calculate available width (use wrap position or content region)
        ImVec2 avail = guiManager_->GetContentRegionAvail();
        if (wrapWidth <= 0.0f || (avail.x > 0.0f && avail.x < wrapWidth)) {
            wrapWidth = avail.x > 0.0f ? avail.x : wrapWidth;
        }
        
        if (wrapWidth > 0.0f) {
            // Split text into lines that fit within wrapWidth
            std::vector<std::string> lines;
            std::string currentLine;
            std::string currentWord;
            
            for (size_t i = 0; i < textStr.length(); ++i) {
                char c = textStr[i];
                
                if (c == ' ' || c == '\t' || c == '\n') {
                    if (!currentWord.empty()) {
                        // Check if the word itself fits
                        ImVec2 wordSize = guiManager_->CalcTextSize(currentWord.c_str());
                        
                        if (wordSize.x > wrapWidth) {
                            // Very long word - need to break it (or just render it and let it overflow)
                            // For now, we'll render it on its own line
                            if (!currentLine.empty()) {
                                lines.push_back(currentLine);
                                currentLine.clear();
                            }
                            currentLine = currentWord;
                        } else {
                            // Check if adding this word would exceed wrap width
                            std::string testLine = currentLine.empty() ? currentWord : currentLine + " " + currentWord;
                            ImVec2 testSize = guiManager_->CalcTextSize(testLine.c_str());
                            
                            if (testSize.x > wrapWidth && !currentLine.empty()) {
                                // Word doesn't fit, start new line
                                lines.push_back(currentLine);
                                currentLine = currentWord;
                            } else {
                                // Word fits, add to current line
                                if (!currentLine.empty()) {
                                    currentLine += " ";
                                }
                                currentLine += currentWord;
                            }
                        }
                        currentWord.clear();
                    }
                    
                    if (c == '\n') {
                        // Explicit line break
                        if (!currentLine.empty()) {
                            lines.push_back(currentLine);
                            currentLine.clear();
                        }
                    }
                } else {
                    currentWord += c;
                }
            }
            
            // Add remaining word and line
            if (!currentWord.empty()) {
                // Check if the word itself fits
                ImVec2 wordSize = guiManager_->CalcTextSize(currentWord.c_str());
                
                if (wordSize.x > wrapWidth) {
                    // Very long word
                    if (!currentLine.empty()) {
                        lines.push_back(currentLine);
                        currentLine.clear();
                    }
                    currentLine = currentWord;
                } else {
                    std::string testLine = currentLine.empty() ? currentWord : currentLine + " " + currentWord;
                    ImVec2 testSize = guiManager_->CalcTextSize(testLine.c_str());
                    
                    if (testSize.x > wrapWidth && !currentLine.empty()) {
                        lines.push_back(currentLine);
                        currentLine = currentWord;
                    } else {
                        if (!currentLine.empty()) {
                            currentLine += " ";
                        }
                        currentLine += currentWord;
                    }
                }
            }
            if (!currentLine.empty()) {
                lines.push_back(currentLine);
            }
            
            // Render each line
            for (size_t i = 0; i < lines.size(); ++i) {
                guiManager_->TextUnformatted(lines[i].c_str());
                if (i < lines.size() - 1) {
                    // Add newline between lines (ImGui will handle spacing)
                }
            }
        } else {
            // No wrap width, render normally
            guiManager_->TextUnformatted(text);
        }
    } else {
        // No wrapping, render normally
        guiManager_->TextUnformatted(text);
    }
}

void AshitaUiRenderer::Text(const char* fmt, ...) {
    if (!guiManager_) {
        return;
    }
    va_list args;
    va_start(args, fmt);
    guiManager_->TextV(fmt, args);
    va_end(args);
}

void AshitaUiRenderer::TextDisabled(const char* fmt, ...) {
    if (!guiManager_) {
        return;
    }
    va_list args;
    va_start(args, fmt);
    guiManager_->TextDisabledV(fmt, args);
    va_end(args);
}

bool AshitaUiRenderer::InputText(const char* label, char* buf, size_t buf_size, int flags) {
    if (!guiManager_) {
        return false;
    }
    return guiManager_->InputText(label, buf, static_cast<unsigned int>(buf_size), flags);
}

bool AshitaUiRenderer::InputTextMultiline(const char* label, char* buf, size_t buf_size, const XIFriendList::UI::ImVec2& size, int flags) {
    if (!guiManager_) {
        return false;
    }
    ImVec2 imguiSize(size.x, size.y);
    return guiManager_->InputTextMultiline(label, buf, static_cast<unsigned int>(buf_size), imguiSize, flags);
}

bool AshitaUiRenderer::SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format) {
    if (!guiManager_) {
        return false;
    }
    return guiManager_->SliderFloat(label, v, v_min, v_max, format);
}

bool AshitaUiRenderer::ColorEdit4(const char* label, float col[4], int flags) {
    if (!guiManager_) {
        return false;
    }
    return guiManager_->ColorEdit4(label, col, flags);
}

void AshitaUiRenderer::Image(void* textureId, const XIFriendList::UI::ImVec2& size, const XIFriendList::UI::ImVec2& uv0, const XIFriendList::UI::ImVec2& uv1, const XIFriendList::UI::ImVec4& tintCol, const XIFriendList::UI::ImVec4& borderCol) {
    if (!guiManager_ || !textureId) {
        return;
    }
    
    ImVec2 imguiSize(size.x, size.y);
    ImVec2 imguiUv0(uv0.x, uv0.y);
    ImVec2 imguiUv1(uv1.x, uv1.y);
    ImVec4 imguiTintCol(tintCol.x, tintCol.y, tintCol.z, tintCol.w);
    
    guiManager_->Image(textureId, imguiSize, imguiUv0, imguiUv1, imguiTintCol);
    (void)borderCol;
}

bool AshitaUiRenderer::MenuItem(const char* label, const char* shortcut, bool selected, bool enabled) {
    if (!guiManager_) {
        return false;
    }
    return guiManager_->MenuItem(label, shortcut, selected, enabled);
}

bool AshitaUiRenderer::BeginCombo(const char* label, const char* preview_value, int flags) {
    if (!guiManager_) {
        return false;
    }
    
#ifndef BUILDING_TESTS
    return guiManager_->BeginCombo(label, preview_value, flags);
#else
    (void)label;
    (void)preview_value;
    (void)flags;
    return false;
#endif
}

void AshitaUiRenderer::EndCombo() {
    if (!guiManager_) {
        return;
    }
    
#ifndef BUILDING_TESTS
    guiManager_->EndCombo();
#else
#endif
}

bool AshitaUiRenderer::Selectable(const char* label, bool selected, int flags, const XIFriendList::UI::ImVec2& size) {
    if (!guiManager_) {
        return false;
    }
    
#ifndef BUILDING_TESTS
    ImVec2 imguiSize(size.x, size.y);
    return guiManager_->Selectable(label, selected, flags, imguiSize);
#else
    (void)label;
    (void)selected;
    (void)flags;
    (void)size;
    return false;
#endif
}

bool AshitaUiRenderer::BeginTable(const char* str_id, int column, int flags, const XIFriendList::UI::ImVec2& outer_size, float inner_width) {
    if (!guiManager_) {
        return false;
    }
    ImVec2 imguiOuterSize(outer_size.x, outer_size.y);
    return guiManager_->BeginTable(str_id, column, flags, imguiOuterSize, inner_width);
}

void AshitaUiRenderer::EndTable() {
    if (guiManager_) {
        guiManager_->EndTable();
    }
}

void AshitaUiRenderer::TableNextRow(int row_flags, float min_row_height) {
    if (guiManager_) {
        guiManager_->TableNextRow(row_flags, min_row_height);
    }
}

void AshitaUiRenderer::TableNextColumn() {
    if (guiManager_) {
        guiManager_->TableNextColumn();
    }
}

void AshitaUiRenderer::TableSetupColumn(const char* label, int flags, float init_width_or_weight, unsigned int user_id) {
    if (guiManager_) {
        guiManager_->TableSetupColumn(label, flags, init_width_or_weight, user_id);
    }
}

void AshitaUiRenderer::TableSetColumnIndex(int column_n) {
    if (guiManager_) {
        guiManager_->TableSetColumnIndex(column_n);
    }
}

void AshitaUiRenderer::TableHeader(const char* label) {
    if (guiManager_) {
        guiManager_->TableHeader(label);
    }
}

void AshitaUiRenderer::SetNextWindowPos(const XIFriendList::UI::ImVec2& pos, int cond) {
    if (guiManager_) {
        ImVec2 imguiPos(pos.x, pos.y);
        guiManager_->SetNextWindowPos(imguiPos, cond);
    }
}

void AshitaUiRenderer::SetNextWindowSize(const XIFriendList::UI::ImVec2& size, int cond) {
    if (guiManager_) {
        ImVec2 imguiSize(size.x, size.y);
        guiManager_->SetNextWindowSize(imguiSize, cond);
    }
}

void AshitaUiRenderer::SetNextWindowBgAlpha(float alpha) {
#ifndef BUILDING_TESTS
    if (guiManager_) {
        guiManager_->SetNextWindowBgAlpha(alpha);
    }
#else
    (void)alpha;
#endif
}

float AshitaUiRenderer::GetWindowBgAlpha() {
#ifndef BUILDING_TESTS
    if (guiManager_) {
        ImGuiStyle& style = guiManager_->GetStyle();
        return style.Colors[ImGuiCol_WindowBg].w;
    }
#endif
    return 1.0f;
}

bool AshitaUiRenderer::Begin(const char* name, bool* p_open, int flags) {
    if (!guiManager_) {
        return false;
    }

    const bool began = guiManager_->Begin(name, p_open, flags);

    if (began) {
        constexpr float kWindowLeftPaddingPx = 1.0f;
        guiManager_->Indent(kWindowLeftPaddingPx);
        ++windowIndentDepth_;
    }

    return began;
}

void AshitaUiRenderer::End() {
    if (guiManager_) {
        if (windowIndentDepth_ > 0) {
            constexpr float kWindowLeftPaddingPx = 1.0f;
            guiManager_->Unindent(kWindowLeftPaddingPx);
            --windowIndentDepth_;
        }
        guiManager_->End();
    }
}

bool AshitaUiRenderer::BeginChild(const char* str_id, const XIFriendList::UI::ImVec2& size, bool border, int flags) {
    if (!guiManager_) {
        return false;
    }
    ImVec2 imguiSize(size.x, size.y);
    return guiManager_->BeginChild(str_id, imguiSize, border, flags);
}

void AshitaUiRenderer::EndChild() {
    if (guiManager_) {
        guiManager_->EndChild();
    }
}

void AshitaUiRenderer::OpenPopup(const char* str_id) {
    if (guiManager_) {
        guiManager_->OpenPopup(str_id);
    }
}

bool AshitaUiRenderer::BeginPopup(const char* str_id) {
    if (!guiManager_) {
        return false;
    }
    return guiManager_->BeginPopup(str_id);
}

void AshitaUiRenderer::EndPopup() {
    if (guiManager_) {
        guiManager_->EndPopup();
    }
}

bool AshitaUiRenderer::BeginPopupContextWindow(const char* str_id, int mouse_button) {
#ifndef BUILDING_TESTS
    if (!guiManager_) {
        return false;
    }
    return guiManager_->BeginPopupContextWindow(str_id, mouse_button);
#else
    (void)str_id;
    (void)mouse_button;
    return false;
#endif
}

bool AshitaUiRenderer::IsItemHovered() {
    if (!guiManager_) {
        return false;
    }
    return guiManager_->IsItemHovered();
}

bool AshitaUiRenderer::IsItemActive() {
    if (!guiManager_) {
        return false;
    }
    return guiManager_->IsItemActive();
}

bool AshitaUiRenderer::IsItemDeactivatedAfterEdit() {
    if (!guiManager_) {
        return false;
    }
    return guiManager_->IsItemDeactivatedAfterEdit();
}

bool AshitaUiRenderer::IsItemClicked(int button) {
    if (!guiManager_) {
        return false;
    }
    return guiManager_->IsItemClicked(button);
}

bool AshitaUiRenderer::IsAnyPopupOpen() {
#ifndef BUILDING_TESTS
    if (!guiManager_) {
        return false;
    }
    return guiManager_->IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
#else
    return false;
#endif
}

bool AshitaUiRenderer::CollapsingHeader(const char* label, bool* p_open) {
    if (!guiManager_) {
        return false;
    }
    return guiManager_->CollapsingHeader(label, p_open);
}

void AshitaUiRenderer::Separator() {
    if (guiManager_) {
        guiManager_->Separator();
    }
}

XIFriendList::UI::ImVec2 AshitaUiRenderer::GetContentRegionAvail() {
    if (!guiManager_) {
        return XIFriendList::UI::ImVec2(0, 0);
    }
    ImVec2 avail = guiManager_->GetContentRegionAvail();
    return XIFriendList::UI::ImVec2(avail.x, avail.y);
}

XIFriendList::UI::ImVec2 AshitaUiRenderer::CalcTextSize(const char* text) {
    if (!guiManager_) {
        return XIFriendList::UI::ImVec2(0, 0);
    }
    ImVec2 size = guiManager_->CalcTextSize(text);
    return XIFriendList::UI::ImVec2(size.x, size.y);
}

void AshitaUiRenderer::PushTextWrapPos(float wrap_pos_x) {
#ifndef BUILDING_TESTS
    if (guiManager_ && wrap_pos_x > 0.0f) {
        // Store wrap position for manual text wrapping
        // ImGui's PushTextWrapPos may not be available in Ashita SDK
        // We'll use the stored value for manual wrapping calculations
        textWrapPos_ = wrap_pos_x;
        textWrapStackDepth_++;
    }
#else
    (void)wrap_pos_x;
#endif
}

void AshitaUiRenderer::PopTextWrapPos() {
#ifndef BUILDING_TESTS
    if (textWrapStackDepth_ > 0) {
        textWrapStackDepth_--;
        if (textWrapStackDepth_ == 0) {
            textWrapPos_ = 0.0f;
        }
    }
#endif
}

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

