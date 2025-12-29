// Toast notification manager (UI layer - direct ImGui)

#ifndef UI_NOTIFICATIONS_TOAST_MANAGER_H
#define UI_NOTIFICATIONS_TOAST_MANAGER_H

#include "App/Notifications/Toast.h"
#include "Core/MemoryStats.h"
#include <vector>
#include <cstdint>

namespace XIFriendList {
namespace UI {
namespace Notifications {

// Toast notification manager (singleton)
class ToastManager {
public:
    static ToastManager& getInstance() {
        static ToastManager instance;
        return instance;
    }
    
    // Add a toast to the queue
    void addToast(const XIFriendList::App::Notifications::Toast& toast);
    
    void update(int64_t currentTime);
    
    // Render all toasts (call each frame after update)
    void render();
    
    // Clear all toasts
    void clear();
    
    size_t getToastCount() const { return toasts_.size(); }
    
    void setPosition(float x, float y);
    
    float getPositionX() const { return positionX_; }
    float getPositionY() const { return positionY_; }
    
    XIFriendList::Core::MemoryStats getMemoryStats() const;

private:
    ToastManager();
    ~ToastManager() = default;
    ToastManager(const ToastManager&) = delete;
    ToastManager& operator=(const ToastManager&) = delete;
    
    // Animation constants (defined in .cpp)
    static const float ANIMATION_DURATION_MS;  // 300ms for slide/fade
    static const float SLIDE_DISTANCE;         // Pixels to slide
    static const float TOAST_SPACING;           // Spacing between toasts
    static const float TOAST_PADDING;           // Padding inside toast
    static const float MAX_TOAST_WIDTH;        // Max width of toast
    
    void updateToast(XIFriendList::App::Notifications::Toast& toast, int64_t currentTime, size_t index);
    
    // Render a single toast
    void renderToast(XIFriendList::App::Notifications::Toast& toast, size_t index);
    
    void getToastColor(XIFriendList::App::Notifications::ToastType type, float* r, float* g, float* b, float* a) const;
    
    // Calculate default position (TopRight) based on viewport size
    
    std::vector<XIFriendList::App::Notifications::Toast> toasts_;
    float positionX_;  // -1 means "use default", otherwise absolute X position
    float positionY_;  // -1 means "use default", otherwise absolute Y position
};

} // namespace Notifications
} // namespace UI
} // namespace XIFriendList

#endif // UI_NOTIFICATIONS_TOAST_MANAGER_H

