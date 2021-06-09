#include <iostream>
#include <Windows.h>

int main()
{
    char buffer[1024]{};
    DWORD dwRead;

    HANDLE logPipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\SotMenuLogPipe"),
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        1,
        1024 * 16,
        1024 * 16,
        NMPWAIT_USE_DEFAULT_WAIT,
        NULL);

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
