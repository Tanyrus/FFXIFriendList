// Toast notification manager implementation (UI layer - direct ImGui)

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "UI/Notifications/ToastManager.h"
#include "Platform/Ashita/ImGuiBridge.h"
#include "Core/MemoryStats.h"
#include "App/NotificationConstants.h"
#ifndef BUILDING_TESTS
#include <Ashita.h>
#include <imgui.h>
#endif
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iomanip>

// Static constant definitions
const float XIFriendList::UI::Notifications::ToastManager::ANIMATION_DURATION_MS = 300.0f;
const float XIFriendList::UI::Notifications::ToastManager::SLIDE_DISTANCE = -400.0f;  // Negative for slide from/to left
const float XIFriendList::UI::Notifications::ToastManager::TOAST_SPACING = 10.0f;
const float XIFriendList::UI::Notifications::ToastManager::TOAST_PADDING = 15.0f;
const float XIFriendList::UI::Notifications::ToastManager::MAX_TOAST_WIDTH = 350.0f;

namespace XIFriendList {
namespace UI {
namespace Notifications {

ToastManager::ToastManager() 
    : positionX_(XIFriendList::App::Notifications::Constants::DEFAULT_NOTIFICATION_POSITION_X)
    , positionY_(XIFriendList::App::Notifications::Constants::DEFAULT_NOTIFICATION_POSITION_Y)
{}

void ToastManager::addToast(const XIFriendList::App::Notifications::Toast& toast) {
    toasts_.push_back(toast);
}

void ToastManager::update(int64_t currentTime) {
    for (size_t i = 0; i < toasts_.size(); ++i) {
        updateToast(toasts_[i], currentTime, i);
    }
    
    // Remove completed toasts
    toasts_.erase(
        std::remove_if(toasts_.begin(), toasts_.end(),
            [](const XIFriendList::App::Notifications::Toast& toast) {
                return toast.state == XIFriendList::App::Notifications::ToastState::COMPLETE;
            }),
        toasts_.end()
    );
}

void ToastManager::updateToast(XIFriendList::App::Notifications::Toast& toast, int64_t currentTime, size_t index) {
    int64_t elapsed = currentTime - toast.createdAt;
    
    switch (toast.state) {
        case XIFriendList::App::Notifications::ToastState::ENTERING: {
            // Animate slide-in and fade-in
            float progress = std::min(1.0f, static_cast<float>(elapsed) / static_cast<float>(ANIMATION_DURATION_MS));
            toast.offsetX = SLIDE_DISTANCE * (1.0f - progress);
            toast.alpha = progress;
            
            // Transition to VISIBLE when animation completes
            if (progress >= 1.0f) {
                toast.state = XIFriendList::App::Notifications::ToastState::VISIBLE;
                toast.offsetX = 0.0f;
                toast.alpha = 1.0f;
            }
            break;
        }
        
        case XIFriendList::App::Notifications::ToastState::VISIBLE: {
            if (toast.dismissed || (toast.duration > 0 && elapsed >= toast.duration)) {
                toast.state = XIFriendList::App::Notifications::ToastState::EXITING;
                toast.createdAt = currentTime;  // Reset timer for exit animation
            }
            break;
        }
        
               case XIFriendList::App::Notifications::ToastState::EXITING: {
                   // Animate slide-out and fade-out (slide to left, so offset becomes more negative)
                   int64_t exitElapsed = currentTime - toast.createdAt;
                   float progress = std::min(1.0f, static_cast<float>(exitElapsed) / static_cast<float>(ANIMATION_DURATION_MS));
                   toast.offsetX = SLIDE_DISTANCE * progress;  // SLIDE_DISTANCE is negative, so this slides to left
                   toast.alpha = 1.0f - progress;
            
            // Transition to COMPLETE when animation completes
            if (progress >= 1.0f) {
                toast.state = XIFriendList::App::Notifications::ToastState::COMPLETE;
            }
            break;
        }
        
        case XIFriendList::App::Notifications::ToastState::COMPLETE:
            // Do nothing, will be removed
            break;
    }
}

void ToastManager::render() {
    if (toasts_.empty()) {
        return;
    }
    
    auto* guiManager = XIFriendList::Platform::Ashita::ImGuiBridge::getGuiManager();
    if (!guiManager) {
        return;
    }
    
    // Render toasts from bottom to top (newest at top)
    for (size_t i = 0; i < toasts_.size(); ++i) {
        size_t reverseIndex = toasts_.size() - 1 - i;  // Reverse order
        renderToast(toasts_[reverseIndex], reverseIndex);
    }
}

void ToastManager::renderToast(XIFriendList::App::Notifications::Toast& toast, size_t index) {
    if (toast.state == XIFriendList::App::Notifications::ToastState::COMPLETE) {
        return;
    }
    
    auto* guiManager = XIFriendList::Platform::Ashita::ImGuiBridge::getGuiManager();
    if (!guiManager) {
        return;
    }
    
    // Calculate base position - use positionX_ and positionY_ directly
    // Default is 0,0 (top-left corner), user can set positive X,Y to move it
    ImVec2 basePos(positionX_, positionY_);
    
    // Stack toasts vertically (newest at top)
    float yOffset = index * (TOAST_SPACING + 60.0f);  // Approximate height per toast (60px per toast)
    
    // Window flags (no focus stealing, time-based dismissal only, click-through behavior)
    // Note: NoMove removed to allow programmatic position updates
    ImGuiWindowFlags flags = 
        ImGuiWindowFlags_NoInputs |  // Click-through behavior (toasts must not block clicks)
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav;
    
    ::ImVec2 pos(
        basePos.x + toast.offsetX,  // Slide animation offset
        basePos.y + yOffset
    );
    
    // Include position hash in window ID to force ImGui to treat it as a new window when position changes
    // This ensures position updates are respected (ImGui caches window positions by ID)
    int posHash = static_cast<int>(positionX_ * 100.0f) ^ static_cast<int>(positionY_ * 100.0f);
    std::string windowId = "Toast_" + std::to_string(index) + "_p" + std::to_string(posHash);
    
    // Set window position and alpha before Begin()
    // Use 0 (ImGuiCond_Always) to force position update every frame
    // This is critical for dynamic position changes when user adjusts X/Y settings
    guiManager->SetNextWindowPos(pos, 0);  // 0 = ImGuiCond_Always
    guiManager->SetNextWindowBgAlpha(toast.alpha);
    
    // Begin toast window (use IGuiManager)
    bool beginResult = guiManager->Begin(windowId.c_str(), nullptr, flags);
    
    // Must call End() exactly once for every Begin() call, regardless of Begin() return value
    if (!beginResult) {
        guiManager->End();
        return;
    }
    
    float r, g, b, a;
    getToastColor(toast.type, &r, &g, &b, &a);
    
    // Render title
    guiManager->PushStyleColor(ImGuiCol_Text, ImVec4(r, g, b, toast.alpha));
    guiManager->TextUnformatted(toast.title.c_str());
    guiManager->PopStyleColor();
    
    // Render message
    guiManager->PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, toast.alpha * 0.9f));
    guiManager->TextUnformatted(toast.message.c_str());
    guiManager->PopStyleColor();
    
