#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>
#include <tlhelp32.h>
#include <iomanip>

#include "offsets.hpp"

template <typename T>
class vec3 {
public:
    T x;
    T y;
    T z;

    vec3() : x{}, y{}, z{} {}
    vec3(const T x, const T y, const T z) : x{x}, y{y}, z{z} {}
};

class nostalgia {
private:
    HANDLE h_process;
    DWORD pid;
public:
    nostalgia() : h_process { nullptr }, pid{} {}
    ~nostalgia() {
        if (h_process) {
            CloseHandle(h_process);
        }
    }

    bool attach_to_process(const std::wstring& executable_name) {
        HANDLE h_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
        if (h_snapshot == INVALID_HANDLE_VALUE) {
            DWORD error = GetLastError();
            std::wcerr << "Failed to retrieve snapshot of processes, error: " << error << "\n\n";
            return false;
        }

        PROCESSENTRY32W process_entry{};
        process_entry.dwSize = sizeof(PROCESSENTRY32W);

        if (!Process32FirstW(h_snapshot, &process_entry)) {
            DWORD error = GetLastError();
            std::wcerr << "Failed to retrieve first process from snapshot, error: " << error << "\n\n";
            CloseHandle(h_snapshot);
            return false;
        }

        bool found = false;
        do {
            if (executable_name == process_entry.szExeFile) {
                std::wcout << "Executable name: " << process_entry.szExeFile << "\nPID: " << process_entry.th32ProcessID << "\n\n";
                pid = process_entry.th32ProcessID;
                found = true;
                break;
            }
        } while (Process32NextW(h_snapshot, &process_entry));
        CloseHandle(h_snapshot);

        if (!found) {
            std::wcerr << "Failed to find process with executable name: " << executable_name << "\n\n";
            return false;
        }

        h_process = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
        if (!h_process) {
            DWORD error = GetLastError();
            std::wcerr << "Failed to open target process, error: " << error << "\n\n";
            return false;
        }

        return true;
    }

    bool attach_to_window(const std::wstring& window_name) {
        HWND hwnd = FindWindowW(NULL, window_name.c_str());
        if (!hwnd) {
            std::wcerr << "Failed to find window: " << window_name << "\n\n";
            return false;
        }

        DWORD pid;
        if (!GetWindowThreadProcessId(hwnd, &pid)) {
            DWORD error = GetLastError();
            std::wcerr << "Failed to retrieve process ID from window, error: " << error << "\n\n";
            return false;
        }

        std::wcout << "Window name: " << window_name << "\nPID: " << pid << "\n\n";

        h_process = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
        if (!h_process) {
            DWORD error = GetLastError();
            std::wcerr << "Failed to open target process, error: " << error << "\n\n";
            return false;
        }

        return true;
    }

    template <typename T>
    void write_mem(const std::uintptr_t address, const T data) {
        if (!WriteProcessMemory(h_process, reinterpret_cast<T*>(address), &data, sizeof(T), NULL)) {
            DWORD error = GetLastError();
            std::wcout << "Failed to write memory at 0x" << std::hex << std::uppercase << std::setw(sizeof(uintptr_t) * 2) << std::setfill(L'0') << address << std::nouppercase << std::dec << " to target process, error: " << error << "\n\n";
        }
    }

    template <typename T>
    T read_mem(const std::uintptr_t address) {
        T result{};
        if (!ReadProcessMemory(h_process, reinterpret_cast<T*>(address), &result, sizeof(T), NULL)) {
            DWORD error = GetLastError();
            std::wcout << "Failed to read memory at 0x" << std::hex << std::uppercase << std::setw(sizeof(uintptr_t) * 2) << std::setfill(L'0') << address << std::nouppercase << std::dec << " from target process, error: " << error << "\n\n";
        }

        return result;
    }

    void update_points(const int points) {
        std::wcout << "Updating points to: " << points << "\n\n";
        write_mem<int>(g_offset_points, points);
    }

    int get_health() {
        std::wcout << "Fetching health...\n\n";
        return read_mem<int>(g_offset_health);
    }

    std::vector<std::uintptr_t> get_entities() {
        std::vector<std::uintptr_t> entities;
        for (int i = 0; i < 25; ++i) {
            auto address = read_mem<std::uintptr_t>(g_offset_entity_list + (i * g_offset_next_entity));
            if (address != NULL) {
                entities.push_back(address);
            }
        }

        return entities;
    }

    void print_positions() {
        auto entities = get_entities();
        for (int i = 0; i < entities.size(); ++i) {
            auto health = read_mem<int>(entities[i] + g_offset_entity_health);
            if (health > 0) {
                auto coords = read_mem<vec3<float>>(entities[i] + g_offset_entity_coords);
                std::wcout << "[Entity " << i << "]\n";
                std::wcout << "Health: " << health << '\n';
                std::wcout << "X: " << coords.x << ", Y: " << coords.y << ", Z: " << coords.z << "\n\n";
            }
        }
    }
};

int main() {
    nostalgia client{};
    //if (!client.attach_to_window(L"Plutonium T4 Co-Op (r5316) (LAN)")) {
    if (!client.attach_to_process(L"plutonium-bootstrapper-win32.exe")) {
        std::wcerr << "Failed to attach to process\n\n";
        return 1;
    }

    client.update_points(9999);
    client.print_positions();

    return 0;
}
