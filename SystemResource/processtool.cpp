#include "stdafx.h"
#include "processtool.h"
#include "atltime.h"





CProcessTool::CProcessTool()
{
	m_pQueryBuf = NULL;
	NtQuerySystemInformation = NULL;
	NtTerminateProcess = NULL;
	WinStationGetProcessSid = NULL;
	CachedGetUserFromSid = NULL;
	hWinstaMod = NULL;
	hUtidllMod = NULL;

	m_QueryBufSize = 0;
}


CProcessTool::~CProcessTool()
{
	if(m_pQueryBuf)
	{
		VirtualFree(m_pQueryBuf,m_QueryBufSize,MEM_DECOMMIT);
	}
	if(hWinstaMod)
	{
		FreeLibrary(hWinstaMod);
	}
	if(hUtidllMod)
	{
		FreeLibrary(hUtidllMod);
	}
}

BOOL CProcessTool::Create()
{
	HMODULE hNtdllMod;
	m_QueryBufSize = 2 *1024 *1024;
	m_pQueryBuf = (LPBYTE)VirtualAlloc(NULL,m_QueryBufSize,
		MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
	if(m_pQueryBuf==NULL)
		return FALSE;

	hNtdllMod = GetModuleHandle(TEXT("ntdll.dll"));
	NtQuerySystemInformation = (PfnNtQuerySystemInformation)GetProcAddress(
		hNtdllMod,TEXT("NtQuerySystemInformation"));
	NtTerminateProcess = (PfnNtTerminateProcess)GetProcAddress(
		hNtdllMod,TEXT("NtTerminateProcess"));


	if(NtQuerySystemInformation==NULL)
	{
		LOG::printError(TEXT("GetProceAddress:NtQuerySystemInformation"));
		return FALSE;
	}

	hWinstaMod = LoadLibrary(_T("Winsta.dll"));
	if(hWinstaMod!=NULL)
	{
		WinStationGetProcessSid = (PfnWinStationGetProcessSid)GetProcAddress(
			hWinstaMod,"WinStationGetProcessSid");
	}
	else
	{
		LOG::printError(TEXT("LoadLibrary(Winsta.dll)"));
	}

	hUtidllMod = LoadLibrary(_T("utildll.dll"));
	if(hUtidllMod!=NULL)
	{
		CachedGetUserFromSid = (PfnCachedGetUserFromSid)GetProcAddress(
			hUtidllMod,"CachedGetUserFromSid");
	}
	else
	{
		LOG::printError(TEXT("LoadLibrary(utildll.dll"));
	}

	return TRUE;
}

BOOL CProcessTool::CreateSnapshotProcess()
{
	NTSTATUS status;
	ULONG ulSize = 0;

	do 
	{
		status = NtQuerySystemInformation(SystemProcessesAndThreadsInformation,
			m_pQueryBuf,m_QueryBufSize,&ulSize);
		if(status==STATUS_INFO_LENGTH_MISMATCH)
		{
			VirtualFree(m_pQueryBuf,m_QueryBufSize,MEM_DECOMMIT);
			m_QueryBufSize += m_QueryBufSize;
			VirtualAlloc(m_pQueryBuf,m_QueryBufSize,MEM_COMMIT|MEM_RESERVE,
				PAGE_READWRITE);
			if(m_pQueryBuf==NULL)return FALSE;
		}
	} while (status == STATUS_INFO_LENGTH_MISMATCH);

	return TRUE;
}



BOOL CProcessTool::ProcessFirst(PSYSTEM_PROCESS_INFORMATION *offset)
{
	*offset = (PSYSTEM_PROCESS_INFORMATION)m_pQueryBuf;
	return TRUE;
}

BOOL CProcessTool::ProcessNext(PSYSTEM_PROCESS_INFORMATION *offset)
{
	if((*offset)->NextEntryOffset==0)
		return FALSE;
	*offset = (PSYSTEM_PROCESS_INFORMATION)((LPBYTE)(*offset)+(*offset)->NextEntryOffset);
	return TRUE;
}


BOOL CProcessTool::CheckProcess(PSYSTEM_PROCESS_INFORMATION pSPI)
{
	MAPPROCESSMARK::iterator iter = m_Mark.find(pSPI->UniqueProcessId);
	if(iter !=m_Mark.end() )
	{
		iter->second = TRUE;
		return TRUE;
	}
	else
	{
		m_Mark.insert(PROCESSPAIR(pSPI->UniqueProcessId,TRUE));
	}
		

}


void CProcessTool::SetCurrentTotalProcessCpuTime(PSYSTEM_PROCESS_INFORMATION offset)
{
	m_TotalCpuTime.QuadPart=0;
	while(TRUE)
	{
		m_TotalCpuTime.QuadPart += offset->KernelTime.QuadPart+offset->UserTime.QuadPart;
		if(offset->NextEntryOffset==0)break;
		offset = (PSYSTEM_PROCESS_INFORMATION)((LPBYTE)offset+offset->NextEntryOffset);
	}
}

void CProcessTool::SetLastTotalProcessCpuTime()
{
	m_LastTotalCpuTime = m_TotalCpuTime;
}


void CProcessTool::SetLastProcessCpuTimeList(PSYSTEM_PROCESS_INFORMATION offset)
{
	MAPPROCESSCPUTIME::iterator iter;
	while(TRUE)
	{
		iter = m_LastProcessCpuTimeList.find(offset->UniqueProcessId);
		if(iter==m_LastProcessCpuTimeList.end())
		{
			m_LastProcessCpuTimeList.insert(MAPPROCESSCPUTIMEPAIR(offset->UniqueProcessId,
				(offset->KernelTime.QuadPart+offset->UserTime.QuadPart)));
		}
		else
		{
			iter->second = (offset->KernelTime.QuadPart+offset->UserTime.QuadPart);
		}
		if(offset->NextEntryOffset==0)break;

		offset = (PSYSTEM_PROCESS_INFORMATION)((LPBYTE)offset+offset->NextEntryOffset);
	}
}


UINT CProcessTool::GetSingleProcessCpuTime(PSYSTEM_PROCESS_INFORMATION offset)
{
	MAPPROCESSCPUTIME::iterator iter;
	double iCpuUsage;

	iter = m_LastProcessCpuTimeList.find(offset->UniqueProcessId);
	if(iter==m_LastProcessCpuTimeList.end())
		return 0;
	LARGE_INTEGER curProcessTime;
	curProcessTime.QuadPart = offset->KernelTime.QuadPart+offset->UserTime.QuadPart;
	curProcessTime.QuadPart -= iter->second;
	
	iCpuUsage = (double)curProcessTime.QuadPart/(m_TotalCpuTime.QuadPart-m_LastTotalCpuTime.QuadPart)*100;
	return (UINT)iCpuUsage;

}

BOOL CProcessTool::GetProcessExtendInfo(PSYSTEM_PROCESS_INFORMATION pSPI,PPROCESS_EXTEND_INFO pExtendInfo)
{
	HANDLE hProcess;
	TCHAR szBuf[2048] ={0};

	memset(pExtendInfo,0,sizeof(PPROCESS_EXTEND_INFO));

	hProcess = OpenProcess(PROCESS_ALL_ACCESS,FALSE,pSPI->UniqueProcessId);
	if(hProcess==NULL)return FALSE;

	//Get Exe Path,Description,Company Name
	if(GetModuleFileNameEx(hProcess,NULL,szBuf,MAX_PATH)!=0)
	{
		pExtendInfo->strExeFilePath = szBuf;
		GetFileDescription(szBuf,pExtendInfo->strDescription,
			pExtendInfo->strCompanyName);
	}

	//Get Virtual Memory Size
	PROCESS_MEMORY_COUNTERS pmc;
	if(GetProcessMemoryInfo(hProcess,&pmc,sizeof(PROCESS_MEMORY_COUNTERS)))
	{
		pExtendInfo->dwVirtualMemorySize = pmc.PagefileUsage;
	}


	//Get Parent Process File Path
	HANDLE hParent =NULL;
	hParent = OpenProcess(PROCESS_VM_READ|PROCESS_QUERY_INFORMATION,
		FALSE,pSPI->InheritedFromUniqueProcessId);
	if(hParent!=NULL)
	{
		memset(szBuf,0,sizeof(szBuf));
		if(GetModuleFileNameEx(hParent,NULL,szBuf,MAX_PATH)!=0)
		{
			pExtendInfo->strParentFilePath = szBuf;
		}
	}

	//Get Username
	if(WinStationGetProcessSid&&CachedGetUserFromSid)
	{
		wchar_t wcsUserName[100]={0};
		char mbsUserName[100]={0};
		size_t NumOfCharConverted;

		DWORD dwSize = sizeof(szBuf)/sizeof(TCHAR);
		FILETIME ftStartTime;
		CopyMemory(&ftStartTime,&pSPI->CreateTime,sizeof(FILETIME));
		if(WinStationGetProcessSid(NULL,pSPI->UniqueProcessId,ftStartTime,
			(LPBYTE)szBuf,&dwSize))
		{
			CachedGetUserFromSid(szBuf,wcsUserName,&dwSize);
#ifdef _UNICODE
			pExtendInfo->strUserName = wcsUserName;
#else
			wcstombs_s(&NumOfCharConverted,mbsUserName,wcsUserName,
				sizeof(mbsUserName));
			pExtendInfo->strUserName = mbsUserName;
#endif
		}

	}
	//Get Start time
	{
		SYSTEMTIME st;
		ZeroMemory(&st,sizeof(SYSTEMTIME));
		FileTimeToSystemTime((FILETIME *)&pSPI->CreateTime,&st);
		CTime Time(st);

		Time +=CTimeSpan(0,8,0,0);
		pExtendInfo->strStartTime = Time.Format(TEXT("%Y-%m-%d %H:%M:%S"));
	}
	


	return TRUE;
}

tstring CProcessTool::GetProcessPriorityClass(HANDLE hProcess)
{
	return NULL;
}

void CProcessTool::GetFileDescription(tstring FileName,tstring &strDescription,tstring &strCompanyName)
{
	BOOL bSucceed = FALSE;
	DWORD dwInfoSize = 0;
	DWORD dwHandle = 0;
	dwInfoSize = sizeof(VS_FIXEDFILEINFO);
	strDescription = _T("");

	dwInfoSize = GetFileVersionInfoSize(FileName.c_str(),&dwHandle);

	if(dwInfoSize >0)
	{
		void *pvInfo = new TCHAR[dwInfoSize];
		if(GetFileVersionInfo(FileName.c_str(),0,dwInfoSize,pvInfo))
		{
			struct LANGANDCODEPAGE{
				WORD wLanguage;
				WORD wCodePage;
			}*lpTranslate;

			UINT cbTranslate = 0;

			if(VerQueryValue(pvInfo,_T("\\VarFileInfo\\Translation"),
				(void**)&lpTranslate,&cbTranslate))
			{
				if(cbTranslate/sizeof(struct LANGANDCODEPAGE)>0){
					const TCHAR *lpBuffer = 0;
					UINT cbSizeBuf = 0;
					TCHAR szSubBlock[50];

					_stprintf_s(szSubBlock,sizeof(szSubBlock),_T("\\StringFileInfo\\%04x%04x\\FileDescription"),
						lpTranslate[0].wLanguage,lpTranslate[0].wCodePage);
					if(VerQueryValue(pvInfo,szSubBlock,(void **)&lpBuffer,&cbSizeBuf))
					{
						strDescription.append(lpBuffer,cbSizeBuf);
					}

					memset(szSubBlock,0,sizeof(szSubBlock));
					_stprintf_s(szSubBlock,50,_T("\\StringFileInfo\\%04x%04x\\CompanyName"),
						lpTranslate[0].wLanguage,lpTranslate[0].wCodePage);
					if(VerQueryValue(pvInfo,szSubBlock,(void **)&lpBuffer,&cbSizeBuf))
					{
						strCompanyName.append(lpBuffer,cbSizeBuf);
					}


				}
			}
		}
	}

}


vector<DWORD> CProcessTool::GetProcessId(TCHAR szProcessName[])
{

	HANDLE hProcessSnap;
	PROCESSENTRY32 pe32;
	VCTPID vctPid;
	tstring strExeFile;
	tstring strProcessName;

	strProcessName = szProcessName;
	transform(strProcessName.begin(),strProcessName.end(),
		strProcessName.begin(),_totlower);
	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
	if(hProcessSnap == INVALID_HANDLE_VALUE)
	{
		LOG::printError(TEXT("CreateToolHelp32Snapshot (of process)"));
		return vctPid;
	}

	pe32.dwSize = sizeof(PROCESSENTRY32);

	if(!Process32First(hProcessSnap,&pe32))
	{
		LOG::printError(TEXT("Process32First"));
	}
	else
	{
		do 
		{
			strExeFile = pe32.szExeFile;
			transform(strExeFile.begin(),strExeFile.end(),strExeFile.begin(),tolower);
			if(_tcscmp(strProcessName.c_str(),strExeFile.c_str())==0)
			{
				vctPid.push_back(pe32.th32ProcessID);
			}
		} while (Process32Next(hProcessSnap,&pe32));
	}

	CloseHandle(hProcessSnap);
	return vctPid;

}

vector<MODULEENTRY32> CProcessTool::GetProcessModules(DWORD dwPID)
{
	HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
	MODULEENTRY32 me32;
	VCTMOD vctMod;

	hModuleSnap =  CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,dwPID);

	memset(&me32,0,sizeof(me32));
	if( hModuleSnap == INVALID_HANDLE_VALUE)
	{
		LOG::printError(TEXT("CreateToolhelp32Snapshot (od modules)"));
		return vctMod;
	}


	me32.dwSize = sizeof(MODULEENTRY32);

	if(!Module32First(hModuleSnap,&me32))
	{
		LOG::printError(TEXT("Module32First"));
		CloseHandle(hModuleSnap);
		return vctMod;
	}

	do 
	{
		vctMod.push_back(me32);

	} while (Module32Next(hModuleSnap,&me32));

	CloseHandle(hModuleSnap);
	return vctMod;

}

