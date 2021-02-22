#ifndef	_SERVICE_H_
#define	_SERVICE_H_

#define CONNECTING_STATE 0 
#define READING_STATE 1 
#define WRITING_STATE 2 
#define INSTANCES 1
#define PIPE_TIMEOUT 5000
#define BUFSIZE 4096

typedef struct 
{ 
    OVERLAPPED oOverlap; 
    HANDLE hPipeInst; 
    TCHAR chRequest[BUFSIZE]; 
    DWORD cbRead;
    TCHAR chReply[BUFSIZE];
    DWORD cbToWrite; 
    DWORD dwState; 
    BOOL fPendingIO; 
} PIPEINST, *LPPIPEINST; 
 
VOID closeAllHandle();
VOID DisconnectAndReconnect(DWORD); 
VOID GetAnswerToRequest(LPPIPEINST);
BOOL ConnectToNewClient(HANDLE, LPOVERLAPPED); 

#endif
