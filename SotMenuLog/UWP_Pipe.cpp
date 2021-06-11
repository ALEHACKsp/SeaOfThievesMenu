#include "UWP_Pipe.h"

inline ULONG BOOL_TO_ERROR(BOOL f)
{
    return f ? NOERROR : GetLastError();
}

volatile UCHAR guz = 0;
BOOL CreatePipeforUWP(PCWSTR PipeName, ULONG dwProcessId, OUT PHANDLE PipeHandle, OUT DWORD* errCode)
{
    if (HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, 0, dwProcessId))
    {
        SECURITY_ATTRIBUTES sa = { sizeof(sa), 0, FALSE };

        // SDDL_EVERYONE + SDDL_ALL_APP_PACKAGES + SDDL_ML_LOW
        // original: S:(ML;;;;;LW)
        // replacement: S:(ML;; NW;;; LW)
        if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
            L"D:(A;;GA;;;WD)(A;;GA;;;AC)S:(ML;;NW;;;LW)",
            SDDL_REVISION_1, &sa.lpSecurityDescriptor, 0))
        {
            *errCode = GetLastError();
            return FALSE;
        }

        BOOL success = FALSE;
        HANDLE hToken;

        ULONG err = BOOL_TO_ERROR(OpenProcessToken(hProcess, TOKEN_QUERY, &hToken));

        if (err == NOERROR)
        {
            PVOID stack = alloca(guz);

            ULONG cb = 0, rcb = 128;

            union {
                PVOID buf;
                PTOKEN_APPCONTAINER_INFORMATION AppConainer;
                PWSTR sz;
            };

            do
            {
                if (cb < rcb)
                {
                    cb = RtlPointerToOffset(buf = alloca(rcb - cb), stack);
                }

                err = BOOL_TO_ERROR(GetTokenInformation(hToken, ::TokenAppContainerSid, buf, cb, &rcb));

                if (err == NOERROR)
                {
                    ULONG SessionId;

                    err = BOOL_TO_ERROR(GetTokenInformation(hToken, ::TokenSessionId, &SessionId, sizeof(SessionId), &rcb));

                    if (err == NOERROR)
                    {
                        PWSTR szSid;

                        err = BOOL_TO_ERROR(ConvertSidToStringSid(AppConainer->TokenAppContainer, &szSid));

                        if (err == NOERROR)
                        {
                            static const WCHAR fmt[] = L"\\\\?\\pipe\\Sessions\\%d\\AppContainerNamedObjects\\%s\\%s";

                            rcb = (1 + _scwprintf(fmt, SessionId, szSid, PipeName)) * sizeof(WCHAR);

                            if (cb < rcb)
                            {
                                cb = RtlPointerToOffset(buf = alloca(rcb - cb), stack);
                            }

                            _swprintf(sz, fmt, SessionId, szSid, PipeName);

                            HANDLE hPipe = CreateNamedPipeW(sz,
                                PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
                                PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                                PIPE_UNLIMITED_INSTANCES,
                                4096,
                                4096,
                                5000,
                                &sa);

                            if (hPipe == INVALID_HANDLE_VALUE)
                            {
                                err = GetLastError();
                            }
                            else
                            {
                                *PipeHandle = hPipe;
                                success = TRUE;
                            }

                            LocalFree(szSid);
                        }
                    }
                    break;
                }
            } while (err == ERROR_INSUFFICIENT_BUFFER);

            CloseHandle(hToken);
        }
        LocalFree(sa.lpSecurityDescriptor);
        *errCode = err;
        CloseHandle(hProcess);
        return success;
    }
    *errCode = GetLastError();
    return FALSE;
}