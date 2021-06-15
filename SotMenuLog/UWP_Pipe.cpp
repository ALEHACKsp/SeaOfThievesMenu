#include "UWP_Pipe.h"

inline ULONG BOOL_TO_ERROR(BOOL f)
{
    return f ? NOERROR : GetLastError();
}

// credit: RbMm of StackOverflow [https://stackoverflow.com/a/60452660]
// changes: removed use of alloca
BOOL CreatePipeWithProperACL(PCWSTR PipeName, ULONG dwProcessId, OUT PHANDLE PipeHandle, OUT DWORD* errCode)
{
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, dwProcessId);
    if (hProcess == NULL)
    {
        *errCode = GetLastError();
        std::cout << "Could not open process (errCode: " << *errCode << ")" << std::endl;
        return FALSE;
    }
    /*---------------------------
    / 1. Get the security descriptor string that we'll apply to the named pipe, so that the appcontainer target process can open it
    //---------------------------
    */
    // NOTE:
    // setting lpSecurityDescriptor to NULL will cause it to be assigned the default security descriptor associated with the access token of the calling process
    // don't know if we have to LocalFree it (probably not?)
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, FALSE };

    // SDDL_EVERYONE + SDDL_ALL_APP_PACKAGES + SDDL_ML_LOW
    // original: S:(ML;;;;;LW)
    // replacement: S:(ML;; NW;;; LW)
    // NOTE:
    // msdn says the security descriptor field of SECURITY_ATTRIBUTES is "self-referencing" and LocalFree must be called on it after we're done
    // weird but okay
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
        L"D:(A;;GA;;;WD)(A;;GA;;;AC)S:(ML;;NW;;;LW)",
        SDDL_REVISION_1, &sa.lpSecurityDescriptor, 0))
    {
        *errCode = GetLastError();
        CloseHandle(hProcess);
        return FALSE;
    }

    /*---------------------------
    / 2. Get target process access token
    //---------------------------
    */
    HANDLE hToken;
    *errCode = BOOL_TO_ERROR(OpenProcessToken(hProcess, TOKEN_QUERY, &hToken));
    if (*errCode != NOERROR)
    {
        std::cout << "Could not open process token (errCode: " << *errCode << ")" << std::endl;
        LocalFree(sa.lpSecurityDescriptor);
        CloseHandle(hProcess);
        return FALSE;
    }
    /*---------------------------
    / 3. Get AppContainerSid via GetTokenInformation (not sure how important the resize logic is since TOKEN_APPCONTAINER_INFORMATION is just a pointer..?)
    /    Maybe microsoft writes the buffer pointed at by pointer to (pointer+sizeof(pointer), so we have to size it to hold both microsoft's buffer and the pointer
    /    Just a guess, don't care enough to actually check
    //---------------------------
    */
    DWORD sizeAppContainerInfoBuf = sizeof(TOKEN_APPCONTAINER_INFORMATION);
    DWORD sizeNeededAppContainerInfoBuf = 0;
    PTOKEN_APPCONTAINER_INFORMATION appContainerInfoBuf = (PTOKEN_APPCONTAINER_INFORMATION)VirtualAlloc(NULL, sizeAppContainerInfoBuf, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    *errCode = BOOL_TO_ERROR(GetTokenInformation(hToken, ::TokenAppContainerSid, appContainerInfoBuf, sizeAppContainerInfoBuf, &sizeNeededAppContainerInfoBuf));
    while (*errCode == ERROR_INSUFFICIENT_BUFFER)
    {
        std::cout << "appContainerInfo buffer was not large enough, resizing to " << sizeNeededAppContainerInfoBuf << " bytes and trying again..." << std::endl;
        VirtualFree(appContainerInfoBuf, 0, MEM_RELEASE);
        sizeAppContainerInfoBuf = sizeNeededAppContainerInfoBuf;
        appContainerInfoBuf = (PTOKEN_APPCONTAINER_INFORMATION)VirtualAlloc(NULL, sizeAppContainerInfoBuf, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        *errCode = BOOL_TO_ERROR(GetTokenInformation(hToken, TOKEN_INFORMATION_CLASS::TokenAppContainerSid, appContainerInfoBuf, sizeAppContainerInfoBuf, &sizeNeededAppContainerInfoBuf));
    }
    if (*errCode != NOERROR)
    {
        std::cout << "Could not get TOKEN_APPCONTAINER_INFORMATION (errCode: " << *errCode << ")" << std::endl;
        VirtualFree(appContainerInfoBuf, 0, MEM_RELEASE);
        LocalFree(sa.lpSecurityDescriptor);
        CloseHandle(hToken);
        CloseHandle(hProcess);
        return FALSE;
    }
    std::cout << "appContainerInfoBuf should now contain TokenAppContainerSid" << std::endl;

    /*---------------------------
    / 4. Get session ID via GetTokenInformation (always a DWORD)
    //---------------------------
    */
    DWORD sessionID = 0;
    DWORD sessionIDSizeNeeded = 0;
    // this always returns a DWORD so i don't see any point in checking sessionIDSizeNeeded and resizing
    *errCode = BOOL_TO_ERROR(GetTokenInformation(hToken, TOKEN_INFORMATION_CLASS::TokenSessionId, &sessionID, sizeof(sessionID), &sessionIDSizeNeeded));
    if (*errCode != NOERROR)
    {
        std::cout << "Could not get TokenSessionId (errCode: " << *errCode << ")" << std::endl;
        VirtualFree(appContainerInfoBuf, 0, MEM_RELEASE);
        LocalFree(sa.lpSecurityDescriptor);
        CloseHandle(hToken);
        CloseHandle(hProcess);
        return FALSE;
    }
    /*---------------------------
    / 5. Convert AppContainerSid to string then construct object namespace string for pipe name
    //---------------------------
    */
    PTSTR szSid;
    // NOTE:
    // places a pointer to null-terminated SID string in szSid
    // msdn says LocalFree must be called on szSid when we're done
    *errCode = BOOL_TO_ERROR(ConvertSidToStringSid(appContainerInfoBuf->TokenAppContainer, &szSid));
    if (*errCode != NOERROR)
    {
        std::cout << "Could not convert AppContainerSid to string (errCode: " << *errCode << ")" << std::endl;
        VirtualFree(appContainerInfoBuf, 0, MEM_RELEASE);
        LocalFree(sa.lpSecurityDescriptor);
        CloseHandle(hToken);
        CloseHandle(hProcess);
        return FALSE;
    }
    static const WCHAR fmt[] = L"\\\\?\\pipe\\Sessions\\%d\\AppContainerNamedObjects\\%s\\%s";
    // note: _scwprintf returns the NUMBER of characters, minus the null-terminated character.
    int sizeOfNameString = (_scwprintf(fmt, sessionID, szSid, PipeName) + 1);
    WCHAR* pipeNameStr = new WCHAR[sizeOfNameString];
    swprintf_s(pipeNameStr, sizeOfNameString * 2, fmt, sessionID, szSid, PipeName);

    /*---------------------------
    / 6. Finally, create the named pipe in the proper object namespace so that the appcontainer target process can see it
    //---------------------------
    */
    *PipeHandle = CreateNamedPipeW(pipeNameStr,
        PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        4096,
        4096,
        5000,
        &sa);

    BOOL retVal = TRUE;
    if (*PipeHandle == INVALID_HANDLE_VALUE)
    {
        *errCode = GetLastError();
        std::cout << "Could not create named pipe (errCode: " << *errCode << ")" << std::endl;
        retVal = FALSE;
    }
    else
    {
        std::cout << "Successfully created named pipe!" << std::endl;
    }
    delete[] pipeNameStr;
    VirtualFree(appContainerInfoBuf, 0, MEM_RELEASE);
    LocalFree(szSid);
    LocalFree(sa.lpSecurityDescriptor);
    CloseHandle(hToken);
    CloseHandle(hProcess);
    return retVal;
}