    guiManager->End();
}

void ToastManager::clear() {
    toasts_.clear();
}

void ToastManager::setPosition(float x, float y) {
    // Convert -1 (old default) to default position
    float newX = (x < 0.0f) ? XIFriendList::App::Notifications::Constants::DEFAULT_NOTIFICATION_POSITION_X : x;
    float newY = (y < 0.0f) ? XIFriendList::App::Notifications::Constants::DEFAULT_NOTIFICATION_POSITION_Y : y;
    
    positionX_ = newX;
    positionY_ = newY;
    
    // Note: We don't clear toasts here anymore. Instead, we include position in window ID
    // so ImGui treats it as a new window when position changes, forcing position update.
}

void ToastManager::getToastColor(XIFriendList::App::Notifications::ToastType type, float* r, float* g, float* b, float* a) const {
    switch (type) {
        case XIFriendList::App::Notifications::ToastType::FriendOnline:
            *r = 0.2f; *g = 1.0f; *b = 0.2f; *a = 1.0f;  // Green
            break;
        case XIFriendList::App::Notifications::ToastType::FriendOffline:
            *r = 0.8f; *g = 0.8f; *b = 0.8f; *a = 1.0f;  // Gray
            break;
        case XIFriendList::App::Notifications::ToastType::FriendRequestReceived:
        case XIFriendList::App::Notifications::ToastType::FriendRequestAccepted:
            *r = 0.2f; *g = 0.6f; *b = 1.0f; *a = 1.0f;  // Blue
            break;
        case XIFriendList::App::Notifications::ToastType::FriendRequestRejected:
            *r = 1.0f; *g = 0.6f; *b = 0.2f; *a = 1.0f;  // Orange
            break;
        case XIFriendList::App::Notifications::ToastType::MailReceived:
            *r = 1.0f; *g = 0.8f; *b = 0.2f; *a = 1.0f;  // Yellow
            break;
        case XIFriendList::App::Notifications::ToastType::Error:
            *r = 1.0f; *g = 0.2f; *b = 0.2f; *a = 1.0f;  // Red
            break;
        case XIFriendList::App::Notifications::ToastType::Warning:
            *r = 1.0f; *g = 0.8f; *b = 0.0f; *a = 1.0f;  // Yellow
            break;
        case XIFriendList::App::Notifications::ToastType::Success:
            *r = 0.2f; *g = 0.8f; *b = 0.2f; *a = 1.0f;  // Green
            break;
        case XIFriendList::App::Notifications::ToastType::Info:
        default:
            *r = 0.8f; *g = 0.8f; *b = 1.0f; *a = 1.0f;  // Light blue
            break;
    }
}

XIFriendList::Core::MemoryStats ToastManager::getMemoryStats() const {
    size_t toastBytes = 0;
    for (const auto& toast : toasts_) {
        toastBytes += sizeof(XIFriendList::App::Notifications::Toast);
        toastBytes += toast.title.capacity();
        toastBytes += toast.message.capacity();
    }
    toastBytes += toasts_.capacity() * sizeof(XIFriendList::App::Notifications::Toast);
    
    return XIFriendList::Core::MemoryStats(toasts_.size(), toastBytes, "Notifications");
}

} // namespace Notifications
} // namespace UI
} // namespace XIFriendList

