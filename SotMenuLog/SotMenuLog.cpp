#include "SotMenuLog.h"

// source for almost this entire file:
// https://docs.microsoft.com/en-us/windows/win32/ipc/named-pipe-server-using-completion-routines

#define TARGET_PROCESS_NAME L"UWP_Testbed_CPP.exe"

HANDLE hPipe;

int main()
{
    std::cout << "[UWP-IPC-Logger] Starting!" << std::endl;

    HANDLE hConnectEvent;
    OVERLAPPED oConnect;
    LPPIPEINST lpPipeInst;
    DWORD dwWait, cbRet;
    BOOL fSuccess, fPendingIO;

    // Create one event object for the connect operation. 
    hConnectEvent = CreateEvent(
        NULL,    // default security attribute
        TRUE,    // manual reset event 
        TRUE,    // initial state = signaled 
        NULL);   // unnamed event object 

    if (hConnectEvent == NULL)
    {
        std::cout << "CreateEvent failed with " << GetLastError() << std::endl;
        return 0;
    }

    oConnect.hEvent = hConnectEvent;

    // Call a subroutine to create one instance, and wait for 
    // the client to connect. 
    fPendingIO = CreateAndConnectInstance(&oConnect);

    while (1)
    {
        // Wait for a client to connect, or for a read or write 
        // operation to be completed, which causes a completion 
        // routine to be queued for execution. 
        dwWait = WaitForSingleObjectEx(
            hConnectEvent,  // event object to wait for 
            INFINITE,       // waits indefinitely 
            TRUE);          // alertable wait enabled 

        switch (dwWait)
        {
            // The wait conditions are satisfied by a completed connect 
            // operation. 
        case 0:
            // If an operation is pending, get the result of the 
            // connect operation. 
            if (fPendingIO)
            {
                fSuccess = GetOverlappedResult(
                    hPipe,     // pipe handle 
                    &oConnect, // OVERLAPPED structure 
                    &cbRet,    // bytes transferred 
                    FALSE);    // does not wait 
                if (!fSuccess)
                {
                    std::cout << "ConnectNamedPipe (" << GetLastError() << ")" << std::endl;
                    return 0;
                }
            }

            // Allocate storage for this instance. 
            lpPipeInst = (LPPIPEINST)GlobalAlloc(
                GPTR, sizeof(PIPEINST));
            if (lpPipeInst == NULL)
            {
                std::cout << "GlobalAlloc failed (" << GetLastError() << ")" << std::endl;
                return 0;
            }
            std::cout << "Allocated lpPipeInst" << std::endl;
            lpPipeInst->hPipeInst = hPipe;

            // Start the read operation for this client. 
            // Note that this same routine is later used as a 
            // completion routine after a write operation. 
            BeginRead((LPOVERLAPPED)lpPipeInst);

            // Create new pipe instance for the next client. 
            fPendingIO = CreateAndConnectInstance(
                &oConnect);
            break;

            // The wait is satisfied by a completed read or write 
            // operation. This allows the system to execute the 
            // completion routine.
        case WAIT_IO_COMPLETION:
            break;

            // An error occurred in the wait function.
        default:
        {
            std::cout << "WaitForSingleObjectEx (" << GetLastError() << ")" << std::endl;
            return 0;
        }
        }
    }

    std::cout << "Exiting!" << std::endl;
    CloseHandle(hPipe);
    std::cout << "Exited." << std::endl;
    return 0;
}

VOID WINAPI BeginRead(LPOVERLAPPED lpOverLap)
{
    LPPIPEINST lpPipeInst;
    BOOL fRead = FALSE;

    // lpOverlap points to storage for this instance. 
    lpPipeInst = (LPPIPEINST)lpOverLap;

    // The write operation has finished, so read the next request (if 
    // there is no error). 
    fRead = ReadFileEx(
        lpPipeInst->hPipeInst,
        lpPipeInst->buffer,
        BUFSIZE,
        (LPOVERLAPPED)lpPipeInst,
        (LPOVERLAPPED_COMPLETION_ROUTINE)CompletedReadRoutine);

    // Disconnect if an error occurred. 
    if (!fRead)
        DisconnectAndClose(lpPipeInst);
}

