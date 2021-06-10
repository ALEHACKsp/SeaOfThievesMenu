#include <iostream>
#include <Windows.h>
#include "comm.h"

int main()
{
    char buffer[1024]{};
    DWORD dwRead;

    const DWORD pid = GetProcessIdByName("SoTGame.exe");

    std::cout << "Sea of Thieves PID: " << pid << "\n";

    HANDLE logPipe;
    if (CreatePipeFromPid(&logPipe, L"SotMenuLogPipe", pid) != NOERROR) {
        std::cout << "Something went wrong";
    }

    while (logPipe != INVALID_HANDLE_VALUE)
    {
        if (ConnectNamedPipe(logPipe, NULL) != FALSE) 
        {
            while (ReadFile(logPipe, buffer, sizeof(buffer) - 1, &dwRead, NULL) != FALSE)
            {
                buffer[dwRead] = '\0';
                printf("%s", buffer);
            }
        }
        DisconnectNamedPipe(logPipe);
    }

    return 0;
}