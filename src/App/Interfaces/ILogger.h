
#ifndef APP_ILOGGER_H
#define APP_ILOGGER_H

#include <string>

namespace XIFriendList {
namespace App {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void debug(const std::string& message) = 0;
    virtual void info(const std::string& message) = 0;
    virtual void warning(const std::string& message) = 0;
    virtual void error(const std::string& message) = 0;
    virtual void log(LogLevel level, 
                     const std::string& module,
                     const std::string& message) = 0;
};

} // namespace App
} // namespace XIFriendList

#endif // APP_ILOGGER_H

