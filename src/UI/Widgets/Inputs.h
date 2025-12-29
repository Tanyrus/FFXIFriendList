// Consolidated input widgets: InputText, InputTextMultiline, Combo, Slider, ColorPicker

#ifndef UI_INPUTS_H
#define UI_INPUTS_H

#include "WidgetSpecs.h"

namespace XIFriendList {
namespace UI {

// InputText widget

// Returns true if text was changed
bool createInputText(const InputTextSpec& spec);

// Used to prevent game from processing Enter key when typing in input boxes
bool IsAnyInputActive();

// InputTextMultiline widget

// Returns true if text was changed
bool createInputTextMultiline(const InputTextMultilineSpec& spec);

// Combo widget

// Combo box specification
struct ComboSpec {
    std::string label;
    std::string id;
    int* currentItem;  // Pointer to current selected index (must remain valid)
    std::vector<std::string> items;  // List of items to display
    bool enabled = true;
    bool visible = true;
    std::function<void(int)> onChange;  // Callback when selection changes (receives new index)
    
    ComboSpec() : currentItem(nullptr) {}
    ComboSpec(const std::string& label, const std::string& id, int* currentItem)
        : label(label), id(id), currentItem(currentItem) {}
};

// Returns true if selection was changed
bool createCombo(const ComboSpec& spec);

// Slider widget

// Returns true if value was changed
bool createSlider(const SliderSpec& spec);

// ColorPicker widget

// Returns true if color was changed
// color must point to float[4] array (R, G, B, A)
bool createColorPicker(const ColorPickerSpec& spec);

} // namespace UI
} // namespace XIFriendList

#endif // UI_INPUTS_H

