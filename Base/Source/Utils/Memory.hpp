#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <filesystem>
#include <string>
#include "Ntdll.hpp"
#include "Logger/Logger.hpp"

struct vector_block {
    uintptr_t first;
    uintptr_t last;
    uintptr_t end;
};

struct shared_ptr_block {
    uintptr_t ptr;
    uintptr_t control_block;
};

struct sso_string {
    union {
        char buffer[16];
        uintptr_t pointer;
    } data;

    uint32_t length;
    uint32_t capacity;
};

class Ntdll {
public:
    static inline TNtQuerySystemInformation NtQuerySystemInformation = nullptr;
    static inline TNtDuplicateObject NtDuplicateObject = nullptr;
    static inline TNtOpenProcess NtOpenProcess = nullptr;
    static inline TNtAllocateVirtualMemory NtAllocateVirtualMemory = nullptr;
    static inline TNtFreeVirtualMemory NtFreeVirtualMemory = nullptr;
    static inline TNtProtectVirtualMemory NtProtectVirtualMemory = nullptr;
    static inline TNtReadVirtualMemory NtReadVirtualMemory = nullptr;
    static inline TNtWriteVirtualMemory NtWriteVirtualMemory = nullptr;

    static void Initialize();
};

class Memory {
public:
    static DWORD FindProcess(const char* process_name);
};

class Process {
public:
    std::string process_name2;
    std::string process_name;
    DWORD process_id;
    HANDLE process_handle;
    uintptr_t image_base;
    size_t image_size;

    Process(const std::string& process_name, const std::string& process_name2 = "");
    ~Process();

    bool Attach();
    void Detach();

    bool IsAlive();

    HANDLE StealHandle(const char* process_name, DWORD desired_access);
    HANDLE OpenHandle(DWORD desired_access);

    std::filesystem::path GetPath();

    std::pair<uintptr_t, size_t> GetModuleInfo(const char* module_name);
    uintptr_t GetModuleExport(uintptr_t module_base, const char* function_name);

    template<typename T>
    T Read(uintptr_t address) {
        T buffer = {};
        SIZE_T bytes_read = 0;

        NTSTATUS nt_status = Ntdll::NtReadVirtualMemory(process_handle, reinterpret_cast<PVOID>(address), &buffer, sizeof(T), &bytes_read);
        if (NT_SUCCESS(nt_status) && bytes_read == sizeof(T)) {
            return buffer;
        }
        return T{};
    }

    template<typename T>
    bool Write(uintptr_t address, const T& value) {
        SIZE_T bytes_written = 0;
        NTSTATUS nt_status = Ntdll::NtWriteVirtualMemory(process_handle, reinterpret_cast<PVOID>(address), const_cast<PVOID>(reinterpret_cast<const void*>(&value)), sizeof(T), &bytes_written);
        return NT_SUCCESS(nt_status) && bytes_written == sizeof(T);
    }

    std::vector<uint8_t> ReadBytes(uintptr_t address, size_t size);
    bool WriteBytes(uintptr_t address, std::vector<uint8_t>& bytes);

    bool ReadPhysical(uintptr_t address, void* buffer, size_t size);
    bool WritePhysical(uintptr_t address, void* buffer, size_t size);

    uintptr_t Allocate(size_t size, DWORD allocation_type = MEM_COMMIT | MEM_RESERVE, DWORD protect = PAGE_READWRITE);
    bool Free(uintptr_t address, DWORD free_type = MEM_RELEASE);
    bool Protect(uintptr_t protect_address, size_t region_size, DWORD protect, PDWORD old_protect = nullptr);

    std::string ReadString(uintptr_t address);
    void WriteString(uintptr_t address, const std::string& str);
};