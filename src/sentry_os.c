#include "sentry_os.h"
#include "sentry_logger.h"
#include "sentry_string.h"
#include "sentry_utils.h"

#ifdef SENTRY_PLATFORM_WINDOWS

#    include <assert.h>
#    include <windows.h>
#    define CURRENT_VERSION "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"

void *
sentry__try_file_version(const LPCWSTR filename)
{
    const DWORD size = GetFileVersionInfoSizeW(filename, NULL);
    if (!size) {
        return NULL;
    }

    void *ffibuf = sentry_malloc(size);
    if (!GetFileVersionInfoW(filename, 0, size, ffibuf)) {
        sentry_free(ffibuf);
        return NULL;
    }
    return ffibuf;
}

int
sentry__get_kernel_version(windows_version_t *win_ver)
{
    void *ffibuf = sentry__try_file_version(L"ntoskrnl.exe");
    if (!ffibuf) {
        ffibuf = sentry__try_file_version(L"kernel32.dll");
    }
    if (!ffibuf) {
        return 0;
    }

    VS_FIXEDFILEINFO *ffi;
    UINT ffi_size;
    if (!VerQueryValueW(ffibuf, L"\\", (LPVOID *)&ffi, &ffi_size)) {
        sentry_free(ffibuf);
        return 0;
    }
    ffi->dwFileFlags &= ffi->dwFileFlagsMask;

    win_ver->major = ffi->dwFileVersionMS >> 16;
    win_ver->minor = ffi->dwFileVersionMS & 0xffff;
    win_ver->build = ffi->dwFileVersionLS >> 16;
    win_ver->ubr = ffi->dwFileVersionLS & 0xffff;

    sentry_free(ffibuf);

    return 1;
}

int
sentry__get_windows_version(windows_version_t *win_ver)
{
    // The `CurrentMajorVersionNumber`, `CurrentMinorVersionNumber` and `UBR`
    // are DWORD, while `CurrentBuild` is a SZ (text).
    uint32_t reg_version = 0;
    DWORD buf_size = sizeof(uint32_t);
    if (RegGetValueA(HKEY_LOCAL_MACHINE, CURRENT_VERSION,
            "CurrentMajorVersionNumber", RRF_RT_REG_DWORD, NULL, &reg_version,
            &buf_size)
        != ERROR_SUCCESS) {
        return 0;
    }
    win_ver->major = reg_version;

    buf_size = sizeof(uint32_t);
    if (RegGetValueA(HKEY_LOCAL_MACHINE, CURRENT_VERSION,
            "CurrentMinorVersionNumber", RRF_RT_REG_DWORD, NULL, &reg_version,
            &buf_size)
        != ERROR_SUCCESS) {
        return 0;
    }
    win_ver->minor = reg_version;

    char buf[32];
    buf_size = sizeof(buf);
    if (RegGetValueA(HKEY_LOCAL_MACHINE, CURRENT_VERSION, "CurrentBuild",
            RRF_RT_REG_SZ, NULL, buf, &buf_size)
        != ERROR_SUCCESS) {
        return 0;
    }
    win_ver->build = (uint32_t)sentry__strtod_c(buf, NULL);

    buf_size = sizeof(uint32_t);
    if (RegGetValueA(HKEY_LOCAL_MACHINE, CURRENT_VERSION, "UBR",
            RRF_RT_REG_DWORD, NULL, &reg_version, &buf_size)
        != ERROR_SUCCESS) {
        return 0;
    }
    win_ver->ubr = reg_version;

    return 1;
}

sentry_value_t
sentry__get_os_context(void)
{
    const sentry_value_t os = sentry_value_new_object();
    if (sentry_value_is_null(os)) {
        return os;
    }
    sentry_value_set_by_key(os, "name", sentry_value_new_string("Windows"));

    bool at_least_one_key_successful = false;
    char buf[32];
    windows_version_t win_ver;
    if (sentry__get_kernel_version(&win_ver)) {
        at_least_one_key_successful = true;

        snprintf(buf, sizeof(buf), "%u.%u.%u.%u", win_ver.major, win_ver.minor,
            win_ver.build, win_ver.ubr);
        sentry_value_set_by_key(
            os, "kernel_version", sentry_value_new_string(buf));
    }

    if (sentry__get_windows_version(&win_ver)) {
        at_least_one_key_successful = true;

        snprintf(buf, sizeof(buf), "%u.%u.%u", win_ver.major, win_ver.minor,
            win_ver.build);
        sentry_value_set_by_key(os, "version", sentry_value_new_string(buf));

        snprintf(buf, sizeof(buf), "%u", win_ver.ubr);
        sentry_value_set_by_key(os, "build", sentry_value_new_string(buf));
    }

    if (at_least_one_key_successful) {
        sentry_value_freeze(os);
        return os;
    }

    sentry_value_decref(os);
    return sentry_value_new_null();
}

static LPTOP_LEVEL_EXCEPTION_FILTER
logging_uef_setter(LPTOP_LEVEL_EXCEPTION_FILTER uef)
{
    (void)uef;
    SENTRY_WARN("Attempt to overwrite UnhandledExceptionFilter");
    sentry_value_t event = sentry_value_new_message_event(SENTRY_LEVEL_WARNING,
        NULL, "Attempt to overwrite UnhandledExceptionFilter");
    sentry_event_value_add_stacktrace(event, NULL, 0);
    sentry_capture_event(event);
    return NULL;
}

