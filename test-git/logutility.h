#ifndef LOGUTILITY_H
#define LOGUTILITY_H
#pragma once

#include <iostream>

#define I_LOG(type, message) do { I_Log(type) << message; } while (0);

enum class LogType
{
    Info = 0,
    Warning = 1,
    Error = 2
};

inline std::ostream& I_Log(LogType type = LogType::Info) {
    switch (type) {
        case LogType::Info:
            return std::cout << "[INFO] ";
        case LogType::Warning:
            return std::cout << "[WARNING] ";
        case LogType::Error:
            return std::cout << "[ERROR] ";
    }
    return std::cout << "[UNKNOWN] ";
}

#endif // LOGUTILITY_H
