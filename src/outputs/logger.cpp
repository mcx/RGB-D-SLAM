#include "logger.hpp"
#include <filesystem>
#include <iostream>
#include <mutex>

namespace rgbd_slam::outputs {

enum class InfoLevel
{
    ALL = 0,  // display all logs
    LOW = 1,  // show logs low and above
    MED = 2,  // show logs medium and above
    HIGH = 3, // show logs high and above

    NONE = 100, // deactivate all logs
};

// prevent too much logging
constexpr InfoLevel INFO_LEVEL = InfoLevel::ALL;

void log(const std::string_view& message, const std::source_location& location) noexcept
{
    if (INFO_LEVEL <= InfoLevel::LOW)
    {
        static std::mutex mut;
        std::scoped_lock<std::mutex> lock(mut);

        // display in blue
        std::cout << "\x1B[34m[INF] " << std::filesystem::path(location.file_name()).filename().string() << "("
                  << location.line() << ":" << location.column()
                  << ") "
                  //<< location.function_name() << " | "
                  << message << "\033[0m\n";
    }
}
void log_warning(const std::string_view& message, const std::source_location& location) noexcept
{
    if (INFO_LEVEL <= InfoLevel::MED)
    {
        static std::mutex mut;
        std::scoped_lock<std::mutex> lock(mut);

        // display in yellow
        std::cerr << "\x1B[33m[WARN] " << std::filesystem::path(location.file_name()).filename().string() << "("
                  << location.line() << ":" << location.column()
                  << ") "
                  //<< location.function_name() << " | "
                  << message << "\033[0m\n";
    }
}
void log_error(const std::string_view& message, const std::source_location& location) noexcept
{
    if (INFO_LEVEL <= InfoLevel::HIGH)
    {
        static std::mutex mut;
        std::scoped_lock<std::mutex> lock(mut);

        // display in red
        std::cerr << "\x1B[31m[ERR] " << std::filesystem::path(location.file_name()).filename().string() << "("
                  << location.line() << ":" << location.column()
                  << ") "
                  //<< location.function_name() << " | "
                  << message << "\033[0m\n";
    }
}

} // namespace rgbd_slam::outputs
