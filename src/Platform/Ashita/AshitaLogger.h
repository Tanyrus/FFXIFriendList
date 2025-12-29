
#ifndef PLATFORM_ASHITA_LOGGER_H
#define PLATFORM_ASHITA_LOGGER_H

#include "App/Interfaces/ILogger.h"
#include <string>

struct ILogManager;

namespace XIFriendList {
namespace Platform {
namespace Ashita {

class AshitaLogger : public ::XIFriendList::App::ILogger {
public:
    AshitaLogger();
    ~AshitaLogger() = default;
    void setLogManager(ILogManager* logManager);
    void debug(const std::string& message) override;
    void info(const std::string& message) override;
    void warning(const std::string& message) override;
    void error(const std::string& message) override;
    void log(::XIFriendList::App::LogLevel level,
             const std::string& module,
             const std::string& message) override;

private:
    ILogManager* logManager_;
    std::string formatMessage(const std::string& module, const std::string& message) const;
};

} // namespace Ashita
} // namespace Platform
} // namespace XIFriendList

#endif // PLATFORM_ASHITA_LOGGER_H

