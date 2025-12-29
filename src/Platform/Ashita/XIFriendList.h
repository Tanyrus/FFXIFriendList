#ifndef PLATFORM_ASHITA_XIFRIENDLIST_H
#define PLATFORM_ASHITA_XIFRIENDLIST_H

#include <memory>
#include <cstdint>

// Must include the real Ashita interface definitions:
#include <Ashita.h>   // from your ASHITA_SDK_DIR

namespace XIFriendList {
namespace Platform {
namespace Ashita {

class AshitaAdapter;
class CommandHandlerHook;
class MenuHook;

// XIFriendList inherits from ::IPlugin (global namespace) to match Ashita ADK interface
class XIFriendList : public ::IPlugin
{
public:
    XIFriendList();
    ~XIFriendList();

    // IPlugin interface - signatures must match Ashita ADK exactly
    bool Initialize(IAshitaCore* core, ILogManager* logger, const uint32_t id) override;
    void Release() override;
    
    // Direct3D callbacks (required when UseDirect3D flag is set)
    bool Direct3DInitialize(IDirect3DDevice8* device) override;
    void Direct3DBeginScene(bool isRenderingBackBuffer) override;
    void Direct3DEndScene(bool isRenderingBackBuffer) override;
    void Direct3DPresent(const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion) override;
    
    // Command handling (required when UseCommands flag is set)
    bool HandleCommand(int32_t mode, const char* command, bool injected) override;
    
    // Packet handling (for zone change detection)
    bool HandleIncomingPacket(uint16_t id, uint32_t size, const uint8_t* data, uint8_t* modified, uint32_t sizeChunk, const uint8_t* dataChunk, bool injected, bool blocked) override;

    // Plugin info methods - these are part of IPlugin interface
    const char* GetName() const override;
    const char* GetAuthor() const override;
    const char* GetDescription() const override;
    const char* GetLink() const override;
    double GetVersion() const override;
    uint32_t GetFlags() const override;

private:
    std::unique_ptr<AshitaAdapter> adapter_;
    bool initialized_{ false };
    IDirect3DDevice8* d3dDevice_{ nullptr };
    
    // Safe plugin-level command handler hook system
    // This wraps our command handler to allow hooks to be registered
    // that execute before/after command processing, without modifying
    // game memory or calling internal game functions.
    std::unique_ptr<CommandHandlerHook> commandHandlerHook_;
    
    // Memory hooks for game menu functions (To List, Messages)
    std::unique_ptr<MenuHook> menuHook_;
    
    // Original command handler implementation (wrapped by hook system)
    bool handleCommandImpl(int32_t mode, const char* command, bool injected);
    
    // FIX: Cooldown period after initialization to skip packet processing backlog
    // This causes stutter even if each packet handler is fast
    uint32_t initializationTime_{ 0 };  // Timestamp when initialization completed
    static constexpr uint32_t PACKET_PROCESSING_COOLDOWN_MS = 500;  // Skip packets for 500ms after init
};

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

#endif

