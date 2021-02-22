#ifndef	_LAUNCHCMDE_H_
#define	_LAUNCHCMDE_H_

bool WriteToPipe(void); 
void closeAllHandle(void);
void ReadFromPipe(DWORD *, CHAR *); 
void ReadFromErrPipe(DWORD *, CHAR *);
void ErrorDisplay(PTSTR, DWORD*, CHAR*, DWORD*);
bool CreateChildProcess(const TCHAR*, DWORD*, DWORD*); 
bool launchCmde(const TCHAR*, DWORD*, DWORD*, DWORD*, CHAR*, DWORD*, CHAR*);

#endif
