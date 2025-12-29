
#ifndef DEBUG_PERF_H
#define DEBUG_PERF_H

#include <cstdint>

namespace XIFriendList {
namespace Debug {

bool IsEnabled();

void Initialize();

void Reset();

void PrintSummary(const char* tag, uint32_t topN = 10);

void PrintSummaryOnce(const char* tag, uint32_t topN = 10);

void MarkFirstInteractive();

class Scope {
public:
    explicit Scope(const char* label);
    ~Scope();
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;

private:
    const char* label_{ nullptr };
    bool enabled_{ false };
    uint64_t startTicks_{ 0 };
};

} // namespace Debug
} // namespace XIFriendList

#define XIFRIENDLIST_PERF_CONCAT_INNER(a, b) a##b
#define XIFRIENDLIST_PERF_CONCAT(a, b) XIFRIENDLIST_PERF_CONCAT_INNER(a, b)

#if defined(_DEBUG) || defined(XIFRIENDLIST_PERF_RELEASE)
    #define XIFRIENDLIST_PERF_COMPILED 1
    #define PERF_SCOPE(label) ::XIFriendList::Debug::Scope XIFRIENDLIST_PERF_CONCAT(perfScope_, __LINE__)(label)
#else
    #define XIFRIENDLIST_PERF_COMPILED 0
    #define PERF_SCOPE(label) do { (void)sizeof(label); } while (0)
#endif

#endif // DEBUG_PERF_H