// CompletedReadRoutine(DWORD, DWORD, LPOVERLAPPED) 
// This routine is called as an I/O completion routine after reading 
// a request from the client. It gets data and writes it to the pipe. 
VOID WINAPI CompletedReadRoutine(DWORD dwErr, DWORD cbBytesRead,
    LPOVERLAPPED lpOverLap)
{
    LPPIPEINST lpPipeInst;
    // lpOverlap points to storage for this instance. 
    lpPipeInst = (LPPIPEINST)lpOverLap;

    // The read operation has finished, so print it (if no 
    // error occurred). 
    if ((dwErr == 0) && (cbBytesRead != 0))
    {
        std::cout << "[Pipe#" << (int)lpPipeInst->hPipeInst << "]: " << lpPipeInst->buffer << std::endl;
        BeginRead(lpOverLap);
    }
    else
    {
        // disconnect if an error occurred.
        DisconnectAndClose(lpPipeInst);
    }
}

// DisconnectAndClose(LPPIPEINST) 
// This routine is called when an error occurs or the client closes 
// its handle to the pipe. 
VOID DisconnectAndClose(LPPIPEINST lpPipeInst)
{
    std::cout << "Disconnecting client from pipe" << std::endl;
    // Disconnect the pipe instance. 
    if (!DisconnectNamedPipe(lpPipeInst->hPipeInst))
    {
        std::cout << "DisconnectNamedPipe failed with " << GetLastError() << std::endl;
    }

    // Close the handle to the pipe instance. 
    CloseHandle(lpPipeInst->hPipeInst);
    std::cout << "Closing handle to pipe" << std::endl;

    // Release the storage for the pipe instance. 
    if (lpPipeInst != NULL)
    {
        std::cout << "Freeing lpPipeInst" << std::endl;
        GlobalFree(lpPipeInst);
    }
}

// CreateAndConnectInstance(LPOVERLAPPED) 
// This function creates a pipe instance and connects to the client. 
// It returns TRUE if the connect operation is pending, and FALSE if 
// the connection has been completed. 
BOOL CreateAndConnectInstance(LPOVERLAPPED lpoOverlap)
{
    //LPCTSTR lpszPipename = TEXT("\\\\.\\pipe\\SotMenuLogPipe");
    std::cout << "Creating named pipe" << std::endl;
    DWORD pid = utils::GetPIDFromName(TARGET_PROCESS_NAME);
    std::cout << "Sea of Thieves PID: " << pid << "\n";
    DWORD errCode = 0;
    BOOL success = CreatePipeWithProperACL(L"SotMenuLogPipe", pid, &hPipe, &errCode);
    if ((success == FALSE) || (hPipe == INVALID_HANDLE_VALUE))
    {
        std::cout << "CreateNamedPipe failed with " << GetLastError() << std::endl;
        return 0;
    }
    std::cout << "Created named pipe#" << (int)hPipe << std::endl;
    // Call a subroutine to connect to the new client. 
    return ConnectToNewClient(hPipe, lpoOverlap);
}

BOOL ConnectToNewClient(HANDLE hPipe, LPOVERLAPPED lpo)
{
    BOOL fConnected, fPendingIO = FALSE;

    // Start an overlapped connection for this pipe instance. 
    fConnected = ConnectNamedPipe(hPipe, lpo);

    // Overlapped ConnectNamedPipe should return zero. 
    if (fConnected)
    {
        std::cout << "ConnectNamedPipe failed with " << GetLastError() << std::endl;
        return 0;
    }

    switch (GetLastError())
    {
        // The overlapped connection in progress. 
        case ERROR_IO_PENDING:
            fPendingIO = TRUE;
            break;

        // Client is already connected, so signal an event. 
        case ERROR_PIPE_CONNECTED:
            if (SetEvent(lpo->hEvent))
                break;

        // If an error occurs during the connect operation... 
        default:
        {
            std::cout << "ConnectNamedPipe failed with " << GetLastError() << std::endl;
            return 0;
        }
    }
    return fPendingIO;
}