void
sentry__lock_unhandled_exception_filter()
{
    HMODULE h_kernel32 = GetModuleHandle("kernel32.dll");
    if (h_kernel32 == NULL) {
        return;
    }

    FARPROC set_uef_addr
        = GetProcAddress(h_kernel32, "SetUnhandledExceptionFilter");
    if (set_uef_addr == NULL) {
        return;
    }

#    if 0
    DWORD current_protection;
    if (!VirtualProtect((LPVOID)set_uef_addr, sizeof(LPVOID),
            PAGE_EXECUTE_READWRITE, &current_protection)) {
        return;
    }
#    endif

    // Calculate the jump instruction
#    if defined(_M_IX86)
    BYTE jump[6] = { 0x68, 0, 0, 0, 0, 0xC3 }; // push <address>; ret
    DWORD_PTR customFunction = (DWORD_PTR)logging_uef_setter;
    memcpy(&jump[1], &customFunction, sizeof(customFunction));
#    elif defined(_M_X64)
    BYTE jump[12] = { 0x48, 0xB8, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF,
        0xE0 }; // push <address>; ret
    uintptr_t customFunction = (uintptr_t)logging_uef_setter;
    memcpy(&jump[2], &customFunction, sizeof(customFunction));
#    elif defined(_M_ARM64)
    uint64_t customFunction = (uint64_t)CustomUnhandledExceptionFilter;
    const uint64_t pc_offset_correction = 8;
    uint64_t offset = custom_function - (uint64_t)original_fFunction - pc_offset_correction;
    uint32_t branch_instruction = 0x14000000 | ((offset >> 2) & 0x03FFFFFF));
    uint32_t nop_instruction =
    BYTE jump[16] = {0};
    memcpy(&jump[0], &customFunction, sizeof(customFunction));
    for (jump + sizeof(branch))
#    else
#        error "Platform not supported!"
#    endif

    // Write the jump instruction
#    if 0
    memcpy((LPVOID)set_uef_addr, jump, sizeof(jump));
#    else
    SIZE_T written;
    WriteProcessMemory(GetCurrentProcess(), (LPVOID)set_uef_addr, jump,
        sizeof(jump), &written);

    assert(written == sizeof(jump));
#    endif

#    if 0
    VirtualProtect((LPVOID)set_uef_addr, sizeof(void *), current_protection,
        &current_protection);
#    endif
}

void
sentry__reserve_thread_stack(void)
{
    const unsigned long expected_stack_size = 64 * 1024;
    unsigned long stack_size = 0;
    SetThreadStackGuarantee(&stack_size);
    if (stack_size < expected_stack_size) {
        stack_size = expected_stack_size;
        SetThreadStackGuarantee(&stack_size);
    }
}

#    if defined(SENTRY_BUILD_SHARED)

BOOL APIENTRY
DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    (void)hModule;
    (void)lpReserved;

    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
        sentry__reserve_thread_stack();
        break;
    default:
        return TRUE;
    }
    return TRUE;
}

#    endif // defined(SENTRY_BUILD_SHARED)

#elif defined(SENTRY_PLATFORM_MACOS)

#    include <sys/sysctl.h>
#    include <sys/utsname.h>

sentry_value_t
sentry__get_os_context(void)
{
    sentry_value_t os = sentry_value_new_object();
    if (sentry_value_is_null(os)) {
        return os;
    }

    sentry_value_set_by_key(os, "name", sentry_value_new_string("macOS"));

    char buf[32];
    size_t buf_len = sizeof(buf);

    if (sysctlbyname("kern.osproductversion", buf, &buf_len, NULL, 0) != 0) {
        goto fail;
    }

    size_t num_dots = 0;
    for (size_t i = 0; i < buf_len; i++) {
        if (buf[i] == '.') {
            num_dots += 1;
        }
    }
    if (num_dots < 2 && buf_len + 3 < sizeof(buf)) {
        strcat(buf, ".0");
    }

    sentry_value_set_by_key(os, "version", sentry_value_new_string(buf));

    buf_len = sizeof(buf);
    if (sysctlbyname("kern.osversion", buf, &buf_len, NULL, 0) != 0) {
        goto fail;
    }

    sentry_value_set_by_key(os, "build", sentry_value_new_string(buf));

    struct utsname uts;
    if (uname(&uts) != 0) {
        goto fail;
    }

    sentry_value_set_by_key(
        os, "kernel_version", sentry_value_new_string(uts.release));

    return os;

fail:

    sentry_value_decref(os);
    return sentry_value_new_null();
}
#elif defined(SENTRY_PLATFORM_UNIX)

#    include <sys/utsname.h>

sentry_value_t
sentry__get_os_context(void)
{
    sentry_value_t os = sentry_value_new_object();
    if (sentry_value_is_null(os)) {
        return os;
    }

    struct utsname uts;
    if (uname(&uts) != 0) {
        goto fail;
    }

    char *build = uts.release;
    size_t num_dots = 0;
    for (; build[0] != '\0'; build++) {
        char c = build[0];
        if (c == '.') {
            num_dots += 1;
        }
        if (!(c >= '0' && c <= '9') && (c != '.' || num_dots > 2)) {
            break;
        }
    }
    char *build_start = build;
    if (build[0] == '-' || build[0] == '.') {
        build_start++;
    }

    if (build_start[0] != '\0') {
        sentry_value_set_by_key(
            os, "build", sentry_value_new_string(build_start));
    }

    build[0] = '\0';

    sentry_value_set_by_key(os, "name", sentry_value_new_string(uts.sysname));
    sentry_value_set_by_key(
        os, "version", sentry_value_new_string(uts.release));

    return os;

fail:

    sentry_value_decref(os);
    return sentry_value_new_null();
}

#else

sentry_value_t
sentry__get_os_context(void)
{
    return sentry_value_new_null();
}

#endif
