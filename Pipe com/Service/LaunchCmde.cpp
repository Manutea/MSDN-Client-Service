#include <windows.h> 
#include <tchar.h>
#include <stdio.h> 
#include <strsafe.h>
#include <string>
#include "Ressources.h"
#include "LaunchCmde.h"

#define BUFSIZE 4096 

HANDLE g_hInputFile = NULL;
HANDLE g_hChildStd_IN_Rd = NULL;
HANDLE g_hChildStd_IN_Wr = NULL;
HANDLE g_hChildStd_OUT_Rd = NULL;
HANDLE g_hChildStd_OUT_Wr = NULL;
HANDLE g_hChildStd_ERR_Rd = NULL;
HANDLE g_hChildStd_ERR_Wr = NULL;

_Inout_opt_ LPWSTR readCommand(const TCHAR*, std::string);

bool launchCmde(const TCHAR* request, DWORD* acquittal, DWORD* errorLevel, DWORD* dwRead, CHAR* chBuf, DWORD* dwReadError, CHAR* chBufError)
{
    TCHAR *cmde = _T("C://Windows//System32//ping.exe");

    SECURITY_ATTRIBUTES saAttr;
    printf("\n->Start of parent execution.\n");

    // Set the bInheritHandle flag so pipe handles are inherited.  
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
    saAttr.bInheritHandle = TRUE; 
    saAttr.lpSecurityDescriptor = NULL; 	

    // Create a pipe for the child process's STDOUT.  
    if ( ! CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0) ) {
        ErrorDisplay(TEXT("StdoutRd CreatePipe"), dwRead, chBuf, acquittal); 	
		closeAllHandle();
		return false;
	}

    // Ensure the read handle to the pipe for STDOUT is not inherited.
    if ( ! SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0) ) {
        ErrorDisplay(TEXT("Stdout SetHandleInformation"), dwRead, chBuf, acquittal); 	
		closeAllHandle();
		return false;
	}
	
    // Create a pipe for the child process's STDERR.  
    if ( ! CreatePipe(&g_hChildStd_ERR_Rd, &g_hChildStd_ERR_Wr, &saAttr, 0) ) {
        ErrorDisplay(TEXT("StderrtRd CreatePipe"), dwRead, chBuf, acquittal); 	
		closeAllHandle(); 
		return false;
	}

    // Ensure the read handle to the pipe for STDERR is not inherited.
    if ( ! SetHandleInformation(g_hChildStd_ERR_Rd, HANDLE_FLAG_INHERIT, 0) ) {
        ErrorDisplay(TEXT("Stderr SetHandleInformation"), dwRead, chBuf, acquittal); 	
		closeAllHandle();
		return false;
	}

    // Create a pipe for the child process's STDIN.  
    if ( ! CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0)) {
        ErrorDisplay(TEXT("Stdin CreatePipe"), dwRead, chBuf, acquittal); 	
		closeAllHandle();
		return false;
	}

    // Ensure the write handle to the pipe for STDIN is not inherited. 
    if ( ! SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0) ) {
        ErrorDisplay(TEXT("Stdin SetHandleInformation"), dwRead, chBuf, acquittal); 	
		closeAllHandle();
		return false;
	}
 
    // Create the child process. 
    if( ! CreateChildProcess(request, acquittal, errorLevel)) {
		ErrorDisplay(TEXT("CreateProcess"), dwRead, chBuf, acquittal);	 	
		CloseHandle(g_hInputFile);
		CloseHandle(g_hChildStd_IN_Wr);
		CloseHandle(g_hChildStd_OUT_Rd);
		CloseHandle(g_hChildStd_ERR_Rd);
		return false;
	}

    // Get a handle to an input file for the parent. 
    // This example assumes a plain text file and uses string output to verify data flow.  
    g_hInputFile = CreateFile(
        cmde, 
        GENERIC_READ, 
        0, 
        NULL, 
        OPEN_EXISTING, 
        FILE_ATTRIBUTE_READONLY, 
        NULL); 

	DWORD d = GetLastError();

    if ( g_hInputFile == INVALID_HANDLE_VALUE ) {
        ErrorDisplay(TEXT("CreateFile"), dwRead, chBuf, acquittal);	
		CloseHandle(g_hInputFile);
		CloseHandle(g_hChildStd_IN_Wr);
		CloseHandle(g_hChildStd_OUT_Rd);
		CloseHandle(g_hChildStd_ERR_Rd);
		return false;
	}

    // Write to the pipe that is the standard input for a child process. 
    // Data is written to the pipe's buffers, so it is not necessary to wait
    // until the child process is running before writing data. 
    printf( "\n->Contents of %S written to child STDIN pipe.\n", cmde);
    if( ! WriteToPipe()) {
        ErrorDisplay(TEXT("StdInWr CloseHandle"), dwRead, chBuf, acquittal); 		
		CloseHandle(g_hInputFile);	
		CloseHandle(g_hChildStd_OUT_Rd);
		CloseHandle(g_hChildStd_ERR_Rd);
		return false;
	}


    // Read from pipe that is the standard output for child process. 
    printf( "\n->Contents of child process STDOUT:\n\n");	
    ReadFromPipe(dwRead, chBuf); 
 
    // Read from pipe that is the standard error for child process. 
    printf( "\n->Contents of child process STDERR:\n\n");
	ReadFromErrPipe(dwReadError, chBufError);

    printf("\n->End of parent execution.\n");

    // The remaining open handles are cleaned up when this process terminates. 
    // To avoid resource leaks in a larger application, close handles explicitly.
	CloseHandle(g_hInputFile);
	CloseHandle(g_hChildStd_OUT_Rd);
	CloseHandle(g_hChildStd_ERR_Rd);
	
	return true;
}
 
