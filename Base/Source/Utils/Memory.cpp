#include "Memory.hpp"

void Ntdll::Initialize() {
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    NtQuerySystemInformation = (TNtQuerySystemInformation)(GetProcAddress(ntdll, "NtQuerySystemInformation"));
    NtDuplicateObject = (TNtDuplicateObject)(GetProcAddress(ntdll, "NtDuplicateObject"));
    NtOpenProcess = (TNtOpenProcess)(GetProcAddress(ntdll, "NtOpenProcess"));
    NtAllocateVirtualMemory = (TNtAllocateVirtualMemory)(GetProcAddress(ntdll, "NtAllocateVirtualMemory"));
    NtFreeVirtualMemory = (TNtFreeVirtualMemory)(GetProcAddress(ntdll, "NtFreeVirtualMemory"));
    NtProtectVirtualMemory = (TNtProtectVirtualMemory)(GetProcAddress(ntdll, "NtProtectVirtualMemory"));
    NtReadVirtualMemory = (TNtReadVirtualMemory)(GetProcAddress(ntdll, "NtReadVirtualMemory"));
    NtWriteVirtualMemory = (TNtWriteVirtualMemory)(GetProcAddress(ntdll, "NtWriteVirtualMemory"));
}

DWORD Memory::FindProcess(const char* process_name) {
    ULONG size = 0x10000;
    uint8_t* buffer = (uint8_t*)(calloc(1, size));

    while (Ntdll::NtQuerySystemInformation(SystemProcessInformation, buffer, size, nullptr) == STATUS_INFO_LENGTH_MISMATCH) {
        buffer = (uint8_t*)(realloc(buffer, size *= 2));
    }

    wchar_t wide_name[256] = {};
    mbstowcs(wide_name, process_name, 256);

    DWORD pid = 0;
    SYSTEM_PROCESS_INFO* process_info = (SYSTEM_PROCESS_INFO*)(buffer);

    while (true) {
        if (process_info->ImageName.Buffer && _wcsicmp(process_info->ImageName.Buffer, wide_name) == 0) {
            pid = (DWORD)(uintptr_t)(process_info->UniqueProcessId);
            break;
        }

        if (!process_info->NextEntryOffset) {
            break;
        }
        process_info = (SYSTEM_PROCESS_INFO*)((uint8_t*)(process_info)+process_info->NextEntryOffset);
    }

    free(buffer);
    return pid;
}

Process::Process(const std::string& process_name, const std::string& process_name2) :
    process_name2(process_name2),
    process_name(process_name),
    process_id(0),
    process_handle(nullptr),
    image_base(0),
    image_size(0)
{}

Process::~Process() {
    Detach();
}

bool Process::Attach() {
    process_id = Memory::FindProcess(process_name.c_str());
    if (!process_id) {
        Logger::Singleton()->Print(LOG_INFO, "Failed to find process id: ??");
        return false;
    }
    Logger::Singleton()->Printf(LOG_INFO, "Found process id: 0x%d", process_id);

    if (process_name2.empty()) {
        process_handle = OpenHandle(PROCESS_ALL_ACCESS);
    }
    else {
        process_handle = StealHandle(process_name2.c_str(), PROCESS_ALL_ACCESS);
    }

    if (!process_handle) {
        Logger::Singleton()->Print(LOG_INFO, "Failed to open handle: ??");
        return false;
    }
    Logger::Singleton()->Printf(LOG_INFO, "Opened handle: 0x%p", process_handle);

    auto image_info = GetModuleInfo(process_name.c_str());
    image_base = image_info.first;
    if (!image_base) {
        Logger::Singleton()->Print(LOG_INFO, "Failed to find image base: ??");
        return false;
    }
    Logger::Singleton()->Printf(LOG_INFO, "Found image base: 0x%p", image_base);

    image_size = image_info.second;
    if (!image_size) {
        Logger::Singleton()->Print(LOG_INFO, "Failed to find image size: ??");
        return false;
    }
    Logger::Singleton()->Printf(LOG_INFO, "Found image size: 0x%zu", image_size);
    return true;
}

void Process::Detach() {
    if (process_handle) {
        CloseHandle(process_handle);
        process_handle = nullptr;
    }

    image_size = 0;
    image_base = 0;
    process_id = 0;
}

bool Process::IsAlive() {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    PROCESSENTRY32W process_entry = {};
    process_entry.dwSize = sizeof(process_entry);

    if (Process32FirstW(snapshot, &process_entry)) {
        do {
            if (process_entry.th32ProcessID == process_id) {
                return true;
            }
        } while (Process32NextW(snapshot, &process_entry));
    }

    CloseHandle(snapshot);
    return false;
}

