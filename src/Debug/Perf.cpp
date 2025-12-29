
#include "Perf.h"
#include "DebugLog.h"
#include "Platform/Ashita/PathUtils.h"

#include <windows.h>
#include <algorithm>
#include <atomic>
#include <fstream>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace XIFriendList {
namespace Debug {

#if !XIFRIENDLIST_PERF_COMPILED

bool IsEnabled() { return false; }
void Initialize() {}
void Reset() {}
void PrintSummary(const char*, uint32_t) {}
void PrintSummaryOnce(const char*, uint32_t) {}
void MarkFirstInteractive() {}

Scope::Scope(const char*) {}
Scope::~Scope() {}

#else

struct Stat {
    uint64_t count{ 0 };
    double totalMs{ 0.0 };
    double maxMs{ 0.0 };
};

static std::once_flag g_InitOnce;
static std::atomic<bool> g_Enabled{ false };
static std::mutex g_Mutex;
static std::unordered_map<std::string, Stat> g_Stats;
static std::unordered_set<std::string> g_SummariesPrinted;
static std::atomic<bool> g_FirstInteractivePrinted{ false };

static uint64_t qpcNow() {
    LARGE_INTEGER v{};
    QueryPerformanceCounter(&v);
    return static_cast<uint64_t>(v.QuadPart);
}

static uint64_t qpcFreq() {
    static uint64_t freq = 0;
    if (freq == 0) {
        LARGE_INTEGER f{};
        QueryPerformanceFrequency(&f);
        freq = static_cast<uint64_t>(f.QuadPart);
    }
    return freq;
}

static double qpcToMs(uint64_t start, uint64_t end) {
    const uint64_t f = qpcFreq();
    if (f == 0) {
        return 0.0;
    }
    return (static_cast<double>(end - start) * 1000.0) / static_cast<double>(f);
}

static std::string getPerfConfigPath() {
    // {GameDir}\config\FFXIFriendList\debug.json
    char exePath[MAX_PATH] = { 0 };
    HMODULE hExe = GetModuleHandleA(nullptr);
    if (hExe != nullptr && GetModuleFileNameA(hExe, exePath, MAX_PATH) > 0) {
        std::string path(exePath);
        size_t lastSlash = path.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            std::string gameDir = path.substr(0, lastSlash);
            lastSlash = gameDir.find_last_of("\\/");
            if (lastSlash != std::string::npos) {
                gameDir = gameDir.substr(0, lastSlash + 1);
                return gameDir + "config\\FFXIFriendList\\debug.json";
            }
        }
    }
    std::string defaultPath = XIFriendList::Platform::Ashita::PathUtils::getDefaultConfigPath("debug.json");
    return defaultPath.empty() ? "C:\\HorizonXI\\Game\\config\\FFXIFriendList\\debug.json" : defaultPath;
}

static bool readBoolFlagFromDebugJson(bool& outEnabled) {
    const std::string path = getPerfConfigPath();
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    std::ostringstream oss;
    oss << file.rdbuf();
    const std::string content = oss.str();
    if (content.empty()) {
        return false;
    }

    const auto findInsensitive = [](const std::string& haystack, const std::string& needle) -> size_t {
        auto toLower = [](unsigned char c) { return static_cast<char>(::tolower(c)); };
        std::string h = haystack;
        std::string n = needle;
        std::transform(h.begin(), h.end(), h.begin(), toLower);
        std::transform(n.begin(), n.end(), n.begin(), toLower);
        return h.find(n);
    };

    const size_t enabledPos = findInsensitive(content, "\"enabled\"");
    if (enabledPos == std::string::npos) {
        return false;
    }
    const size_t colon = content.find(':', enabledPos);
    if (colon == std::string::npos) {
        return false;
    }
    const size_t t = findInsensitive(content.substr(colon), "true");
    const size_t f = findInsensitive(content.substr(colon), "false");
    if (t != std::string::npos && (f == std::string::npos || t < f)) {
        outEnabled = true;
        return true;
    }
    if (f != std::string::npos) {
        outEnabled = false;
        return true;
    }
    return false;
}

void Initialize() {
    std::call_once(g_InitOnce, []() {
        bool enabled = false;

#if defined(_DEBUG)
        enabled = true;
#endif

        bool configValue = false;
        if (readBoolFlagFromDebugJson(configValue)) {
            enabled = configValue;
        }

        g_Enabled.store(enabled);
        if (enabled) {
            XIFriendList::Debug::DebugLog::getInstance().push("[Perf] enabled=true");
        }
    });
}

bool IsEnabled() {
    Initialize();
    return g_Enabled.load();
}

void Reset() {
    if (!IsEnabled()) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_Mutex);
    g_Stats.clear();
    g_SummariesPrinted.clear();
    g_FirstInteractivePrinted.store(false);
}

static void addSampleLocked(const char* label, double ms) {
    if (label == nullptr) {
        return;
    }
    Stat& s = g_Stats[label];
    s.count += 1;
    s.totalMs += ms;
    if (ms > s.maxMs) {
        s.maxMs = ms;
    }
}

Scope::Scope(const char* label)
    : label_(label) {
    enabled_ = IsEnabled() && (label_ != nullptr);
    if (!enabled_) {
        return;
    }
    startTicks_ = qpcNow();
}

Scope::~Scope() {
    if (!enabled_) {
        return;
    }
    const uint64_t endTicks = qpcNow();
    const double ms = qpcToMs(startTicks_, endTicks);
    std::lock_guard<std::mutex> lock(g_Mutex);
    addSampleLocked(label_, ms);
}

void PrintSummary(const char* tag, uint32_t topN) {
    if (!IsEnabled()) {
        return;
    }

    std::vector<std::pair<std::string, Stat>> items;
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        items.reserve(g_Stats.size());
        for (const auto& kv : g_Stats) {
            items.emplace_back(kv.first, kv.second);
        }
    }

    std::sort(items.begin(), items.end(), [](const auto& a, const auto& b) {
        return a.second.totalMs > b.second.totalMs;
    });

    double total = 0.0;
    for (const auto& kv : items) {
        total += kv.second.totalMs;
    }

    std::ostringstream header;
    header << "[PerfSummary:" << (tag ? tag : "Unknown") << "] total=" << std::fixed << std::setprecision(2) << total << "ms";
    XIFriendList::Debug::DebugLog::getInstance().push(header.str());

    const uint32_t count = (topN < items.size()) ? topN : static_cast<uint32_t>(items.size());
    for (uint32_t i = 0; i < count; ++i) {
        const auto& label = items[i].first;
        const auto& s = items[i].second;
        std::ostringstream line;
        line << "  " << label
             << " count=" << s.count
             << " total=" << std::fixed << std::setprecision(2) << s.totalMs << "ms"
             << " max=" << s.maxMs << "ms";
        XIFriendList::Debug::DebugLog::getInstance().push(line.str());
    }
}

void PrintSummaryOnce(const char* tag, uint32_t topN) {
    if (!IsEnabled()) {
        return;
    }
    const std::string key = tag ? tag : "Unknown";
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        if (g_SummariesPrinted.find(key) != g_SummariesPrinted.end()) {
            return;
        }
        g_SummariesPrinted.insert(key);
    }
    PrintSummary(tag, topN);
}

void MarkFirstInteractive() {
    if (!IsEnabled()) {
        return;
    }
    bool expected = false;
    if (!g_FirstInteractivePrinted.compare_exchange_strong(expected, true)) {
        return;
    }
    PrintSummaryOnce("FirstInteractive", 10);
}

#endif // XIFRIENDLIST_PERF_COMPILED

} // namespace Debug
} // namespace XIFriendList