_Inout_opt_ LPWSTR readCommand(const TCHAR* request, std::string command) {
	int index = 1;
	while(request[index] != '\0') {
		command += (char)request[index];
		index++;
	} 
	command += '\0';
	wchar_t* wtext = new wchar_t[BUFSIZE];
	size_t outSize;
	mbstowcs_s(&outSize, wtext, command.length(), command.c_str(), command.length()-1);
	return wtext;
}

// Create a child process that uses the previously created pipes for STDIN and STDOUT.
bool CreateChildProcess(const TCHAR* request, DWORD* acquittal, DWORD* errorLevel)
{ 
    PROCESS_INFORMATION piProcInfo; 
    STARTUPINFO siStartInfo;
    BOOL bSuccess = FALSE; 
 
    // Set up members of the PROCESS_INFORMATION structure.  
    ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );
 
    // Set up members of the STARTUPINFO structure. 
    // This structure specifies the STDIN and STDOUT handles for redirection. 
    ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
    siStartInfo.cb = sizeof(STARTUPINFO); 
    siStartInfo.hStdError = g_hChildStd_ERR_Wr;
    siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
    siStartInfo.hStdInput = g_hChildStd_IN_Rd;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	_In_opt_ LPCWSTR lpApplicationName;
	_Inout_opt_ LPWSTR lpCommandLine;

	if(request[0] == COM_DISKPART) {
		lpApplicationName = _T("C:\\Windows\\System32\\diskpart.exe");
		lpCommandLine = readCommand(request, "diskpart.exe ");
	}

	else if(request[0] == COM_STORCLI) {
		lpApplicationName = _T("C:\\Windows\\System32\\storcli.exe");
		lpCommandLine = readCommand(request, "storcli.exe ");
	}
	
	else if(request[0] == COM_ARCCONF) {
		lpApplicationName = _T("C:\\Windows\\System32\\arcconf.exe");
		lpCommandLine = readCommand(request, "arcconf.exe ");
	}

	else if(request[0] == '4') {
		lpApplicationName = _T("C:\\Windows\\System32\\ping.exe");
		lpCommandLine = readCommand(request, "ping.exe ");
	}
	
	else if(request[0] == '5') {
		lpApplicationName = _T("C:\\Windows\\System32\\eventcreate.exe");
		lpCommandLine = readCommand(request, "eventcreate ");
	}

	else {
		*acquittal = ACQUI_COM_ERR;
		return false;
	}

    // Create the child process.     
    bSuccess = CreateProcess(lpApplicationName, 
        lpCommandLine,     // command line 
        NULL,          // process security attributes 
        NULL,          // primary thread security attributes 
        TRUE,          // handles are inherited 
        0,             // creation flags 
        NULL,          // use parent's environment 
        NULL,          // use parent's current directory 
        &siStartInfo,  // STARTUPINFO pointer 
        &piProcInfo);  // receives PROCESS_INFORMATION 
   
    // If an error occurs, exit the application. 
    if ( ! bSuccess ) {
		*acquittal = ACQUI_SYN_ERR;
        return false;
	}
    else 
    {
		*acquittal = ACQUI_OK;
		WaitForSingleObject( piProcInfo.hProcess, INFINITE );
		int result = GetExitCodeProcess(piProcInfo.hProcess, errorLevel);

        // Close handles to the child process and its primary thread.
        // Some applications might keep these handles to monitor the status
        // of the child process, for example. 
        CloseHandle(piProcInfo.hProcess);
        CloseHandle(piProcInfo.hThread);
        
        // Close handles to the stdin and stdout pipes no longer needed by the child process.
        // If they are not explicitly closed, there is no way to recognize that the child process has ended.
        CloseHandle(g_hChildStd_OUT_Wr);     
        CloseHandle(g_hChildStd_ERR_Wr);
        CloseHandle(g_hChildStd_IN_Rd);
    }

	return true;
}
 
