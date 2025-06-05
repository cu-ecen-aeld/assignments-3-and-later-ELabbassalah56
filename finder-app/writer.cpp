#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <syslog.h>
#include <thread>
#include <chrono>
#include <atomic>

namespace fs = std::filesystem;
std::atomic<bool> isRunning(true);

class writer
{
public:
    writer(const fs::path &filePath, const std::string &str) noexcept
        :  _str{str},_file{filePath} {}

    ~writer() = default; // Explicitly define a destructor

    // Delete copy semantics
    writer(const writer &) = delete;
    writer &operator=(const writer &) = delete;

    // Implement move semantics
    writer(writer &&other) noexcept
        : _str(std::move(other._str)), _file(std::move(other._file)) {}

    writer &operator=(writer &&other) noexcept
    {
        if (this != &other)
        {
            _str = std::move(other._str);
            _file = std::move(other._file);
        }
        return *this;
    }

    bool writeToFile() const
    {
        try
        {
            // Ensure parent directory exists
            if (!fs::exists(_file.parent_path()))
            {
                fs::create_directories(_file.parent_path());
            }

            // Open file for writing
            std::ofstream file(_file);
            if (file)
            {
                syslog(LOG_DEBUG, "Writing '%s' to %s", _str.c_str(), _file.c_str());
                file << _str;
                syslog(LOG_INFO, "File written successfully: %s", _file.c_str());
                std::cout << "File written successfully: " << _file << std::endl;
                return true;
            }
            else
            {
                syslog(LOG_ERR, "Error opening file: %s", _file.c_str());
                std::cerr << "Error opening file: " << _file << std::endl;
                return false;
            }
        }
        catch (const fs::filesystem_error &e)
        {
            syslog(LOG_ERR, "Filesystem error: %s", e.what());
            std::cerr << "Filesystem error: " << e.what() << '\n';
            return false;
        }
    }

private:
    std::string _str;
    fs::path _file;
};

void animate_processing(const fs::path &filePath)
{
    const std::string animation[] = {".", "..", "..."};
    int idx = 0;

    while (isRunning)
    {
        std::cout << "\rProcessing in directory: " << filePath << " " << animation[idx] << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        idx = (idx + 1) % 3; // Cycle through . .. ...
    }

    // Once the animation stops, clean up the output.
    syslog(LOG_INFO, "\rProcessing in directory: %s Done!", filePath.c_str());
}

bool processWriterOperation(const fs::path &filePath, const std::string &str)
{
    writer obj{filePath, str};
    return obj.writeToFile();
}

int main(int argc, char const *argv[])
{
    if (argc == 3U)
    {
        openlog("Writer", LOG_PID | LOG_CONS, LOG_USER);
        // Example usage
        std::thread animationThread{animate_processing, argv[1]};
        std::thread processWriterOperationThread{processWriterOperation, argv[1], argv[2]};
        isRunning = false;
        animationThread.join();
        processWriterOperationThread.join();
        closelog();
    }
    else
    {
        syslog(LOG_ERR, "Usage: %s <file_path> <content_to_write>", argv[0]);
        std::cerr << "Usage: " << argv[0] << " <file_path> <content_to_write>\n";
        return 1;
    }
    return 0;
}
