#ifndef __INCLUDES_H__
#define __INCLUDES_H__
#ifdef UNICODE
#undef UNICODE
#endif
#ifdef _UNICODE
#undef _UNICODE
#endif

#include <Windows.h>
#include <tchar.h>
#include <ShlObj.h>
#include <string>
#include <iostream>
#include <iomanip>
#include <strsafe.h>
#include <Shlwapi.h>
namespace DeployDriver
{
	void ErrorExit(LPTSTR lpszFunction)
	{
		// Retrieve the system error message for the last-error code

		LPVOID lpMsgBuf;
		LPVOID lpDisplayBuf;
		DWORD dw = GetLastError();

		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dw,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0, NULL);
		// Display the error message and exit the process
		lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
			(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
		StringCchPrintf((LPTSTR)lpDisplayBuf,
			LocalSize(lpDisplayBuf) / sizeof(TCHAR),
			TEXT("%s failed with error %d: %s"),
			lpszFunction, dw, lpMsgBuf);
		MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

		LocalFree(lpMsgBuf);
		LocalFree(lpDisplayBuf);
		ExitProcess(dw);
	}

	using std::string;
	using std::cout;
	using std::cin;
	using std::endl;
	using std::setw;
	using std::setfill;
#ifndef ecout
#define ecout cout<<"[* ERROR *] "
#endif ecout
#ifndef icout
#define icout cout<<"[*] "
#endif icout
#ifndef iicout
#define iicout cout<<"\t[*] "
#endif iicout
#ifndef mbok
#define mbok(m) MessageBox(NULL,m,"",MB_OK)
#endif mbok
#ifndef freeMem
#define freeMem(m) if(m){delete m; m = 0;}
#endif
#ifndef freeMemA
#define freeMemA(m) if(m){delete [] m; m = 0;}
#endif
#ifndef freeHandle
#define freeHandle(h) if(h!=NULL){CloseHandle(h);h=NULL;}
#endif
}

#endif