// Read from a file and write its contents to the pipe for the child's STDIN.
// Stop when there is no more data. 
bool WriteToPipe(void) 
{ 
    DWORD dwRead, dwWritten; 
    CHAR chBuf[BUFSIZE];
    BOOL bSuccess = FALSE;
 
    for (;;) 
    { 
        bSuccess = ReadFile(g_hInputFile, chBuf, BUFSIZE, &dwRead, NULL);
        if ( ! bSuccess || dwRead == 0 ) break; 
      
        bSuccess = WriteFile(g_hChildStd_IN_Wr, chBuf, dwRead, &dwWritten, NULL);
        if ( ! bSuccess ) break; 
    } 
 
    // Close the pipe handle so the child process stops reading.  
    if ( ! CloseHandle(g_hChildStd_IN_Wr) ) 
		return false;
	
	return true;
}  

// Read output from the child process's pipe for STDOUT
// and write to the parent process's pipe for STDOUT. 
// Stop when there is no more data. 
void ReadFromPipe(DWORD *dwRead, CHAR *chBuf) 
{ 
    DWORD dwWritten, dwTmp;
    BOOL bSuccess = FALSE;
    HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	bool continu = true;
    while (continu) 
    { 
        bSuccess = ReadFile( g_hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwTmp, NULL);
        if(dwTmp > 0)
            *dwRead = dwTmp;
        if( ! bSuccess || dwTmp == 0 ) 
            break;     
        bSuccess = WriteFile(hParentStdOut, chBuf, dwTmp, &dwWritten, NULL);
        if (! bSuccess ) 
            break; 
    }
} 
 
// Read output from the child process's pipe for STDERR
// and write to the parent process's pipe for STDERR. 
// Stop when there is no more data. 
void ReadFromErrPipe(DWORD *dwReadError, CHAR *chBufError) 
{ 
    DWORD dwWrittenError, dwTmpError;
    BOOL bSuccess = FALSE;
    HANDLE hParentStdErr = GetStdHandle(STD_ERROR_HANDLE);

	bool continu = true;
	while (continu) 
    { 
        bSuccess = ReadFile( g_hChildStd_ERR_Rd, chBufError, BUFSIZE, &dwTmpError, NULL);
        if(dwTmpError > 0)
            *dwReadError = dwTmpError;
        if( ! bSuccess || dwTmpError == 0 ) 
            break;     
        bSuccess = WriteFile(hParentStdErr, chBufError, dwTmpError, &dwWrittenError, NULL);
        if (! bSuccess ) 
            break; 
    }
} 

// Format a readable error message, display a message box, 
// and exit from the application.
void ErrorDisplay(PTSTR lpszFunction, DWORD * dwRead, CHAR * chBuf, DWORD * acquittal) 
{ 
    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError(); 

	DWORD test = 1;
	dwRead = &test;
	chBuf = "0";

	*acquittal = ACQUI_SER_ERR;

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
        (lstrlen((LPCTSTR)lpMsgBuf)+lstrlen((LPCTSTR)lpszFunction)+40)*sizeof(TCHAR)); 
    StringCchPrintf((LPTSTR)lpDisplayBuf, 
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s failed with error %d: %s"), 
        lpszFunction, dw, lpMsgBuf); 
    //MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
}

void closeAllHandle() {	 	
	CloseHandle(g_hInputFile);
	CloseHandle(g_hChildStd_IN_Rd);
	CloseHandle(g_hChildStd_IN_Wr);
	CloseHandle(g_hChildStd_OUT_Rd);
	CloseHandle(g_hChildStd_OUT_Wr);
	CloseHandle(g_hChildStd_ERR_Rd);
	CloseHandle(g_hChildStd_ERR_Wr);
}
