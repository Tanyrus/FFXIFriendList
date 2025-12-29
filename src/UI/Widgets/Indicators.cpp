// Consolidated indicator widgets implementation

#include "UI/Widgets/Indicators.h"
#include "UI/Interfaces/IUiRenderer.h"
#include "UI/Notifications/Notification.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace XIFriendList {
namespace UI {

// Text widget implementation

void createText(const TextSpec& spec) {
    if (!spec.visible || spec.text.empty()) {
        return;
    }
    
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr) {
        return;
    }
    
    // Use PushID for stable widget IDs
    renderer->PushID(spec.id.c_str());
    
    // Use TextUnformatted for cached strings (avoids formatting overhead)
    renderer->TextUnformatted(spec.text.c_str());
    
    renderer->PopID();
}


void createStatusBadge(const StatusBadgeSpec& spec) {
    if (!spec.visible || spec.text.empty()) {
        return;
    }
    
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr) {
        return;
    }
    
    // Use PushID for stable widget IDs
    renderer->PushID(spec.id.c_str());
    
    // Calculate spacing to push badge to the right
    ImVec2 avail = renderer->GetContentRegionAvail();
    ImVec2 textSize = renderer->CalcTextSize(spec.text.c_str());
    float spacing = avail.x - textSize.x - 20.0f; // 20px padding
    
    if (spacing > 0.0f) {
        renderer->SameLine(0.0f, spacing);
    } else {
        renderer->NewLine();
    }
    
    // Render status text directly (simpler than using Text widget)
    renderer->TextUnformatted(spec.text.c_str());
    
    renderer->PopID();
}

// ActionBanner widget implementation

void createActionBanner(const ActionBannerSpec& spec) {
    if (!spec.visible || spec.message.empty()) {
        return;
    }
    
    IUiRenderer* renderer = GetUiRenderer();
    if (renderer == nullptr) {
        return;
    }
    
    // Use PushID for stable widget IDs
    renderer->PushID(spec.id.c_str());
    
    // Add visual prefix to indicate success/error (without color, per MDC rules)
    std::string displayMessage = spec.message;
    if (spec.success) {
        displayMessage = "[OK] " + displayMessage;
    } else {
        displayMessage = "[ERROR] " + displayMessage;
    }
    
    // Render message text
    renderer->TextUnformatted(displayMessage.c_str());
    
    renderer->PopID();
}

} // namespace UI
} // namespace XIFriendList

