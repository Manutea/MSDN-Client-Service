#include <windows.h> 
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include "Client.h"
#include "../Service/Ressources.h"

#define BUFSIZE 512


//ping 192.9.202.209.30
int _tmain(int argc, TCHAR *argv[]) 
{ 
	if(argc < 3)
		return -1;

	TCHAR cmdToSend[BUFSIZE];

	if(false == buildRequest(cmdToSend, argc, argv))
		return -1;

	HANDLE hPipe; 
	LPTSTR lpvMessage = cmdToSend;

	TCHAR  chBuf[BUFSIZE]; 
	BOOL   fSuccess = FALSE; 
	DWORD  cbRead, cbToWrite, cbWritten, dwMode; 
	LPTSTR lpszPipename = TEXT("\\\\.\\pipe\\cortexApplyCmd"); 

	// Try to open a named pipe; wait for it, if necessary. 
	bool continu = true;
	while (continu) 
	{ 
		hPipe = CreateFile( 
		lpszPipename,   // pipe name 
		GENERIC_READ |  // read and write access 
		GENERIC_WRITE, 
		0,              // no sharing 
		NULL,           // default security attributes
		OPEN_EXISTING,  // opens existing pipe 
		0,              // default attributes 
		NULL);          // no template file 
 
		// Break if the pipe handle is valid. 
 
		if (hPipe != INVALID_HANDLE_VALUE) 
        break; 
 
		// Exit if an error other than ERROR_PIPE_BUSY occurs. 
 
		if (GetLastError() != ERROR_PIPE_BUSY) 
		{
			#if DEBUG
				_tprintf( TEXT("Could not open pipe. GLE=%d\n"), GetLastError() ); 
			#endif
			return -1;
		}
 
		// All pipe instances are busy, so wait for 20 seconds. 
 
		if ( ! WaitNamedPipe(lpszPipename, 20000)) 
		{ 
			#if DEBUG
				printf("Could not open pipe: 20 second wait timed out."); 
			#endif
			return -1;
		} 
	} 
 
	// The pipe connected; change to message-read mode. 
 
	dwMode = PIPE_READMODE_MESSAGE; 
	fSuccess = SetNamedPipeHandleState( 
		hPipe,    // pipe handle 
		&dwMode,  // new pipe mode 
		NULL,     // don't set maximum bytes 
		NULL);    // don't set maximum time 
	if ( ! fSuccess) 
	{
		#if DEBUG
			_tprintf( TEXT("SetNamedPipeHandleState failed. GLE=%d\n"), GetLastError() ); 
		#endif
		return -1;
	}
 
	// Send a message to the pipe server. 
 
	cbToWrite = (lstrlen(lpvMessage)+1)*sizeof(TCHAR);
	#if DEBUG
		_tprintf( TEXT("Sending %d byte message: \"%s\"\n"), cbToWrite, lpvMessage); 
	#endif 

	fSuccess = WriteFile( 
		hPipe,                  // pipe handle 
		lpvMessage,             // message 
		cbToWrite,              // message length 
		&cbWritten,             // bytes written 
		NULL);                  // not overlapped 

	if ( ! fSuccess) 
	{
		#if DEBUG
			_tprintf( TEXT("WriteFile to pipe failed. GLE=%d\n"), GetLastError() ); 
		#endif 
		return -1;
	}
   
	#if DEBUG
		printf("\nMessage sent to server, receiving reply as follows:\n"); 
	#endif
 
	do 
	{ 
	// Read from the pipe.  
		fSuccess = ReadFile( 
			hPipe,    // pipe handle 
			chBuf,    // buffer to receive reply 
			BUFSIZE*sizeof(TCHAR),  // size of buffer 
			&cbRead,  // number of bytes read 
			NULL);    // not overlapped 
 
		if ( ! fSuccess && GetLastError() != ERROR_MORE_DATA )
			break; 
    
		#if DEBUG
			_tprintf( TEXT("\"%s\"\n"), chBuf );  
		#else
			if(chBuf[ERR_INDEX] == '0')
				_tprintf(chBuf);
			else
				_tprintf(chBuf);

		#endif
	} while ( ! fSuccess);  // repeat loop if ERROR_MORE_DATA 

	if ( ! fSuccess)
	{    
		#if DEBUG
			_tprintf( TEXT("ReadFile from pipe failed. GLE=%d\n"), GetLastError() );
		#endif
		return -1;
	}
      
	#if DEBUG
		printf("\n<End of message, press ENTER to terminate connection and exit>");
	#endif
		_getch();
 
	CloseHandle(hPipe);  
	return 0; 
}

bool buildRequest(TCHAR* cmdToSend, int argc, TCHAR *argv[]) {

	TCHAR ping[] = _T("ping");
	TCHAR arconf[] = _T("arcconf");
	TCHAR storcli[] = _T("storcli");
	TCHAR diskpart[] = _T("diskpart");
	TCHAR eventcreate[] = _T("eventcreate");

	TCHAR szName[20];
	wcscpy_s(szName, argv[1]);

	DWORD cmdNumer;
	if(_tcsicmp (ping, szName) == 0)
		cmdNumer = COM_PING;
	else if(_tcsicmp (arconf, szName) == 0)
		cmdNumer = COM_ARCCONF;
	else if(_tcsicmp (storcli, szName) == 0)
		cmdNumer = COM_STORCLI;
	else if(_tcsicmp (diskpart, szName) == 0)
		cmdNumer = COM_DISKPART;	
	else if(_tcsicmp (eventcreate, szName) == 0)
		cmdNumer = COM_EVEN;
	else
		return false;

	for(int i = 0; i < BUFSIZE; i++){
		cmdToSend[i] = _TEXT('');
	}
	
	cmdToSend[0] = (TCHAR)((int)cmdNumer + '0');
	cmdToSend[1] = ' ';

	int index = 0;
	for(auto i = 2; i < argc; i++) {	
		while(argv[i][index++] != '\0'){
			if(index >= BUFSIZE)
				return false;
			cmdToSend[index+1] = argv[i][index-1];
		};	
	}

	return true;
}