HANDLE Process::StealHandle(const char* process_name, DWORD desired_access) {
    DWORD target_pid = Memory::FindProcess(process_name);
    if (!target_pid) {
        return nullptr;
    }

    HANDLE target_handle = OpenProcess(PROCESS_DUP_HANDLE, FALSE, target_pid);
    if (!target_handle) {
        return nullptr;
    }

    ULONG size = 0x10000;
    SYSTEM_HANDLE_INFORMATION* system_info = (SYSTEM_HANDLE_INFORMATION*)(calloc(1, size));

    while (Ntdll::NtQuerySystemInformation(SystemHandleInformation, system_info, size, nullptr) == STATUS_INFO_LENGTH_MISMATCH) {
        system_info = (SYSTEM_HANDLE_INFORMATION*)(realloc(system_info, size *= 2));
    }

    HANDLE result = nullptr;
    for (ULONG i = 0; i < system_info->HandleCount; i++) {
        auto& handle = system_info->Handles[i];
        if (handle.ProcessId != target_pid) {
            continue;
        }

        HANDLE duplicated_handle = nullptr;

        NTSTATUS nt_status = Ntdll::NtDuplicateObject(target_handle, (HANDLE)(uintptr_t)(handle.Handle), GetCurrentProcess(), &duplicated_handle, desired_access, 0, 0);
        if (!NT_SUCCESS(nt_status)) {
            continue;
        }

        if (GetProcessId(duplicated_handle) == process_id) {
            result = duplicated_handle;
            break;
        }
        CloseHandle(duplicated_handle);
    }

    free(system_info);
    CloseHandle(target_handle);
    return result;
}

HANDLE Process::OpenHandle(DWORD desired_access) {
    CLIENT_ID client_id = {};
    client_id.UniqueProcess = ULongToHandle(process_id);
    client_id.UniqueThread = nullptr;

    OBJECT_ATTRIBUTES object_attributes = {};
    object_attributes.Length = sizeof(object_attributes);

    HANDLE process_handle = nullptr;

    NTSTATUS status = Ntdll::NtOpenProcess(
        &process_handle,
        desired_access,
        &object_attributes,
        &client_id
    );
    return NT_SUCCESS(status) ? process_handle : nullptr;
}

std::filesystem::path Process::GetPath() {
    char buffer[MAX_PATH];
    DWORD buffer_size = sizeof(buffer);

    if (QueryFullProcessImageNameA(process_handle, 0, buffer, &buffer_size)) {
        return std::filesystem::path(buffer);
    }
    return {};
}

std::pair<uintptr_t, size_t> Process::GetModuleInfo(const char* module_name) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, process_id);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return std::make_pair(0, 0);
    }

    MODULEENTRY32W module_entry = {};
    module_entry.dwSize = sizeof(module_entry);

    wchar_t wide_name[256];
    mbstowcs(wide_name, module_name, 256);

    uintptr_t module_base = 0;
    size_t module_size = 0;

    if (Module32FirstW(snapshot, &module_entry)) {
        do {
            if (_wcsicmp(module_entry.szModule, wide_name) == 0) {
                module_base = reinterpret_cast<uintptr_t>(module_entry.modBaseAddr);
                module_size = static_cast<size_t>(module_entry.modBaseSize);
                break;
            }
        } while (Module32NextW(snapshot, &module_entry));
    }

    CloseHandle(snapshot);
    return std::make_pair(module_base, module_size);
}

uintptr_t Process::GetModuleExport(uintptr_t module_base, const char* function_name) {
    IMAGE_DOS_HEADER dos_header = Read<IMAGE_DOS_HEADER>(module_base);
    IMAGE_NT_HEADERS nt_headers = Read<IMAGE_NT_HEADERS>(module_base + dos_header.e_lfanew);

    IMAGE_DATA_DIRECTORY export_directory_data = nt_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (export_directory_data.VirtualAddress == 0 || export_directory_data.Size == 0) {
        return 0;
    }

    IMAGE_EXPORT_DIRECTORY export_directory = Read<IMAGE_EXPORT_DIRECTORY>(module_base + export_directory_data.VirtualAddress);
    std::vector<DWORD> name_rvas(export_directory.NumberOfNames);
    std::vector<WORD> ordinals(export_directory.NumberOfNames);
    std::vector<DWORD> function_rvas(export_directory.NumberOfFunctions);

    if (!ReadPhysical(module_base + export_directory.AddressOfNames, name_rvas.data(), sizeof(DWORD) * name_rvas.size())) {
        return 0;
    }

    if (!ReadPhysical(module_base + export_directory.AddressOfNameOrdinals, ordinals.data(), sizeof(WORD) * ordinals.size())) {
        return 0;
    }

    if (!ReadPhysical(module_base + export_directory.AddressOfFunctions, function_rvas.data(), sizeof(DWORD) * function_rvas.size())) {
        return 0;
    }

    char buffer[256];
    for (size_t i = 0; i < name_rvas.size(); ++i) {
        if (!ReadPhysical(module_base + name_rvas[i], buffer, sizeof(buffer))) {
            continue;
        }

        if (strcmp(buffer, function_name) == 0) {
            WORD ordinal = ordinals[i];
            if (ordinal >= function_rvas.size()) {
                return 0;
            }
            return module_base + function_rvas[ordinal];
        }
    }
    return 0;
}

