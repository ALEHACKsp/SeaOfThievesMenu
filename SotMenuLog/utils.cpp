#include "utils.h"


DWORD utils::GetPIDFromName(const wchar_t* name)
{
    DWORD pid = -1;

    while (pid == -1)
    {
        HANDLE procSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (procSnapshot == INVALID_HANDLE_VALUE)
        {
            std::cout << "Could not create snapshot (" << GetLastError() << ")" << std::endl;
        }

        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(procSnapshot, &pe32) == FALSE)
        {
            CloseHandle(procSnapshot);
            std::cout << "Process32First failed for snapshot (" << GetLastError() << ")" << std::endl;
        }
        do
        {
            if (_wcsicmp(pe32.szExeFile, name) == 0)
            {
                // found game, get pid and exit loop
                pid = pe32.th32ProcessID;
                break;
            }
        } while (Process32Next(procSnapshot, &pe32));

        CloseHandle(procSnapshot);
        Sleep(300);
    }
    return pid;
}