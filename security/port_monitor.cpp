// === PORT MONITOR (version 1) ===
//

#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

class TcpSocketEntry {
    private:
    public:
    std::string localAddress;
    unsigned int localPort;
    std::string state;
    std::string exposure;
};

class SocketMonitor {
    private:
    std::string tcpStateFromHex(const std::string& stateHex) {
        if (stateHex == "0A") {
            return "LISTEN";
        }

        return "OTHER";
    }

    std::string hexToStringIPv4Address(const std::string& hexAddress) {
        if (hexAddress.length() != 8) {
            return "invalid";
        }

        unsigned int byte1 = std::stoul(hexAddress.substr(6, 2), nullptr, 16);
        unsigned int byte2 = std::stoul(hexAddress.substr(4, 2), nullptr, 16);
        unsigned int byte3 = std::stoul(hexAddress.substr(2, 2), nullptr, 16);
        unsigned int byte4 = std::stoul(hexAddress.substr(0, 2), nullptr, 16);

        return std::to_string(byte1) + "." + std::to_string(byte2) + "." + std::to_string(byte3) + "." +std::to_string(byte4);
    }

    unsigned int hexToStringPort(const std::string& hexPort) {
        return std::stoul(hexPort, nullptr, 16);
    }

    std::string classifyExposure(const std::string& address) {
        if (address == "127.0.0.1" ||
            address.rfind("127.", 0) == 0) {
                return "LOOPBACK";
        }

        if (address == "0.0.0.0") {
            return "ALL_INTERFACES";
        }

        return "SPECIFIC_INTERFACE";
    }

    TcpSocketEntry parseTcpLine(const std::string& line) {
        std::istringstream lineStream(line);

        std::string slot;
        std::string localAddressPort;
        std::string remoteAddressPort;
        std::string stateHex;

        lineStream >> slot >> localAddressPort >> remoteAddressPort >> stateHex;

        size_t colonPosition = localAddressPort.find(':');

        std::string localAddressHex = localAddressPort.substr(0, colonPosition);
        std::string localPortHex = localAddressPort.substr(colonPosition + 1);

        TcpSocketEntry entry;
        entry.localAddress = hexToStringIPv4Address(localAddressHex);
        entry.localPort = hexToStringPort(localPortHex);
        entry.state = tcpStateFromHex(stateHex);
        entry.exposure = classifyExposure(entry.localAddress);

        return entry;
    }

    std::vector<TcpSocketEntry> readTcpSockets() {
        std::vector<TcpSocketEntry> entries;

        std::ifstream tcpFile("/proc/net/tcp");

        if (!tcpFile.is_open()) {
            std::cerr << "WARNING: Could not open /proc/net/tcp." << std::endl;
            return entries;
        }

        std::string line;

        // Skip header line:
        std::getline(tcpFile, line);

        while (std::getline(tcpFile, line)) {
            TcpSocketEntry entry = parseTcpLine(line);

            if (entry.state == "LISTEN") {
                entries.push_back(entry);
            }
        }

        return entries;
    }
    
    public:
    SocketMonitor() {}
    ~SocketMonitor() {}

    void initialize() {
        std::vector<TcpSocketEntry> listeningSockets = readTcpSockets();

        std::cout << std::left
                  << std::setw(18) << "LOCAL ADDRESS"
                  << std::setw(10) << "PORT"
                  << std::setw(10) << "STATE"
                  << std::setw(20) << "EXPOSURE"
                  << std::endl; 

        std::cout << std::string(58, '-') << std::endl;

        for (const TcpSocketEntry& socket : listeningSockets) {
            std::cout << std::left
                      << std::setw(18) << socket.localAddress
                      << std::setw(10) << socket.localPort
                      << std::setw(10) << socket.state
                      << std::setw(20) << socket.exposure
                      << std::endl;
        }
    }
};

int main() {

    SocketMonitor socketMonitor;
    socketMonitor.initialize();

    return 0;
}