std::vector<uint8_t> Process::ReadBytes(uintptr_t address, size_t size) {
    std::vector<uint8_t> bytes(size);
    SIZE_T bytes_read = 0;
    NTSTATUS nt_status = Ntdll::NtReadVirtualMemory(process_handle, reinterpret_cast<PVOID>(address), bytes.data(), bytes.size(), &bytes_read);

    if (NT_SUCCESS(nt_status) && bytes_read == size) {
        return bytes;
    }
    return std::vector<uint8_t>();
}

bool Process::WriteBytes(uintptr_t address, std::vector<uint8_t>& bytes) {
    SIZE_T bytes_written = 0;
    NTSTATUS nt_status = Ntdll::NtWriteVirtualMemory(process_handle, reinterpret_cast<PVOID>(address), bytes.data(), bytes.size(), &bytes_written);
    return NT_SUCCESS(nt_status) && bytes_written == bytes.size();
}

bool Process::ReadPhysical(uintptr_t address, void* buffer, size_t size) {
    SIZE_T bytes_read = 0;
    NTSTATUS nt_status = Ntdll::NtReadVirtualMemory(process_handle, reinterpret_cast<PVOID>(address), buffer, size, &bytes_read);
    return NT_SUCCESS(nt_status) && bytes_read == size;
}

bool Process::WritePhysical(uintptr_t address, void* buffer, size_t size) {
    SIZE_T bytes_written = 0;
    NTSTATUS nt_status = Ntdll::NtWriteVirtualMemory(process_handle, reinterpret_cast<PVOID>(address), buffer, size, &bytes_written);
    return NT_SUCCESS(nt_status) && bytes_written == size;
}

uintptr_t Process::Allocate(size_t size, DWORD allocation_type, DWORD protect) {
    PVOID region_address = nullptr;
    SIZE_T region_size = size;

    NTSTATUS nt_status = Ntdll::NtAllocateVirtualMemory(process_handle, &region_address, 0, &region_size, allocation_type, protect);
    if (NT_SUCCESS(nt_status)) {
        return reinterpret_cast<uintptr_t>(region_address);
    }
    return 0;
}

bool Process::Free(uintptr_t address, DWORD free_type) {
    PVOID region_address = reinterpret_cast<PVOID>(address);
    SIZE_T region_size = 0;

    NTSTATUS nt_status = Ntdll::NtFreeVirtualMemory(process_handle, &region_address, &region_size, free_type);
    if (NT_SUCCESS(nt_status)) {
        return true;
    }
    return false;
}

bool Process::Protect(uintptr_t region_address, size_t region_size, DWORD protect, PDWORD old_protect) {
    PVOID address = reinterpret_cast<PVOID>(region_address);
    SIZE_T size = static_cast<SIZE_T>(region_size);
    ULONG old = 0;

    NTSTATUS nt_status = Ntdll::NtProtectVirtualMemory(process_handle, &address, &size, protect, &old);
    if (old_protect) {
        *old_protect = static_cast<DWORD>(old);
    }
    return NT_SUCCESS(nt_status);
}

std::string Process::ReadString(uintptr_t address) {
    sso_string sso = Read<sso_string>(address);
    if (sso.length < 1 || sso.length > 255) {
        return "";
    }

    if (sso.length >= 16) {
        auto bytes = ReadBytes(sso.data.pointer, sso.length);
        if (bytes.empty()) {
            return "";
        }
        return std::string(reinterpret_cast<char*>(bytes.data()), sso.length);
    }
    else {
        return std::string(sso.data.buffer, sso.length);
    }
}

void Process::WriteString(uintptr_t address, const std::string& str) {
    sso_string sso = Read<sso_string>(address);

    bool old_was_long_str = (sso.length >= 16);
    auto old_ptr = sso.data.pointer;

    if (str.length() > sso.capacity) {
        size_t new_capacity = sso.capacity == 0 ? 16 : sso.capacity;
        while (new_capacity < str.length()) {
            new_capacity *= 2;
        }

        auto data_ptr = Allocate(new_capacity, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!data_ptr) {
            return;
        }

        sso.data.pointer = data_ptr;
        sso.capacity = new_capacity;
    }
    sso.length = static_cast<uint32_t>(str.length());

    if (sso.length >= 16) {
        WritePhysical(sso.data.pointer, (void*)(str.data()), str.length());
        Write<sso_string>(address, sso);
    }
    else {
        memset(sso.data.buffer, 0, 16);
        memcpy(sso.data.buffer, str.data(), str.length());
        Write<sso_string>(address, sso);
    }

    if (old_was_long_str && old_ptr && old_ptr != sso.data.pointer) {
        Free(old_ptr, MEM_RELEASE);
    }
}