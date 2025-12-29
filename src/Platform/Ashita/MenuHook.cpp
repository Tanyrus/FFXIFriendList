#include "MenuHook.h"
#include "AshitaAdapter.h"
#include "PatternScanFingerprint.h"
#include <Ashita.h>
#include <windows.h>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <fstream>
#include "UI/Commands/WindowCommands.h"
#include "Debug/DebugLog.h"
#include <chrono>
#include <mutex>
#include <unordered_set>
#include "Debug/Perf.h"


namespace XIFriendList {
namespace Platform {
namespace Ashita {

MenuHook* g_MenuHookInstance = nullptr;
uintptr_t g_FriendListTrampoline = 0;

static std::mutex g_MenuHookLogMutex;
static std::unordered_set<std::string> g_MenuHookLoggedOnce;
static int g_MenuHookInitializeCallCount = 0;
static bool shouldLogOnce(const std::string& key) {
    std::lock_guard<std::mutex> lock(g_MenuHookLogMutex);
    return g_MenuHookLoggedOnce.insert(key).second;
}

static void pushDebugLogLine(const std::string& line) {
    ::XIFriendList::Debug::DebugLog::getInstance().push(line);
}

static void logInfoOnce(ILogManager* logger, const std::string& key, const std::string& message) {
    if (logger == nullptr) {
        return;
    }
    if (!shouldLogOnce(key)) {
        return;
    }
    logger->Log(static_cast<uint32_t>(::Ashita::LogLevel::Info), "MenuHook", message.c_str());
}

static void logInfo(ILogManager* logger, const std::string& message) {
    if (logger == nullptr) {
        return;
    }
    logger->Log(static_cast<uint32_t>(::Ashita::LogLevel::Info), "MenuHook", message.c_str());
}

static std::string formatHexPtr(uintptr_t value) {
    std::ostringstream oss;
    oss << "0x" << std::hex << value << std::dec;
    return oss.str();
}

static const uint8_t FRIEND_PATTERN[] = {0x53, 0x56, 0x57, 0x8B, 0xF1, 0xE8, 0xFF, 0xFF, 0xFF, 0xFF, 0x6A, 0x02, 0xE8, 0xFF, 0xFF, 0xFF, 0xFF, 0x83, 0xC4, 0x04};
static const uint8_t FRIEND_MASK[]    = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF};
static const uint8_t MSG_PATTERN[]    = {0x56, 0x8B, 0xF1, 0xE8, 0xFF, 0xFF, 0xFF, 0xFF, 0x6A, 0x01, 0xE8, 0xFF, 0xFF, 0xFF, 0xFF, 0x8B, 0x00, 0x6A, 0x01};
static const uint8_t MSG_MASK[]       = {0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF};

enum HookStage {
    STAGE_ENTRY = 1,
    STAGE_BEFORE_HELPER = 2,
    STAGE_AFTER_HELPER = 3,
    STAGE_BEFORE_TRAMPOLINE = 4
};

static constexpr uint32_t HOOK_TOGGLE_COOLDOWN_MS = 450;

extern "C" void LogHookState(int32_t stageId, void* thisPtr, int32_t param, uintptr_t esp, uintptr_t ecx) {
    if (g_MenuHookInstance && g_MenuHookInstance->logger_) {
        const char* stageName = "UNKNOWN";
        switch (stageId) {
            case STAGE_ENTRY: stageName = "ENTRY"; break;
            case STAGE_BEFORE_HELPER: stageName = "BEFORE_HELPER"; break;
            case STAGE_AFTER_HELPER: stageName = "AFTER_HELPER"; break;
            case STAGE_BEFORE_TRAMPOLINE: stageName = "BEFORE_TRAMPOLINE"; break;
        }
        std::ostringstream msg;
        msg << "[HOOK DEBUG] " << stageName << " - thisPtr=0x" << std::hex << (uintptr_t)thisPtr
            << " param=" << std::dec << param << " ESP=0x" << std::hex << esp
            << " ECX=0x" << ecx << std::dec;
        g_MenuHookInstance->logger_->Log(static_cast<uint32_t>(::Ashita::LogLevel::Info), "MenuHook", msg.str().c_str());
    }
}

extern "C" void __fastcall Hooked_FUNC_YkWndFriendMain_OpenFriend_Fastcall(void* thisPtr, int32_t param) {
    if (g_MenuHookInstance) {
        g_MenuHookInstance->onFriendListOpen(thisPtr, param);
    }
}

extern "C" void __declspec(naked) Hooked_FUNC_YkWndFriendMain_OpenFriend() {
    __asm {
        mov edx, dword ptr [esp+4]
        call Hooked_FUNC_YkWndFriendMain_OpenFriend_Fastcall
        ret 4
    }
}

extern "C" void __fastcall Hooked_FUNC_YkWndMessageList_Open_Fastcall(void* thisPtr, int32_t param1_low, int32_t param1_high, int32_t param2) {
    int64_t param1 = ((int64_t)param1_high << 32) | (uint32_t)param1_low;
    if (g_MenuHookInstance) {
        g_MenuHookInstance->onMessageListOpen(thisPtr, param1, param2);
    }
}

extern "C" void __declspec(naked) Hooked_FUNC_YkWndMessageList_Open() {
    __asm {
        mov edx, dword ptr [esp+4]
        push dword ptr [esp+12]
        push dword ptr [esp+12]
        call Hooked_FUNC_YkWndMessageList_Open_Fastcall
        ret 12
    }
}

MenuHook::MenuHook()
    : hookInstalled_(false)
    , friendListHookAddress_(0)
    , messageListHookAddress_(0)
    , friendListTrampoline_(0)
    , messageListTrampoline_(0)
    , logger_(nullptr)
    , adapter_(nullptr)
{
    memset(friendListOriginalBytes_, 0, sizeof(friendListOriginalBytes_));
    memset(messageListOriginalBytes_, 0, sizeof(messageListOriginalBytes_));
}

MenuHook::~MenuHook() {
    shutdown();
}

bool MenuHook::initialize(IAshitaCore* core, ILogManager* logger, ::XIFriendList::Platform::Ashita::AshitaAdapter* adapter) {
    PERF_SCOPE("MenuHook::initialize");
    if (hookInstalled_) {
        return true;
    }
    
    logger_ = logger;
    adapter_ = adapter;
    g_MenuHookInstance = this;

    g_MenuHookInitializeCallCount++;
    const bool verbose = (g_MenuHookInitializeCallCount <= 5);
    if (verbose) {
        std::ostringstream msg;
        msg << "Initialize call #" << g_MenuHookInitializeCallCount
            << " (verbose logging enabled for first 5 initializes)";
        logInfoOnce(logger, "MenuHook.InitializeCallCount." + std::to_string(g_MenuHookInitializeCallCount), msg.str());
    }
    
    if (!core || !logger || !adapter) {
        if (logger) {
            logger->Log(2, "MenuHook", "Invalid parameters for initialize");
        }
        return false;
    }

    bool friendOk = installFriendListHook(core, logger, true);
    bool messageOk = installMessageListHook(core, logger, true);
    hookInstalled_ = friendOk || messageOk;
    return hookInstalled_;
}

void MenuHook::shutdown() {
    if (friendListHookAddress_ != 0) {
        DWORD oldProtect = 0;
        if (::VirtualProtect((void*)friendListHookAddress_, 5, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            memcpy((void*)friendListHookAddress_, friendListOriginalBytes_, 5);
            DWORD dummy;
            ::VirtualProtect((void*)friendListHookAddress_, 5, oldProtect, &dummy);
        }
    }
    
    if (messageListHookAddress_ != 0) {
        DWORD oldProtect = 0;
        if (::VirtualProtect((void*)messageListHookAddress_, 5, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            memcpy((void*)messageListHookAddress_, messageListOriginalBytes_, 5);
            DWORD dummy;
            ::VirtualProtect((void*)messageListHookAddress_, 5, oldProtect, &dummy);
        }
    }
    
    if (friendListTrampoline_ != 0) {
        ::VirtualFree((void*)friendListTrampoline_, 0, MEM_RELEASE);
        friendListTrampoline_ = 0;
        g_FriendListTrampoline = 0;
    }
    
    if (messageListTrampoline_ != 0) {
        ::VirtualFree((void*)messageListTrampoline_, 0, MEM_RELEASE);
        messageListTrampoline_ = 0;
    }
    
    hookInstalled_ = false;
    friendListHookAddress_ = 0;
    messageListHookAddress_ = 0;
    g_MenuHookInstance = nullptr;
}

bool MenuHook::installFriendListHookAtAddress(ILogManager* logger, uintptr_t funcAddr) {
    if (logger == nullptr || funcAddr == 0) {
        return false;
    }

    memcpy(friendListOriginalBytes_, (void*)funcAddr, 5);

    SIZE_T trampolineSize = 16;
    void* trampolineMem = ::VirtualAlloc(nullptr, trampolineSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!trampolineMem) {
        logger->Log(2, "MenuHook", "Failed to allocate trampoline memory for FriendList hook");
        return false;
    }

    uintptr_t trampolineAddr = (uintptr_t)trampolineMem;
    friendListTrampoline_ = trampolineAddr;
    g_FriendListTrampoline = trampolineAddr;

    memcpy(trampolineMem, friendListOriginalBytes_, 5);

    uint8_t* trampolineBytes = (uint8_t*)trampolineMem;
    uintptr_t originalContinue = funcAddr + 5;
    int32_t trampolineJmpOffset = (int32_t)(originalContinue - (trampolineAddr + 5 + 5));
    trampolineBytes[5] = 0xE9;
    memcpy(&trampolineBytes[6], &trampolineJmpOffset, 4);

    DWORD oldTrampProtect = 0;
    ::VirtualProtect(trampolineMem, trampolineSize, PAGE_EXECUTE_READ, &oldTrampProtect);

    DWORD oldProtect = 0;
    if (!::VirtualProtect((void*)funcAddr, 5, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        logger->Log(2, "MenuHook", "Failed to unprotect memory for FriendList hook");
        return false;
    }

    uintptr_t hookAddr = (uintptr_t)Hooked_FUNC_YkWndFriendMain_OpenFriend;
    int32_t jmpOffset = (int32_t)(hookAddr - (funcAddr + 5));

    uint8_t jmp[5];
    jmp[0] = 0xE9;
    memcpy(&jmp[1], &jmpOffset, 4);
    memcpy((void*)funcAddr, jmp, 5);

    DWORD dummy;
    ::VirtualProtect((void*)funcAddr, 5, oldProtect, &dummy);

    friendListHookAddress_ = funcAddr;
    logger->Log(static_cast<uint32_t>(::Ashita::LogLevel::Info), "MenuHook", "FriendList hook installed successfully!");
    return true;
}

bool MenuHook::installMessageListHookAtAddress(ILogManager* logger, uintptr_t funcAddr) {
    if (logger == nullptr || funcAddr == 0) {
        return false;
    }

    memcpy(messageListOriginalBytes_, (void*)funcAddr, 5);

    DWORD oldProtect = 0;
    if (!::VirtualProtect((void*)funcAddr, 5, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        logger->Log(2, "MenuHook", "Failed to unprotect memory for MessageList hook");
        return false;
    }

    uintptr_t hookAddr = (uintptr_t)Hooked_FUNC_YkWndMessageList_Open;
    int32_t jmpOffset = (int32_t)(hookAddr - (funcAddr + 5));

    uint8_t jmp[5];
    jmp[0] = 0xE9;
    memcpy(&jmp[1], &jmpOffset, 4);
    memcpy((void*)funcAddr, jmp, 5);

    DWORD dummy;
    ::VirtualProtect((void*)funcAddr, 5, oldProtect, &dummy);

    messageListHookAddress_ = funcAddr;
    logger->Log(static_cast<uint32_t>(::Ashita::LogLevel::Info), "MenuHook", "MessageList hook installed successfully!");
    return true;
}

std::optional<uintptr_t> MenuHook::tryResolveFriendListByOffset(IAshitaCore* core) const {
    if (core == nullptr) {
        return std::nullopt;
    }
    IOffsetManager* offsetMgr = core->GetOffsetManager();
    if (offsetMgr == nullptr) {
        return std::nullopt;
    }

    struct Candidate { const char* section; const char* key; };
    const Candidate candidates[] = {
        { "FriendList", "FUNC_YkWndFriendMain_OpenFriend" },
        { "Menu", "FUNC_YkWndFriendMain_OpenFriend" },
        { "Functions", "FUNC_YkWndFriendMain_OpenFriend" },
        { "YkWndFriendMain", "FUNC_YkWndFriendMain_OpenFriend" },
        { "UI", "FUNC_YkWndFriendMain_OpenFriend" },
        { "Interface", "FUNC_YkWndFriendMain_OpenFriend" },
    };

    const uintptr_t moduleBase = ::Ashita::Memory::GetModuleBase("FFXiMain.dll");
    if (moduleBase == 0) {
        return std::nullopt;
    }

    for (const auto& c : candidates) {
        const int32_t offset = offsetMgr->Get(c.section, c.key);
        if (offset != 0) {
            return moduleBase + static_cast<uintptr_t>(offset);
        }
    }
    return std::nullopt;
}

std::optional<uintptr_t> MenuHook::tryResolveMessageListByOffset(IAshitaCore* core) const {
    if (core == nullptr) {
        return std::nullopt;
    }
    IOffsetManager* offsetMgr = core->GetOffsetManager();
    if (offsetMgr == nullptr) {
        return std::nullopt;
    }

    struct Candidate { const char* section; const char* key; };
    const Candidate candidates[] = {
        { "MessageList", "FUNC_YkWndMessageList_Open" },
        { "Menu", "FUNC_YkWndMessageList_Open" },
        { "Functions", "FUNC_YkWndMessageList_Open" },
        { "YkWndMessageList", "FUNC_YkWndMessageList_Open" },
        { "UI", "FUNC_YkWndMessageList_Open" },
        { "Interface", "FUNC_YkWndMessageList_Open" },
    };

    const uintptr_t moduleBase = ::Ashita::Memory::GetModuleBase("FFXiMain.dll");
    if (moduleBase == 0) {
        return std::nullopt;
    }

    for (const auto& c : candidates) {
        const int32_t offset = offsetMgr->Get(c.section, c.key);
        if (offset != 0) {
            return moduleBase + static_cast<uintptr_t>(offset);
        }
    }
    return std::nullopt;
}

void MenuHook::onFriendListOpen(void* thisPtr, int32_t param) {
    (void)thisPtr;
    (void)param;
    if (adapter_) {
        static uint32_t s_lastOpenedTick = 0;
        const uint32_t now = GetTickCount();
        const bool isVisible = adapter_->isQuickOnlineWindowVisible();
        if (!isVisible) {
            adapter_->openQuickOnlineWindow();
            s_lastOpenedTick = now;
            ::XIFriendList::Debug::MarkFirstInteractive();
            adapter_->triggerRefreshOnOpen();
            return;
        }
        if (now - s_lastOpenedTick < HOOK_TOGGLE_COOLDOWN_MS) {
            return;
        }
        adapter_->closeQuickOnlineWindow();
        s_lastOpenedTick = now;
    }
}

void MenuHook::onMessageListOpen(void* thisPtr, int64_t param1, int32_t param2) {
    (void)thisPtr;
    (void)param1;
    (void)param2;
}

bool MenuHook::installFriendListHook(IAshitaCore* core, ILogManager* logger, bool allowSlowResolution) {
    
    if (!core || !logger) {
        return false;
    }
    
    logger->Log(static_cast<uint32_t>(::Ashita::LogLevel::Info), "MenuHook", "Installing FriendList (To List) hook...");

    if (!allowSlowResolution) {
        return false;
    }
    
    // Try IOffsetManager first
    IOffsetManager* offsetMgr = core->GetOffsetManager();
    uintptr_t funcAddr = 0;
    if (offsetMgr == nullptr) {
        logInfoOnce(logger, "MenuHook.FriendList.OffsetMgrMissing", "FriendList hook: IOffsetManager is null (will require fallback address resolution).");
    }
    
    // Keep offset support, but avoid large OffsetManager spam:
    // try a small set of likely section/key pairs.
    if (offsetMgr) {
        struct Candidate { const char* section; const char* key; };
        const Candidate candidates[] = {
            { "FriendList", "FUNC_YkWndFriendMain_OpenFriend" },
            { "Menu", "FUNC_YkWndFriendMain_OpenFriend" },
            { "Functions", "FUNC_YkWndFriendMain_OpenFriend" },
            { "YkWndFriendMain", "FUNC_YkWndFriendMain_OpenFriend" },
            { "UI", "FUNC_YkWndFriendMain_OpenFriend" },
            { "Interface", "FUNC_YkWndFriendMain_OpenFriend" },
        };

        for (const auto& c : candidates) {
            const int32_t offset = offsetMgr->Get(c.section, c.key);
            if (offset != 0) {
                const uintptr_t moduleBase = ::Ashita::Memory::GetModuleBase("FFXiMain.dll");
                if (moduleBase != 0) {
                    funcAddr = moduleBase + static_cast<uintptr_t>(offset);
                    std::ostringstream offsetMsg;
                    offsetMsg << "Found FriendList offset via [" << c.section << "/" << c.key << "]: 0x" << std::hex << funcAddr << std::dec;
                    logger->Log(static_cast<uint32_t>(::Ashita::LogLevel::Info), "MenuHook", offsetMsg.str().c_str());
                    break;
                }
            }
        }
    }
    
    if (funcAddr == 0) {
        const uint8_t pattern[] = {0x53, 0x56, 0x57, 0x8B, 0xF1, 0xE8, 0xFF, 0xFF, 0xFF, 0xFF, 0x6A, 0x02, 0xE8, 0xFF, 0xFF, 0xFF, 0xFF, 0x83, 0xC4, 0x04};
        const uint8_t mask[] =    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF};
        logInfoOnce(logger, "MenuHook.FriendList.OffsetMissing", "FriendList hook: offsets not found; using pattern scan fallback.");
        funcAddr = findFunctionByPattern("FFXiMain.dll", pattern, mask, sizeof(pattern), "FriendListHook");
        if (funcAddr != 0) {
            std::ostringstream msg;
            msg << "Found FriendList function via pattern scanning: 0x" << std::hex << funcAddr << std::dec;
            logger->Log(static_cast<uint32_t>(::Ashita::LogLevel::Info), "MenuHook", msg.str().c_str());
        }
    }
    
    if (funcAddr == 0) {
        logger->Log(2, "MenuHook", "Could not find FriendList function address");
        return false;
    }
    
    memcpy(friendListOriginalBytes_, (void*)funcAddr, 5);
    
    SIZE_T trampolineSize = 16;
    void* trampolineMem = ::VirtualAlloc(nullptr, trampolineSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!trampolineMem) {
        logger->Log(2, "MenuHook", "[DEBUG] Failed to allocate trampoline memory");
        return false;
    }
    
    uintptr_t trampolineAddr = (uintptr_t)trampolineMem;
    friendListTrampoline_ = trampolineAddr;
    g_FriendListTrampoline = trampolineAddr;
    
    memcpy(trampolineMem, friendListOriginalBytes_, 5);
    
    uint8_t* trampolineBytes = (uint8_t*)trampolineMem;
    uintptr_t originalContinue = funcAddr + 5;
    int32_t trampolineJmpOffset = (int32_t)(originalContinue - (trampolineAddr + 5 + 5));
    trampolineBytes[5] = 0xE9;
    memcpy(&trampolineBytes[6], &trampolineJmpOffset, 4);
    
    DWORD oldTrampProtect = 0;
    ::VirtualProtect(trampolineMem, trampolineSize, PAGE_EXECUTE_READ, &oldTrampProtect);
    
    DWORD oldProtect = 0;
    if (!::VirtualProtect((void*)funcAddr, 5, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        logger->Log(2, "MenuHook", "Failed to unprotect memory for FriendList hook");
        return false;
    }
    
    uintptr_t hookAddr = (uintptr_t)Hooked_FUNC_YkWndFriendMain_OpenFriend;
    int32_t jmpOffset = (int32_t)(hookAddr - (funcAddr + 5));
    
    uint8_t jmp[5];
    jmp[0] = 0xE9;
    memcpy(&jmp[1], &jmpOffset, 4);
    memcpy((void*)funcAddr, jmp, 5);
    
    DWORD dummy;
    ::VirtualProtect((void*)funcAddr, 5, oldProtect, &dummy);
    
    friendListHookAddress_ = funcAddr;
    logger->Log(static_cast<uint32_t>(::Ashita::LogLevel::Info), "MenuHook", "FriendList hook installed successfully!");
    
    return true;
}

bool MenuHook::installMessageListHook(IAshitaCore* core, ILogManager* logger, bool allowSlowResolution) {
    
    if (!core || !logger) {
        return false;
    }
    
    logger->Log(static_cast<uint32_t>(::Ashita::LogLevel::Info), "MenuHook", "Installing MessageList (Messages) hook...");

    if (!allowSlowResolution) {
        return false;
    }
    
    IOffsetManager* offsetMgr = core->GetOffsetManager();
    uintptr_t funcAddr = 0;
    if (offsetMgr == nullptr) {
        logInfoOnce(logger, "MenuHook.MessageList.OffsetMgrMissing", "MessageList hook: IOffsetManager is null (will require fallback address resolution).");
    }
    
    if (offsetMgr) {
        struct Candidate { const char* section; const char* key; };
        const Candidate candidates[] = {
            { "MessageList", "FUNC_YkWndMessageList_Open" },
            { "Menu", "FUNC_YkWndMessageList_Open" },
            { "Functions", "FUNC_YkWndMessageList_Open" },
            { "YkWndMessageList", "FUNC_YkWndMessageList_Open" },
            { "UI", "FUNC_YkWndMessageList_Open" },
            { "Interface", "FUNC_YkWndMessageList_Open" },
        };

        for (const auto& c : candidates) {
            const int32_t offset = offsetMgr->Get(c.section, c.key);
            if (offset != 0) {
                const uintptr_t moduleBase = ::Ashita::Memory::GetModuleBase("FFXiMain.dll");
                if (moduleBase != 0) {
                    funcAddr = moduleBase + static_cast<uintptr_t>(offset);
                    std::ostringstream offsetMsg;
                    offsetMsg << "Found MessageList offset via [" << c.section << "/" << c.key << "]: 0x" << std::hex << funcAddr << std::dec;
                    logger->Log(static_cast<uint32_t>(::Ashita::LogLevel::Info), "MenuHook", offsetMsg.str().c_str());
                    break;
                }
            }
        }
    }
    
    if (funcAddr == 0) {
        const uint8_t pattern[] = {0x56, 0x8B, 0xF1, 0xE8, 0xFF, 0xFF, 0xFF, 0xFF, 0x6A, 0x01, 0xE8, 0xFF, 0xFF, 0xFF, 0xFF, 0x8B, 0x00, 0x6A, 0x01};
        const uint8_t mask[] =    {0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF};
        logInfoOnce(logger, "MenuHook.MessageList.OffsetMissing", "MessageList hook: offsets not found; using pattern scan fallback.");
        funcAddr = findFunctionByPattern("FFXiMain.dll", pattern, mask, sizeof(pattern), "MessageListHook");
        if (funcAddr != 0) {
            std::ostringstream msg;
            msg << "Found MessageList function via pattern scanning: 0x" << std::hex << funcAddr << std::dec;
            logger->Log(static_cast<uint32_t>(::Ashita::LogLevel::Info), "MenuHook", msg.str().c_str());
        }
    }
    
    if (funcAddr == 0) {
        logger->Log(2, "MenuHook", "Could not find MessageList function address");
        return false;
    }
    
    memcpy(messageListOriginalBytes_, (void*)funcAddr, 5);
    
    DWORD oldProtect = 0;
    if (!::VirtualProtect((void*)funcAddr, 5, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        logger->Log(2, "MenuHook", "Failed to unprotect memory for MessageList hook");
        return false;
    }
    
    uintptr_t hookAddr = (uintptr_t)Hooked_FUNC_YkWndMessageList_Open;
    int32_t jmpOffset = (int32_t)(hookAddr - (funcAddr + 5));
    
    uint8_t jmp[5];
    jmp[0] = 0xE9;
    memcpy(&jmp[1], &jmpOffset, 4);
    memcpy((void*)funcAddr, jmp, 5);
    
    DWORD dummy;
    ::VirtualProtect((void*)funcAddr, 5, oldProtect, &dummy);
    
    messageListHookAddress_ = funcAddr;
    logger->Log(static_cast<uint32_t>(::Ashita::LogLevel::Info), "MenuHook", "MessageList hook installed successfully!");
    
    return true;
}

uintptr_t MenuHook::findFunctionByPattern(const char* moduleName, const uint8_t* pattern, const uint8_t* mask, size_t patternSize, const char* patternId) {
    uintptr_t moduleBase = ::Ashita::Memory::GetModuleBase(moduleName);
    if (moduleBase == 0) {
        logInfoOnce(logger_, std::string("MenuHook.Scan.ModuleBaseMissing.") + (patternId ? patternId : "Unknown"),
                    std::string("Pattern scan aborted: module base is 0 for ") + (moduleName ? moduleName : "(null)") +
                        " patternId=" + (patternId ? patternId : "Unknown"));
        return 0;
    }
    
    uintptr_t moduleSize = ::Ashita::Memory::GetModuleSize(moduleName);
    if (moduleSize == 0) {
        logInfoOnce(logger_, std::string("MenuHook.Scan.ModuleSizeMissing.") + (patternId ? patternId : "Unknown"),
                    std::string("Pattern scan aborted: module size is 0 for ") + (moduleName ? moduleName : "(null)") +
                        " base=" + formatHexPtr(moduleBase) +
                        " patternId=" + (patternId ? patternId : "Unknown"));
        return 0;
    }

    const ModuleFingerprint fp = computeModuleFingerprint(moduleBase, moduleSize);
    const auto start = std::chrono::steady_clock::now();
    
    for (uintptr_t addr = moduleBase; addr < moduleBase + moduleSize - patternSize; addr++) {
        bool match = true;
        for (size_t i = 0; i < patternSize; i++) {
            if (mask[i] == 0x00) {
                continue;
            }
            uint8_t byte = *(uint8_t*)(addr + i);
            if (byte != pattern[i]) {
                match = false;
                break;
            }
        }
        if (match) {
            const auto end = std::chrono::steady_clock::now();
            const double elapsedMs = std::chrono::duration<double, std::milli>(end - start).count();

            const uintptr_t foundAddress = addr;
            const uintptr_t foundRva = foundAddress - moduleBase;

            std::ostringstream summary;
            summary << "[MenuHook] Pattern=" << (patternId ? patternId : "Unknown")
                    << " source=scan"
                    << " module=" << (moduleName ? moduleName : "(null)")
                    << " " << ::XIFriendList::Platform::Ashita::formatModuleFingerprint(fp)
                    << " foundAddress=" << formatHexPtr(foundAddress)
                    << " foundRva=" << formatHexPtr(foundRva)
                    << " elapsedMs=" << std::fixed << std::setprecision(2) << elapsedMs;

            // Easy-to-copy single line: send to both Ashita log and in-plugin DebugLog.
            logInfo(logger_, summary.str());
            pushDebugLogLine(summary.str());

            return addr;
        }
    }

    const auto end = std::chrono::steady_clock::now();
    const double elapsedMs = std::chrono::duration<double, std::milli>(end - start).count();

    std::ostringstream summary;
    summary << "[MenuHook] Pattern=" << (patternId ? patternId : "Unknown")
            << " source=scan"
            << " module=" << (moduleName ? moduleName : "(null)")
            << " " << ::XIFriendList::Platform::Ashita::formatModuleFingerprint(fp)
            << " foundAddress=0x0"
            << " foundRva=0x0"
            << " elapsedMs=" << std::fixed << std::setprecision(2) << elapsedMs
            << " result=NOT_FOUND";
    logInfo(logger_, summary.str());
    pushDebugLogLine(summary.str());
    
    return 0;
}

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

