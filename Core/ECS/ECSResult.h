#pragma once

#include "ECSConstants.h"
#include "EntityID.h"
#include <variant>
#include <string>
#include <optional>
#include <type_traits>

namespace BGE {

// Error codes for ECS operations
enum class ECSError {
    None = 0,
    InvalidEntity,
    InvalidComponent,
    ComponentNotFound,
    ComponentAlreadyExists,
    ArchetypeLimitReached,
    OutOfMemory,
    InvalidOperation,
    ConcurrentModification,
    SerializationError,
    ValidationError
};

// Error information with details
struct ECSErrorInfo {
    ECSError code;
    std::string message;
    std::string details;
    
    ECSErrorInfo(ECSError c, const std::string& msg, const std::string& det = "") 
        : code(c), message(msg), details(det) {}
};

// Result type for ECS operations
template<typename T>
class Result {
public:
    Result(T&& value) : m_data(std::move(value)) {}
    Result(const T& value) : m_data(value) {}
    Result(ECSErrorInfo error) : m_data(error) {}
    
    bool IsSuccess() const { 
        return std::holds_alternative<T>(m_data); 
    }
    
    bool IsError() const { 
        return std::holds_alternative<ECSErrorInfo>(m_data); 
    }
    
    T& GetValue() { 
        return std::get<T>(m_data); 
    }
    
    const T& GetValue() const { 
        return std::get<T>(m_data); 
    }
    
    T* TryGetValue() {
        return std::get_if<T>(&m_data);
    }
    
    const T* TryGetValue() const {
        return std::get_if<T>(&m_data);
    }
    
    const ECSErrorInfo& GetError() const { 
        return std::get<ECSErrorInfo>(m_data); 
    }
    
    T ValueOr(const T& defaultValue) const {
        if (auto* val = TryGetValue()) {
            return *val;
        }
        return defaultValue;
    }
    
    operator bool() const { return IsSuccess(); }
    
private:
    std::variant<T, ECSErrorInfo> m_data;
};

// Specialization for void results
template<>
class Result<void> {
public:
    Result() : m_error(std::nullopt) {}
    Result(ECSErrorInfo error) : m_error(error) {}
    
    bool IsSuccess() const { return !m_error.has_value(); }
    bool IsError() const { return m_error.has_value(); }
    
    const ECSErrorInfo& GetError() const { 
        return m_error.value(); 
    }
    
    operator bool() const { return IsSuccess(); }
    
private:
    std::optional<ECSErrorInfo> m_error;
};

// Helper functions for creating results
template<typename T>
Result<std::decay_t<T>> Success(T&& value) {
    return Result<std::decay_t<T>>(std::forward<T>(value));
}

inline Result<void> Success() {
    return Result<void>();
}

template<typename T>
Result<std::decay_t<T>> Error(ECSError code, const std::string& message, const std::string& details = "") {
    return Result<std::decay_t<T>>(ECSErrorInfo(code, message, details));
}

inline Result<void> Error(ECSError code, const std::string& message, const std::string& details = "") {
    return Result<void>(ECSErrorInfo(code, message, details));
}

// Convenience typedef for ECS operations
template<typename T>
using ECSResult = Result<T>;

// Validation helpers
class ECSValidator {
public:
    static bool IsValidEntityIndex(uint32_t index) {
        return index < EntityID::INDEX_MASK;
    }
    
    static bool IsValidComponentType(ComponentTypeID typeID) {
        return typeID < MAX_COMPONENTS;
    }
    
    static bool IsValidArchetypeIndex(uint32_t index) {
        return index < UINT32_MAX;
    }
    
    template<typename T>
    static Result<void> ValidateComponentData(const T& /*component*/) {
        // Perform component-specific validation
        // This can be specialized for different component types
        return Success();
    }
};

// Logging helpers for errors
#define ECS_LOG_ERROR(error) \
    BGE_LOG_ERROR("ECS", "Error " + std::to_string(static_cast<int>(error.code)) + \
                  ": " + error.message + " | " + error.details)

#define ECS_VALIDATE(condition, error_code, message) \
    if (!(condition)) { \
        auto err = ECSErrorInfo(error_code, message, #condition); \
        ECS_LOG_ERROR(err); \
        return Error<decltype(auto)>(error_code, message, #condition); \
    }

} // namespace BGE