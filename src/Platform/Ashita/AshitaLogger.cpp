#include "AshitaLogger.h"
#include <sstream>

#ifndef BUILDING_TESTS
#include <Ashita.h>
#endif

namespace XIFriendList {
namespace Platform {
namespace Ashita {

AshitaLogger::AshitaLogger()
    : logManager_(nullptr)
{
}

void AshitaLogger::setLogManager(ILogManager* logManager) {
    logManager_ = logManager;
}

void AshitaLogger::debug(const std::string& message) {
    log(::XIFriendList::App::LogLevel::Debug, "", message);
}

void AshitaLogger::info(const std::string& message) {
    log(::XIFriendList::App::LogLevel::Info, "", message);
}

void AshitaLogger::warning(const std::string& message) {
    log(::XIFriendList::App::LogLevel::Warning, "", message);
}

void AshitaLogger::error(const std::string& message) {
    log(::XIFriendList::App::LogLevel::Error, "", message);
}

void AshitaLogger::log(::XIFriendList::App::LogLevel level,
                       const std::string& module,
                       const std::string& message) {
    if (logManager_ == nullptr) {
        return;  // No logging available
    }
    
#ifdef BUILDING_TESTS
    (void)level;
    (void)module;
    (void)message;
#else
    std::string formatted = formatMessage(module, message);
    
    uint32_t ashitaLevel;
    switch (level) {
        case ::XIFriendList::App::LogLevel::Debug:
            ashitaLevel = 0;
            break;
        case ::XIFriendList::App::LogLevel::Info:
            ashitaLevel = 1;
            break;
        case ::XIFriendList::App::LogLevel::Warning:
            ashitaLevel = 2;
            break;
        case ::XIFriendList::App::LogLevel::Error:
            ashitaLevel = 3;
            break;
        default:
            ashitaLevel = 1;
            break;
    }
    
    const char* moduleName = module.empty() ? "XIFriendList" : module.c_str();
    
    logManager_->Log(ashitaLevel, moduleName, formatted.c_str());
#endif
}

std::string AshitaLogger::formatMessage(const std::string& module, const std::string& message) const {
    if (module.empty()) {
        return message;
    }
    
    std::ostringstream oss;
    oss << "[" << module << "] " << message;
    return oss.str();
}

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

