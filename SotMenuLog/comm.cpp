#include "comm.h"

ULONG CreatePipeFromPid(OUT PHANDLE PipeHandle, PCWSTR PipeName, ULONG dwProcessId) {
    if (HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, 0, dwProcessId)) {
        ULONG err = CreatePipeFromProcess(PipeHandle, PipeName, hProcess);
        CloseHandle(hProcess);
        return err;
    }
    return GetLastError();
}

DWORD GetProcessIdByName(const char* name) {
    PVOID snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 process;
    process.dwSize = sizeof(process);

    DWORD pid = 0;
    while (Process32Next(snapshot, &process)) {
        _bstr_t b(process.szExeFile);
        const char* c = b;

        if (strcmp(c, name) == 0) {
            pid = process.th32ProcessID;
            break;
        }
    }

    CloseHandle(snapshot);
    return pid;
}

// https://stackoverflow.com/questions/60431348/ipc-uwp-c-sharp-pipe-client-fails-on-connect-c-server
volatile UCHAR guz = 0;

ULONG CreatePipeFromProcess(OUT PHANDLE PipeHandle, PCWSTR PipeName, HANDLE hProcess) {

    SECURITY_ATTRIBUTES sa = { sizeof(sa), 0, FALSE };

    // SDDL_EVERYONE + SDDL_ALL_APP_PACKAGES + SDDL_ML_LOW
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
        L"D:(A;;GA;;;WD)(A;;GA;;;AC)S:(ML;;;;;LW)",
        SDDL_REVISION_1, &sa.lpSecurityDescriptor, 0)) {
        return GetLastError();
    }

    HANDLE hToken;

    ULONG err = BOOL_TO_ERROR(OpenProcessToken(hProcess, TOKEN_QUERY, &hToken));

    if (err != NOERROR) {
        LocalFree(sa.lpSecurityDescriptor);
        return err;
    }

    PVOID stack = malloc(guz);

    ULONG cb = 0, rcb = 128;

    union {
        PVOID buf;
        PTOKEN_APPCONTAINER_INFORMATION AppConainer;
        PWSTR sz;
    };

    do {
        if (cb < rcb) {
            cb = RtlPointerToOffset(buf = malloc(rcb - cb), stack);
        }

        err = BOOL_TO_ERROR(GetTokenInformation(hToken, ::TokenAppContainerSid, buf, cb, &rcb));

       /* if (err != NOERROR) {
            continue;
        }*/

        ULONG SessionId;

        err = BOOL_TO_ERROR(GetTokenInformation(hToken, ::TokenSessionId, &SessionId, sizeof(SessionId), &rcb));

        if (err == NOERROR) {
            PWSTR szSid;

            if (AppConainer == nullptr) {
                continue;
            }

            err = BOOL_TO_ERROR(ConvertSidToStringSid(AppConainer->TokenAppContainer, &szSid));

            if (err == NOERROR) {
                static const WCHAR fmt[] = L"\\\\?\\pipe\\Sessions\\%d\\AppContainerNamedObjects\\%s\\%s";
                rcb = (1 + _scwprintf(fmt, SessionId, szSid, PipeName)) * sizeof(WCHAR);

                if (cb < rcb)
                {
                    cb = RtlPointerToOffset(buf = malloc(rcb - cb), stack);
                }

                std::cout << "SoT pipe location: " << fmt << "\n";

                HANDLE hPipe = CreateNamedPipeW(fmt,
                    PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                    PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                    PIPE_UNLIMITED_INSTANCES, 1024 * 16, 1024 * 16, 0, &sa);

                if (hPipe == INVALID_HANDLE_VALUE) {
                    err = GetLastError();
                }
                else {
                    *PipeHandle = hPipe;
                }
                LocalFree(szSid);
            }
        }

        break;

    } while (err == ERROR_INSUFFICIENT_BUFFER);

    CloseHandle(hToken);

    LocalFree(sa.lpSecurityDescriptor);

    return err;
}