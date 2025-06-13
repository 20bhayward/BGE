#pragma once

#include <string>
#include <fstream>
#include <memory>
#include <sstream>
#include <chrono>
#include <iomanip>

namespace BGE {

enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARNING = 3,
    ERROR = 4,
    CRITICAL = 5
};

class Logger {
public:
    static Logger& Instance();
    
    void Initialize(const std::string& logFilePath = "", LogLevel level = LogLevel::INFO);
    void Shutdown();
    
    void Log(LogLevel level, const std::string& category, const std::string& message);
    
    void Trace(const std::string& category, const std::string& message);
    void Debug(const std::string& category, const std::string& message);
    void Info(const std::string& category, const std::string& message);
    void Warning(const std::string& category, const std::string& message);
    void Error(const std::string& category, const std::string& message);
    void Critical(const std::string& category, const std::string& message);
    
    void SetLogLevel(LogLevel level) { m_logLevel = level; }
    LogLevel GetLogLevel() const { return m_logLevel; }
    
    void EnableConsoleOutput(bool enable) { m_consoleOutput = enable; }
    void EnableFileOutput(bool enable) { m_fileOutput = enable; }
    
private:
    Logger() = default;
    ~Logger() = default;
    
    std::string GetTimestamp();
    std::string LogLevelToString(LogLevel level);
    
    LogLevel m_logLevel = LogLevel::INFO;
    bool m_consoleOutput = true;
    bool m_fileOutput = false;
    std::unique_ptr<std::ofstream> m_logFile;
};

#define BGE_LOG_TRACE(category, message) BGE::Logger::Instance().Trace(category, message)
#define BGE_LOG_DEBUG(category, message) BGE::Logger::Instance().Debug(category, message)
#define BGE_LOG_INFO(category, message) BGE::Logger::Instance().Info(category, message)
#define BGE_LOG_WARNING(category, message) BGE::Logger::Instance().Warning(category, message)
#define BGE_LOG_ERROR(category, message) BGE::Logger::Instance().Error(category, message)
#define BGE_LOG_CRITICAL(category, message) BGE::Logger::Instance().Critical(category, message)

} // namespace BGE