vector<THREADENTRY32> CProcessTool::GetProcessThreads(DWORD dwOwnerPID)
{
	HANDLE hThreadSnap = INVALID_HANDLE_VALUE;
	THREADENTRY32 te32;
	VCTTHREAD vctThread;

	memset(&te32,0,sizeof(te32));
	hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);
	if(hThreadSnap == INVALID_HANDLE_VALUE)
		return vctThread;

	te32.dwSize = sizeof(THREADENTRY32);

	if(!Thread32First(hThreadSnap,&te32))
	{
		LOG::printError(TEXT("Thread32First"));
		CloseHandle(hThreadSnap);
		return vctThread;
	}


	do 
	{
		if(te32.th32OwnerProcessID == dwOwnerPID)
		{
			vctThread.push_back(te32);
		}
	} while (Thread32Next(hThreadSnap,&te32));

	CloseHandle(hThreadSnap);
	return vctThread;

}

UInt64 CProcessTool::GetProcessMemoryUsage(DWORD dwPID)
{
	PROCESS_MEMORY_COUNTERS pmc;
	HANDLE hProcess;
	BOOL reply;
	BOOL succeed = FALSE;

	hProcess = OpenProcess(PROCESS_ALL_ACCESS|PROCESS_VM_READ,FALSE,dwPID);
	if(hProcess == NULL)
	{
		LOG::printError(TEXT("OpenProcess"));
	}	
	else
	{
		memset(&pmc,0,sizeof(pmc));

		reply = GetProcessMemoryInfo(hProcess,&pmc,sizeof(pmc));
		if(!reply)
			LOG::printError(TEXT("GetProcessMemoryInfo"));
		else
			succeed = TRUE;

	}
	CloseHandle(hProcess);
	if(succeed)
		return pmc.WorkingSetSize/1024;
	else
		return 0;

}

