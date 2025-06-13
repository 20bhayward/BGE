#include "Logger.h"
#include <iostream>

namespace BGE {

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

void Logger::Initialize(const std::string& logFilePath, LogLevel level) {
    m_logLevel = level;
    
    if (!logFilePath.empty()) {
        m_logFile = std::make_unique<std::ofstream>(logFilePath, std::ios::app);
        m_fileOutput = m_logFile->is_open();
        if (!m_fileOutput) {
            std::cerr << "Failed to open log file: " << logFilePath << std::endl;
        }
    }
}

void Logger::Shutdown() {
    if (m_logFile) {
        m_logFile->close();
        m_logFile.reset();
    }
    m_fileOutput = false;
}

void Logger::Log(LogLevel level, const std::string& category, const std::string& message) {
    if (level < m_logLevel) {
        return;
    }
    
    std::string timestamp = GetTimestamp();
    std::string levelStr = LogLevelToString(level);
    
    std::ostringstream oss;
    oss << "[" << timestamp << "] [" << levelStr << "] [" << category << "] " << message;
    std::string logMessage = oss.str();
    
    if (m_consoleOutput) {
        if (level >= LogLevel::ERROR) {
            std::cerr << logMessage << std::endl;
        } else {
            std::cout << logMessage << std::endl;
        }
    }
    
    if (m_fileOutput && m_logFile) {
        *m_logFile << logMessage << std::endl;
        m_logFile->flush();
    }
}

void Logger::Trace(const std::string& category, const std::string& message) {
    Log(LogLevel::TRACE, category, message);
}

void Logger::Debug(const std::string& category, const std::string& message) {
    Log(LogLevel::DEBUG, category, message);
}

void Logger::Info(const std::string& category, const std::string& message) {
    Log(LogLevel::INFO, category, message);
}

void Logger::Warning(const std::string& category, const std::string& message) {
    Log(LogLevel::WARNING, category, message);
}

void Logger::Error(const std::string& category, const std::string& message) {
    Log(LogLevel::ERROR, category, message);
}

void Logger::Critical(const std::string& category, const std::string& message) {
    Log(LogLevel::CRITICAL, category, message);
}

std::string Logger::GetTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

std::string Logger::LogLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

} // namespace BGE