#include "WDK.h"

typedef struct _PROCESS_EXTEND_INFO
{
	tstring strDescription;
	tstring strCompanyName;
	tstring strExeFilePath;
	tstring strUserName;
	tstring strStartTime;
	tstring strPrority;
	tstring strParentFilePath;
	DWORD  dwVirtualMemorySize;
	
}PROCESS_EXTEND_INFO,*PPROCESS_EXTEND_INFO;


class CProcessTool
{
private:
	typedef DWORD (WINAPI *PfnNtQuerySystemInformation)(ULONG,PVOID,ULONG,PULONG);
	typedef DWORD (__stdcall *PfnNtTerminateProcess)(HANDLE,UINT);
	typedef BYTE (WINAPI *PfnWinStationGetProcessSid)(HANDLE,DWORD,FILETIME,PBYTE,PDWORD);
	typedef VOID (WINAPI *PfnCachedGetUserFromSid)(PSID,PWCHAR,PULONG);

	typedef map<DWORD,BOOL> MAPPROCESSMARK;
	typedef pair<DWORD,BOOL> PROCESSPAIR;
	typedef map<DWORD,LONGLONG> MAPPROCESSCPUTIME;
	typedef pair<DWORD,LONGLONG> MAPPROCESSCPUTIMEPAIR;

public:
	CProcessTool();
	virtual ~CProcessTool();

public:
	BOOL Create();
	BOOL CreateSnapshotProcess();
	BOOL ProcessFirst(PSYSTEM_PROCESS_INFORMATION *offset);
	BOOL ProcessNext(PSYSTEM_PROCESS_INFORMATION *offset);
	BOOL CheckProcess(PSYSTEM_PROCESS_INFORMATION pSPI);
	
public:
	void SetCurrentTotalProcessCpuTime(PSYSTEM_PROCESS_INFORMATION offset);
	void SetLastTotalProcessCpuTime();
	void SetLastProcessCpuTimeList(PSYSTEM_PROCESS_INFORMATION offset);
	UINT GetSingleProcessCpuTime(PSYSTEM_PROCESS_INFORMATION offset);
	BOOL GetProcessExtendInfo(PSYSTEM_PROCESS_INFORMATION pSPI,PPROCESS_EXTEND_INFO pExtendInfo);
	tstring GetProcessPriorityClass(HANDLE hProcess);
	void GetFileDescription(tstring FileName,tstring &strDescription,tstring &strCompanyName);
	BOOL TerminateProcess(DWORD dwPid);
public:
	vector<DWORD> GetProcessId(TCHAR szProcessName[]);
	vector<MODULEENTRY32> GetProcessModules(DWORD dwPID);
	vector<THREADENTRY32> GetProcessThreads(DWORD dwOwnerPID);
	UInt64 GetProcessMemoryUsage(DWORD dwPID);
	__int32 GetProcessCPUUsage(DWORD dwPID);

private:
	PfnNtQuerySystemInformation NtQuerySystemInformation;
	PfnWinStationGetProcessSid WinStationGetProcessSid;
	PfnCachedGetUserFromSid   CachedGetUserFromSid;
	PfnNtTerminateProcess    NtTerminateProcess;
	LPBYTE m_pQueryBuf;
	ULONG m_QueryBufSize;
	MAPPROCESSMARK m_Mark;
	LARGE_INTEGER m_TotalCpuTime;
	LARGE_INTEGER m_LastTotalCpuTime;
	MAPPROCESSCPUTIME m_LastProcessCpuTimeList;
	HMODULE hWinstaMod;
	HMODULE hUtidllMod;

};