__int32 CProcessTool::GetProcessCPUUsage(DWORD dwPID)
{

	HANDLE hProcess;
	HANDLE hEvent;
	__int64 nCreateionTime = 0;
	__int64 nExitTime = 0;
	__int64 nKernelTimeOld = 0,nKernelTimeNew = 0;
	__int64 nUserTimeOld = 0,nUserTimeNew = 0;
	__int64 nTickCountOld = 0,nTickCountNew = 0;
	__int32 nCpuUsagePercent = 0;
	__int32 nProcessorCount = 0;
	SYSTEM_INFO systemInfo;

	GetSystemInfo(&systemInfo);
	nProcessorCount = systemInfo.dwNumberOfProcessors;


	hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
	hProcess = OpenProcess(PROCESS_ALL_ACCESS,FALSE,dwPID);
	if(hProcess == NULL)
		LOG::printError(TEXT("OpenProcess"));
	else
	{
		nTickCountOld = GetTickCount();

		if(!GetProcessTimes(hProcess,(LPFILETIME)&nCreateionTime,(LPFILETIME)&nExitTime,
			(LPFILETIME)&nKernelTimeOld,(LPFILETIME)&nUserTimeOld))
			return 0;

		WaitForSingleObject(hEvent,20);
		nTickCountNew = GetTickCount();
		if(!GetProcessTimes(hProcess,(LPFILETIME)&nCreateionTime,(LPFILETIME)&nExitTime,
			(LPFILETIME)&nKernelTimeNew,(LPFILETIME)&nUserTimeNew))
			return 0;

		nCpuUsagePercent = (__int32)((nKernelTimeNew-nKernelTimeOld+nUserTimeNew-
			nUserTimeOld)/(100 * (nProcessorCount * (nTickCountNew - nTickCountOld))));
		CloseHandle(hEvent);
		CloseHandle(hProcess);
		return nCpuUsagePercent;
	}

	CloseHandle(hEvent);
	CloseHandle(hProcess);

}


BOOL CProcessTool::TerminateProcess(DWORD dwPid)
{
	BOOL bSucceed = FALSE;
	if(NtTerminateProcess)
	{
		HANDLE hProcess;

		hProcess = OpenProcess(PROCESS_ALL_ACCESS,FALSE,dwPid);
		if(hProcess)
		{
			 if(NtTerminateProcess(hProcess,1)==0)
			 {
				 bSucceed = TRUE;
			 }
			 else
			 {
				 LOG::printError(TEXT("NtTerminateProcess"));
			 }
		}
	}

	return bSucceed;
}