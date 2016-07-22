#include "stdafx.h"
#include "SystemResource.h"
#include <io.h>
#include "Aclapi.h"




#define _USEXML_
#define DIV 1024
#define WIDTH 7

CSystemResource::CSystemResource()
{
	 
	m_pLzmaCom = NULL;
	m_pLzmaUncom = NULL;
	m_hLzmaMod = NULL;
	Initialize();
}


BOOL CSystemResource::Initialize()
{

	m_ProcessTool.Create();
	EnablePrivilege(SE_CHANGE_NOTIFY_NAME,TRUE);
	EnablePrivilege(SE_SECURITY_NAME,TRUE);
	EnablePrivilege(SE_BACKUP_NAME,TRUE);
	EnablePrivilege(SE_DEBUG_NAME,TRUE);
	EnablePrivilege(SE_IMPERSONATE_NAME,TRUE);
	EnablePrivilege(SE_CREATE_GLOBAL_NAME,TRUE);

	m_hLzmaMod = LoadLibrary(TEXT("lzma.dll"));
	if(m_hLzmaMod)
	{
		m_pLzmaCom = (lzmaCompress)GetProcAddress(m_hLzmaMod,"LzmaCompress");
		m_pLzmaUncom = (lzmaUncompress)GetProcAddress(m_hLzmaMod,"LzmaUncompress");
	}
	else
	{
		LOG::printError(_T("LoadLibrary(lzma.dll)"));
		return FALSE;
	}
	return TRUE;
}
CSystemResource::~CSystemResource()
{
	if(m_hLzmaMod)
	{
		FreeLibrary(m_hLzmaMod);
	}
}

BOOL CSystemResource::EnablePrivilege(PCTSTR szPrivilege, BOOL fEnable)
{

	HANDLE hToken;
	LUID Luid;
	TOKEN_PRIVILEGES tkp;

	if ( ! OpenProcessToken( GetCurrentProcess(),
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken ) )
		return false;


	if ( ! LookupPrivilegeValue( NULL, szPrivilege, &Luid ) )
	{
		CloseHandle( hToken );
		return false;
	}


	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Luid = Luid;
	tkp.Privileges[0].Attributes = fEnable?SE_PRIVILEGE_ENABLED:0;

	if ( ! AdjustTokenPrivileges( hToken, FALSE, &tkp, sizeof(tkp), NULL, NULL ) )
		CloseHandle( hToken );

	return true;
}

MEMORYSTATUSEX CSystemResource::GetTotalMemoryStatus()
{
	MEMORYSTATUSEX statex;

	statex.dwLength = sizeof(statex);

	GlobalMemoryStatusEx(&statex);

	return statex;
}

__int64 CSystemResource::CompareFileTime ( FILETIME time1, FILETIME time2 )
{
	__int64 a = time1.dwHighDateTime << 32 | time1.dwLowDateTime ;
	__int64 b = time2.dwHighDateTime << 32 | time2.dwLowDateTime ;

	return   (b - a);
}

UINT CSystemResource::GetTotalCpuStatus()
{
	HANDLE hEvent;
	BOOL res;

	FILETIME preidleTime;
	FILETIME prekernelTime;
	FILETIME preuserTime;


	FILETIME idelTime;
	FILETIME kernelTime;
	FILETIME userTime;
	__int64 idle;
	__int64 kernel;
	__int64 user;
	UINT cpu;
	UINT cpuidle;


	res = GetSystemTimes(&idelTime,&kernelTime,&userTime);

	preidleTime = idelTime;
	prekernelTime = kernelTime;
	preuserTime = userTime;

	hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);

	WaitForSingleObject(hEvent,20);
	res = GetSystemTimes(&idelTime,&kernelTime,&userTime);

	idle = CompareFileTime(preidleTime,idelTime);
	kernel = CompareFileTime(prekernelTime,kernelTime);
	user = CompareFileTime(preuserTime,userTime);

	cpu =  (UINT)((kernel +user - idle) *100/(kernel+user));

	return cpu;


}



BOOL CSystemResource::GetProcessList2()
{
	TCHAR pName[1000]={0};
	size_t returnValue;
	PSYSTEM_PROCESS_INFORMATION offset;
	pugi::xml_node process,module,thread;
	PROCESS_EXTEND_INFO extendInfo;



	if(!(m_ProcessTool.CreateSnapshotProcess())||!(m_ProcessTool.ProcessFirst(&offset)))
		return FALSE;
	m_ProcessTool.SetCurrentTotalProcessCpuTime(offset);
	m_ProcessTool.SetLastTotalProcessCpuTime();
	m_ProcessTool.SetLastProcessCpuTimeList(offset);
	



	Sleep(20);
	if(!(m_ProcessTool.CreateSnapshotProcess())||!(m_ProcessTool.ProcessFirst(&offset)))
		return FALSE;
	m_ProcessTool.SetCurrentTotalProcessCpuTime(offset);
	Json::Value item;
	Json::Value arrayObj;
	do 
	{

		if(offset->ImageName.Length==0&&offset->UniqueProcessId==0)
			offset->ImageName.Buffer = L"System Idle Process";
#ifdef _UNICODE

		pName = offset->ImageName.Buffer;
#else		
		if(offset->ImageName.Buffer!=NULL)
		{
			wcstombs_s(&returnValue,pName,
				(size_t)sizeof(pName),offset->ImageName.Buffer,(size_t)sizeof(pName));
		}
#endif
#ifdef _USEXML_		
		process = m_XMLSystem.append_child(_T("process"));
		process.append_attribute(_T("name")) = pName;
		process.append_attribute(_T("pid")) = (UINT)offset->UniqueProcessId;
		process.append_attribute(_T("cpuusage")) = (UINT)m_ProcessTool.GetSingleProcessCpuTime(offset);
		process.append_attribute(_T("memoryusage")) = (UInt64)offset->WorkingSetSize/1024;
		process.append_attribute(_T("threadcnt")) = (UINT)offset->NumberOfThreads;
		process.append_attribute(_T("iowritecnt")) = (UInt64)offset->WriteOperationCount.QuadPart;
		process.append_attribute(_T("ioreadcnt")) = (UInt64)offset->ReadOperationCount.QuadPart;
#else
		item[TEXT("name")] = pName;
		item[TEXT("pid")] = (UINT)offset->UniqueProcessId;
		item[TEXT("cpuusage")] = (UINT)m_ProcessTool.GetSingleProcessCpuTime(offset);
		item[TEXT("memoryusage")] = (UInt64)offset->WorkingSetSize/1024;
		item[TEXT("threadcnt")] = (UINT)offset->NumberOfThreads;
		item[TEXT("iowritecnt")] = (UInt64)offset->WriteOperationCount.QuadPart;
		item[TEXT("ioreadcnt")] = (UInt64)offset->ReadOperationCount.QuadPart;
#endif
		if(m_ProcessTool.GetProcessExtendInfo(offset,&extendInfo))
		{
			if(extendInfo.strUserName.size())
			{
#ifdef _USEXML_
				process.append_attribute(_T("user")) = extendInfo.strUserName.c_str();
#else
				item[TEXT("user")] = extendInfo.strUserName.c_str();
#endif
				
				
			}
			if(extendInfo.strExeFilePath.size())
			{
#ifdef _USEXML_
				process.append_attribute(_T("path")) = extendInfo.strExeFilePath.c_str();
#else
				item[_T("path")] = extendInfo.strExeFilePath.c_str();
#endif
								
			}
			if(extendInfo.strDescription.size())
			{
#ifdef _USEXML_
				process.append_attribute(_T("description")) = extendInfo.strDescription.c_str();
#else
				item[TEXT("description")] = extendInfo.strDescription.c_str();
#endif
				
				
			}
			if(extendInfo.strCompanyName.size())
			{
#ifdef _USEXML_
				process.append_attribute(_T("companyname")) = extendInfo.strCompanyName.c_str();
#else
				item[TEXT("companyname")] = extendInfo.strCompanyName.c_str();
#endif
				
				
			}
			
			memset(&extendInfo,0,sizeof(PROCESS_EXTEND_INFO));
		}
#ifndef _USEXML_
		arrayObj.append(item);
#endif
		
	} while (m_ProcessTool.ProcessNext(&offset));
#ifndef _USEXML_
		m_JsonSystem[TEXT("process")] = arrayObj;
#endif
	

}

BOOL CSystemResource::GetSystemResource()
{
	SYSTEM_INFO systemInfo;
	pugi::xml_node autostart,root;
	tstring systembit,systemName;
	VCTAUTOSTARTINFO vctAutoStartInfo;
	LOG::show(TEXT("GetSystemResource::Start"));

	
	SafeGetNativeSystem(&systemInfo);
	m_nProcessorCount = systemInfo.dwNumberOfProcessors;
	
	if(systemInfo.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64||
		systemInfo.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_IA64)
	{
		systembit = "64";
	}
	else
	{
		systembit = "32";
	}




	MEMORYSTATUSEX statex;
	statex =  GetTotalMemoryStatus();

#ifdef _USEXML_	
	root = m_XMLDoc.append_child(_T("root"));
	m_XMLSystem = root.append_child(_T("system"));
	m_XMLSystem.append_attribute(_T("version")) = GetOSName().c_str();
	m_XMLSystem.append_attribute(_T("osbit")) = systembit.c_str();
	m_XMLSystem.append_attribute(_T("cpuusage")) = GetTotalCpuStatus();

	m_XMLSystem.append_attribute(_T("memoryusagepercent")) = (UINT)statex.dwMemoryLoad;
	m_XMLSystem.append_attribute(_T("totalphys")) = statex.ullTotalPhys/1024;
	m_XMLSystem.append_attribute(_T("availphys")) = statex.ullAvailPhys/1024;
	m_XMLSystem.append_attribute(_T("totalpagefile")) = statex.ullTotalPageFile/1024;
	m_XMLSystem.append_attribute(_T("availpagefile")) = statex.ullAvailPageFile/1024;
	m_XMLSystem.append_attribute(_T("totalvirtual")) = statex.ullTotalVirtual/1024;
	m_XMLSystem.append_attribute(_T("availvirtual")) = statex.ullAvailVirtual/1024;
#else

	m_JsonSystem[TEXT("version")] = GetOSName().c_str();
	m_JsonSystem[TEXT("osbit")] = systembit.c_str();
	m_JsonSystem[TEXT("cpuusage")] = GetTotalCpuStatus();
	m_JsonSystem[TEXT("memoryusagepercent")] = (UINT)statex.dwMemoryLoad;
	m_JsonSystem[TEXT("totalphys")] = statex.ullTotalPhys/1024;
	m_JsonSystem[TEXT("availphys")] = statex.ullAvailPhys/1024;
	m_JsonSystem[TEXT("totalpagefile")] = statex.ullTotalPageFile/1024;
	m_JsonSystem[TEXT("availpagefile")] = statex.ullAvailPageFile/1024;
	m_JsonSystem[TEXT("totalvirtual")] = statex.ullTotalVirtual/1024;
	m_JsonSystem[TEXT("availvirtual")] = statex.ullAvailVirtual/1024;
#endif	
	m_AutoRunsTool.GetRegRun(vctAutoStartInfo);
	m_AutoRunsTool.GetServicesDrives();
	MAPKEYVALUEDATA::iterator iter;
	Json::Value item;
	Json::Value arrayObj;
	for(int i =0;i<vctAutoStartInfo.size();i++)
	{
		for(iter=vctAutoStartInfo[i].mapKeyValueData.begin();
			iter!=vctAutoStartInfo[i].mapKeyValueData.end();
			++iter)
		{
#ifdef _USEXML_
			autostart = m_XMLSystem.append_child(TEXT("autostart"));
			autostart.append_attribute(TEXT("name")) = iter->first.c_str();
			autostart.append_attribute(TEXT("type"))= TEXT("logon");
			autostart.append_attribute(TEXT("filepath")) = iter->second.c_str();
			autostart.append_attribute(TEXT("regpath")) = vctAutoStartInfo[i].strRegPath.c_str();
#else
			item["name"] = iter->first.c_str();
			item["type"] = TEXT("logon");
			item["filepath"] = iter->second.c_str();
			item["regpath"] = vctAutoStartInfo[i].strRegPath.c_str();
			arrayObj.append(item);
#endif
		}
	}
#ifndef _USEXML_
		m_JsonSystem["autostart"] = arrayObj;
#endif
	
	return GetProcessList2();

}
BOOL CSystemResource::SaveXmlFile(tstring strFilePath)
{
	BOOL bSucceed = FALSE;
	LOG::show("SaveXmlFile::Start");
	if(strFilePath.size())
	{
		tstring strTemp;
		__int32 iPos;

		iPos = strFilePath.find("\\");
		if(iPos!=tstring::npos)
		{
			strTemp = strFilePath.substr(0,iPos);

			if(_access(strTemp.c_str(),0)==0)
			{
				LOG::show("SaveXmlFile::Filepath(%s) existed",strFilePath.c_str());
				bSucceed = m_XMLDoc.save_file(strFilePath.c_str());
			}
		}
		
	}
	if(bSucceed)
	{
		LOG::show("SaveXmlFile::Save file(%s) successfully!",strFilePath.c_str());
	}
	else
	{
		LOG::show("SaveXmlFile::Save file(%s) failed!",strFilePath.c_str());
	}
	return bSucceed;
}

 vector<DWORD> CSystemResource::GetProcessId(TCHAR szProcessName[])
{
	
	return m_ProcessTool.GetProcessId(szProcessName);

}


vector<MODULEENTRY32> CSystemResource::GetProcessModules(DWORD dwPID)
{
	return m_ProcessTool.GetProcessModules(dwPID);

}

vector<THREADENTRY32> CSystemResource::GetProcessThreads(DWORD dwOwnerPID)
{
	return m_ProcessTool.GetProcessThreads(dwOwnerPID);

}


UInt64 CSystemResource::GetProcessMemoryUsage(DWORD dwPID)
{
	return m_ProcessTool.GetProcessMemoryUsage(dwPID);
}



__int32 CSystemResource::GetProcessCPUUsage(DWORD dwPID)
{

	return m_ProcessTool.GetProcessCPUUsage(dwPID);

}

BOOL CSystemResource::TerminateProcess(DWORD dwPid)
{
	return m_ProcessTool.TerminateProcess(dwPid);
}


tstring CSystemResource::GetOSName()
{
	tstring osname;
	SYSTEM_INFO info;      
	SafeGetNativeSystem(&info);     
	OSVERSIONINFOEX os;   
	os.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);     

	osname = _T("unknown OperatingSystem.");  

	if(GetVersionEx((OSVERSIONINFO *)&os))  
	{   

		LOG::show(TEXT("GetOSVersion::MajorVer = %d,MinorVer = %d"),os.dwMajorVersion,os.dwMinorVersion);
		switch(os.dwMajorVersion)
		{  
		case 4:  
			{
				switch(os.dwMinorVersion) 
				{   
				case 0:  
					if(os.dwPlatformId==VER_PLATFORM_WIN32_NT)  
						osname =_T("Microsoft Windows NT 4.0"); //1996年7月发布   
					else if(os.dwPlatformId==VER_PLATFORM_WIN32_WINDOWS)  
						osname =_T("Microsoft Windows 95");  
					break;  
				case 10:  
					osname =_T("Microsoft Windows 98");  
					break;  
				case 90:  
					osname =_T("Microsoft Windows Me");  
					break;  
				}  
				break;  
			}
		case 5:  
			{
				switch(os.dwMinorVersion)   
				{   
				case 0:  
					osname =_T("Microsoft Windows 2000");//1999年12月发布  
					break;  

				case 1:  
					osname =_T("Microsoft Windows XP");//2001年8月发布  
					break;  

				case 2:  
					if(os.wProductType==VER_NT_WORKSTATION   
						&& info.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)  
					{  
						osname =_T("Microsoft Windows XP Professional x64 Edition");  
					}  
					else if(GetSystemMetrics(SM_SERVERR2)==0)  
						osname =_T("Microsoft Windows Server 2003");//2003年3月发布   
					else if(GetSystemMetrics(SM_SERVERR2)!=0)  
						osname =_T("Microsoft Windows Server 2003 R2");  
					break;  
				}  
				break;  
			}
		case 6: 
			{
				switch(os.dwMinorVersion)  
				{  
				case 0:  
					if(os.wProductType == VER_NT_WORKSTATION)  
						osname =_T("Microsoft Windows Vista");  
					else  
						osname =_T("Microsoft Windows Server 2008");//服务器版本   
					break;  
				case 1:  
					{
						if(os.wProductType == VER_NT_WORKSTATION)  
							osname =_T("Microsoft Windows 7");  
						else  
							osname =_T("Microsoft Windows Server 2008 R2");  
						break; 
					}

				case 2:
					{
						if(os.wProductType == VER_NT_WORKSTATION)
							osname = _T("Windows 8");
						else
							osname = _T("Windows Server 2012");
							
						break;
					}
				case 3:
					{
						if(os.wProductType == VER_NT_WORKSTATION)
							osname = _T("Windows 8.1");
						else
							osname = _T("Windows Server 2012 R2");
						break;
					}
				}  
				break;
			}

		case 10:
			{
				switch(os.dwMinorVersion)
				{
				case 0:
					if(os.wProductType == VER_NT_WORKSTATION)
						osname = _T("Windows 10");
					else
						osname = _T("Windows Server 2016 Technical Preview");
					break;
				}
				break;
			}
			
		}  
	}  

	return osname;
}


tstring CSystemResource::GetOSVerMark()  
{  
	tstring vmark;
	OSVERSIONINFOEX os;   
	os.dwOSVersionInfoSize=sizeof(OSVERSIONINFOEX);   
	vmark = _T("");  

	if(GetVersionEx((OSVERSIONINFO*)&os))  
	{   
		switch(os.dwMajorVersion)  
		{                
		case 5:   
			switch(os.dwMinorVersion)  
			{   
			case 0:                  //Windows 2000   
				if(os.wSuiteMask==VER_SUITE_ENTERPRISE)   
					vmark=_T("Advanced Server");   
				break;   
			case 1:                  //Windows XP   
				if(os.wSuiteMask==VER_SUITE_EMBEDDEDNT)   
					vmark=_T("Embedded");   
				else if(os.wSuiteMask==VER_SUITE_PERSONAL)   
					vmark=_T("Home Edition");   
				else   
					vmark=_T("Professional");   
				break;   
			case 2:   
				if(GetSystemMetrics(SM_SERVERR2)==0   
					&&os.wSuiteMask==VER_SUITE_BLADE)  //Windows Server 2003   
					vmark=_T("Web Edition");  
				else if(GetSystemMetrics(SM_SERVERR2)==0   
					&& os.wSuiteMask==VER_SUITE_COMPUTE_SERVER)  
					vmark=_T("Compute Cluster Edition");  
				else if(GetSystemMetrics(SM_SERVERR2)==0  
					&& os.wSuiteMask==VER_SUITE_STORAGE_SERVER)  
					vmark=_T("Storage Server");  
				else if(GetSystemMetrics(SM_SERVERR2)==0   
					&& os.wSuiteMask==VER_SUITE_DATACENTER)  
					vmark=_T("Datacenter Edition");  
				else if(GetSystemMetrics(SM_SERVERR2)==0   
					&& os.wSuiteMask==VER_SUITE_ENTERPRISE)  
					vmark=_T("Enterprise Edition");  
				else if(GetSystemMetrics(SM_SERVERR2)!=0  
					&& os.wSuiteMask==VER_SUITE_STORAGE_SERVER)  
					vmark=_T("Storage Server");  
				break;  
			}  
			break;  

		case 6:  
			switch(os.dwMinorVersion)  
			{  
			case 0:  
				if(os.wProductType!=VER_NT_WORKSTATION   
					&& os.wSuiteMask==VER_SUITE_DATACENTER)  
					vmark=_T("Datacenter Server");  
				else if(os.wProductType!=VER_NT_WORKSTATION   
					&& os.wSuiteMask==VER_SUITE_ENTERPRISE)  
					vmark=_T("Enterprise");  
				else if(os.wProductType==VER_NT_WORKSTATION   
					&& os.wSuiteMask==VER_SUITE_PERSONAL)  //Windows Vista  
					vmark =_T("Home");  
				break;  
			}  
			break;  
		}  
	}  
}  

BOOL CSystemResource::CreateSystemProcess(LPTSTR szProcessName)
{
	HANDLE hProcess; 
	HANDLE hToken, hNewToken; 
	DWORD dwPid; 
	VCTPID vctPid;

	PACL pOldDAcl = NULL; 
	PACL pNewDAcl = NULL; 
	BOOL bDAcl; 
	BOOL bDefDAcl; 
	DWORD dwRet; 

	PACL pSacl = NULL; 
	PSID pSidOwner = NULL; 
	PSID pSidPrimary = NULL; 
	DWORD dwAclSize = 0; 
	DWORD dwSaclSize = 0; 
	DWORD dwSidOwnLen = 0; 
	DWORD dwSidPrimLen = 0; 

	DWORD dwSDLen; 
	EXPLICIT_ACCESS ea; 
	PSECURITY_DESCRIPTOR pOrigSd = NULL; 
	PSECURITY_DESCRIPTOR pNewSd = NULL; 

	STARTUPINFO si; 
	PROCESS_INFORMATION pi; 

	BOOL bError; 

	if ( !EnablePrivilege(SE_DEBUG_NAME, TRUE) ) 
	{ 
		printf( "EnableDebugPriv() to failed!\n" ); 

		bError = TRUE; 
		goto Cleanup; 
	} 

	// 
	// 选择 WINLOGON 进程 
	// 
	
	vctPid = GetProcessId(_T("WINLOGON.EXE" ));
	if ( vctPid.size()==0 ) 
	{ 
		LOG::printError(_T("GetProcessId()"));   

		bError = TRUE; 
		goto Cleanup; 
	} 
	dwPid = vctPid[0];
	hProcess = OpenProcess( PROCESS_QUERY_INFORMATION, FALSE, dwPid ); 
	if ( hProcess == NULL ) 
	{ 
		LOG::printError(_T("OpenProcess()"));   

		bError = TRUE; 
		goto Cleanup; 
	} 

	if ( !OpenProcessToken( hProcess, READ_CONTROL | WRITE_DAC, &hToken ) ) 
	{ 
		LOG::printError("OpenProcessToken()"); 

		bError = TRUE; 
		goto Cleanup; 
	} 

	// 
	// 设置 ACE 具有所有访问权限 
	// 
	ZeroMemory( &ea, sizeof( EXPLICIT_ACCESS ) ); 
	BuildExplicitAccessWithName( &ea, 
		"Everyone", 
		TOKEN_ALL_ACCESS, 
		GRANT_ACCESS, 
		0 ); 

	if ( !GetKernelObjectSecurity( hToken, 
		DACL_SECURITY_INFORMATION, 
		pOrigSd, 
		0, 
		&dwSDLen ) ) 
	{ 
		// 
		// 第一次调用给出的参数肯定返回这个错误，这样做的目的是 
		// 为了得到原安全描述符 pOrigSd 的长度 
		// 
		if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) 
		{ 
			pOrigSd = ( PSECURITY_DESCRIPTOR ) HeapAlloc( GetProcessHeap(), 
				HEAP_ZERO_MEMORY, 
				dwSDLen ); 
			if ( pOrigSd == NULL ) 
			{ 
				printf( "Allocate pSd memory to failed!\n" ); 

				bError = TRUE; 
				goto Cleanup; 
			} 

			// 
			// 再次调用才正确得到安全描述符 pOrigSd 
			// 
			if ( !GetKernelObjectSecurity( hToken, 
				DACL_SECURITY_INFORMATION, 
				pOrigSd, 
				dwSDLen, 
				&dwSDLen ) ) 
			{ 
				printf( "GetKernelObjectSecurity() = %d\n", GetLastError() ); 
				bError = TRUE; 
				goto Cleanup; 
			} 
		} 
		else 
		{ 
			printf( "GetKernelObjectSecurity() = %d\n", GetLastError() ); 
			bError = TRUE; 
			goto Cleanup; 
		} 
	} 

	// 
	// 得到原安全描述符的访问控制列表 ACL 
	// 
	if ( !GetSecurityDescriptorDacl( pOrigSd, &bDAcl, &pOldDAcl, &bDefDAcl ) ) 
	{ 
		printf( "GetSecurityDescriptorDacl() = %d\n", GetLastError() ); 

		bError = TRUE; 
		goto Cleanup; 
	} 

	// 
	// 生成新 ACE 权限的访问控制列表 ACL 
	// 
	dwRet = SetEntriesInAcl( 1, &ea, pOldDAcl, &pNewDAcl ); 
	if ( dwRet != ERROR_SUCCESS ) 
	{ 
		printf( "SetEntriesInAcl() = %d\n", GetLastError() ); 
		pNewDAcl = NULL; 

		bError = TRUE; 
		goto Cleanup; 
	} 

	if ( !MakeAbsoluteSD( pOrigSd, 
		pNewSd, 
		&dwSDLen, 
		pOldDAcl, 
		&dwAclSize, 
		pSacl, 
		&dwSaclSize, 
		pSidOwner, 
		&dwSidOwnLen, 
		pSidPrimary, 
		&dwSidPrimLen ) ) 
	{ 
		// 
		// 第一次调用给出的参数肯定返回这个错误，这样做的目的是 
		// 为了创建新的安全描述符 pNewSd 而得到各项的长度 
		// 
		if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) 
		{ 
			pOldDAcl = ( PACL ) HeapAlloc( GetProcessHeap(), 
				HEAP_ZERO_MEMORY, 
				dwAclSize ); 
			pSacl = ( PACL ) HeapAlloc( GetProcessHeap(), 
				HEAP_ZERO_MEMORY, 
				dwSaclSize ); 
			pSidOwner = ( PSID ) HeapAlloc( GetProcessHeap(), 
				HEAP_ZERO_MEMORY, 
				dwSidOwnLen ); 
			pSidPrimary = ( PSID ) HeapAlloc( GetProcessHeap(), 
				HEAP_ZERO_MEMORY, 
				dwSidPrimLen ); 
			pNewSd = ( PSECURITY_DESCRIPTOR ) HeapAlloc( GetProcessHeap(), 
				HEAP_ZERO_MEMORY, 
				dwSDLen ); 

			if ( pOldDAcl == NULL || 
				pSacl == NULL || 
				pSidOwner == NULL || 
				pSidPrimary == NULL || 
				pNewSd == NULL ) 
			{ 
				LOG::printError("Allocate SID or ACL to failed!\n" ); 

				bError = TRUE; 
				goto Cleanup; 
			} 

			// 
			// 再次调用才可以成功创建新的安全描述符 pNewSd 
			// 但新的安全描述符仍然是原访问控制列表 ACL 
			// 
			if ( !MakeAbsoluteSD( pOrigSd, 
				pNewSd, 
				&dwSDLen, 
				pOldDAcl, 
				&dwAclSize, 
				pSacl, 
				&dwSaclSize, 
				pSidOwner, 
				&dwSidOwnLen, 
				pSidPrimary, 
				&dwSidPrimLen ) ) 
			{ 
				printf( "MakeAbsoluteSD() = %d\n", GetLastError() ); 

				bError = TRUE; 
				goto Cleanup; 
			} 
		} 
		else 
		{ 
			printf( "MakeAbsoluteSD() = %d\n", GetLastError() ); 

			bError = TRUE; 
			goto Cleanup; 
		} 
	} 

	// 
	// 将具有所有访问权限的访问控制列表 pNewDAcl 加入到新的 
	// 安全描述符 pNewSd 中 
	// 
	if ( !SetSecurityDescriptorDacl( pNewSd, bDAcl, pNewDAcl, bDefDAcl ) ) 
	{ 
		LOG::printError(_T("SetSecurityDescriptorDacl()")); 

		bError = TRUE; 
		goto Cleanup; 
	} 

	// 
	// 将新的安全描述符加到 TOKEN 中 
	// 
	if ( !SetKernelObjectSecurity( hToken, DACL_SECURITY_INFORMATION, pNewSd ) ) 
	{ 
		printf( "SetKernelObjectSecurity() = %d\n", GetLastError() ); 

		bError = TRUE; 
		goto Cleanup; 
	} 

	// 
	// 再次打开 WINLOGON 进程的 TOKEN，这时已经具有所有访问权限 
	// 
	if ( !OpenProcessToken( hProcess, TOKEN_ALL_ACCESS, &hToken ) ) 
	{ 
		LOG::printError( _T("OpenProcessToken()"));   

		bError = TRUE; 
		goto Cleanup; 
	} 

	// 
	// 复制一份具有相同访问权限的 TOKEN 
	// 
	if ( !DuplicateTokenEx( hToken, 
		TOKEN_ALL_ACCESS, 
		NULL, 
		SecurityImpersonation, 
		TokenPrimary, 
		&hNewToken ) ) 
	{ 
		LOG::printError(_T("DuplicateTokenEx()"));   

		bError = TRUE; 
		goto Cleanup; 
	} 


	ZeroMemory( &si, sizeof( STARTUPINFO ) ); 
	si.cb = sizeof( STARTUPINFO ); 

	// 
	// 不虚拟登陆用户的话，创建新进程会提示 
	// 1314 客户没有所需的特权错误 
	// 
	ImpersonateLoggedOnUser( hNewToken ); 


	// 
	// 我们仅仅是需要建立高权限进程，不用切换用户 
	// 所以也无需设置相关桌面，有了新 TOKEN 足够 
	// 


	// 
	// 利用具有所有权限的 TOKEN，创建高权限进程 
	// 
	if ( !CreateProcessAsUser( hNewToken, 
		NULL, 
		szProcessName, 
		NULL, 
		NULL, 
		FALSE, 
		NULL, //NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE, 
		NULL, 
		NULL, 
		&si, 
		&pi ) ) 
	{ 
		LOG::printError( "CreateProcessAsUser()");   

		bError = TRUE; 
		goto Cleanup; 
	} 

	bError = FALSE; 

Cleanup: 
	if ( pOrigSd ) 
	{ 
		HeapFree( GetProcessHeap(), 0, pOrigSd ); 
	} 
	if ( pNewSd ) 
	{ 
		HeapFree( GetProcessHeap(), 0, pNewSd ); 
	} 
	if ( pSidPrimary ) 
	{ 
		HeapFree( GetProcessHeap(), 0, pSidPrimary ); 
	} 
	if ( pSidOwner ) 
	{ 
		HeapFree( GetProcessHeap(), 0, pSidOwner ); 
	} 
	if ( pSacl ) 
	{ 
		HeapFree( GetProcessHeap(), 0, pSacl ); 
	} 
	if ( pOldDAcl ) 
	{ 
		HeapFree( GetProcessHeap(), 0, pOldDAcl ); 
	} 

	CloseHandle( pi.hProcess ); 
	CloseHandle( pi.hThread ); 
	CloseHandle( hToken ); 
	CloseHandle( hNewToken ); 
	CloseHandle( hProcess ); 

	if ( bError ) 
	{ 
		return FALSE; 
	} 

	return TRUE; 
}



BOOL CSystemResource::lzmaCompressfile(const TCHAR desfile[],const TCHAR srcFile[])
{
	BOOL bSuccess = FALSE;
	byte  prop[5]={0};
	size_t nPropSize = 5;
	if(m_pLzmaCom)
	{
		FILE *fp;
		_tfopen_s(&fp,srcFile,_T("rb"));
		if(NULL !=fp)
		{
			fseek(fp,0,SEEK_END);
			size_t nSrcLen = ftell(fp);
			fseek(fp,0,SEEK_SET);

			byte* pSrcData = (byte*)malloc(nSrcLen);
			fread(pSrcData,nSrcLen,1,fp);
			fclose(fp);

			size_t nDesLen = nSrcLen;
			byte* pDesData = (byte*)malloc(nDesLen);
			int nRet = m_pLzmaCom(pDesData,&nDesLen,pSrcData,nSrcLen,
				prop,&nPropSize,9,1<<24,3,0,2,32,2);
			if(SZ_ERROR_OUTPUT_EOF == nRet)
			{
				free(pDesData);
				nDesLen += nSrcLen;
				pDesData = (byte*)malloc(nDesLen);
				nRet = m_pLzmaCom(pDesData,&nDesLen,pSrcData,nSrcLen,prop,&nPropSize,
					9,(1<<24),3,0,2,32,2);
			}
			if(SZ_OK == nRet)
			{
				_tfopen_s(&fp,desfile,_T("wb+"));
				if(NULL != fp)
				{
					if(lzmaWriteHeadFile(fp,prop,(unsigned __int64)nSrcLen))
					{
						fseek(fp,(8+LZMA_PROPS_SIZE),SEEK_SET);
						if(fwrite(pDesData,nDesLen,1,fp)==nDesLen)
						{
							bSuccess = TRUE;
						}
						
					}
					fclose(fp);
				}
			}
			free(pDesData);
			free(pSrcData);


		}
	}

	return bSuccess;
}


BOOL CSystemResource::lzmaWriteHeadFile(FILE *file,_TUCHAR *prop,unsigned __int64 unPackSize)
{
	int iRet;
	byte header[LZMA_PROPS_SIZE + 8] = {0};
	for(int i =0;i<LZMA_PROPS_SIZE;i++)
	{
		header[i] = prop[i];
	}

	for(int i =0;i<8;i++)
	{
		header[LZMA_PROPS_SIZE+i] = (byte)(unPackSize>>(i*8));
	}


	fseek(file,0,SEEK_SET);
	iRet = fwrite((char*)header,1,(LZMA_PROPS_SIZE+8),file);
	if(iRet !=(LZMA_PROPS_SIZE+8))
	{
		return FALSE;
	}
	fseek(file,0,SEEK_SET);
	return TRUE;
}



BOOL CSystemResource::lzmaUncompressfile(const TCHAR desFile[],const TCHAR srcFile[])
{
	BOOL bSuccess = FALSE;
	byte prop[5] ={0};
	size_t nPropSize = 5;
	unsigned __int64  unPackSize;
	if(m_pLzmaUncom)
	{
		FILE *fp;
		_tfopen_s(&fp,srcFile,TEXT("rb"));
		if(NULL != fp)
		{
			if(lzmaReadHeadFile(fp,prop,&unPackSize))
			{
				fseek(fp,0,SEEK_END);
				size_t nSrcLen = ftell(fp);
				fseek(fp,0,SEEK_SET);

				byte *pSrcData = (byte*)malloc(nSrcLen);
				fread(pSrcData,nSrcLen,1,fp);
				fclose(fp);

				size_t nDesLen = (size_t)unPackSize;
				byte* pDesData = (byte*)malloc(nDesLen);
				nSrcLen -= (8+LZMA_PROPS_SIZE);
				int nRet = m_pLzmaUncom(pDesData,&nDesLen,&pSrcData[8+LZMA_PROPS_SIZE],&nSrcLen,
					prop,&nPropSize);
				if(SZ_OK == nRet)
				{
					_tfopen_s(&fp,desFile,TEXT("wb+"));
					if(NULL != fp)
					{
						if(fwrite(pDesData,nDesLen,1,fp)==nDesLen)
						{
							bSuccess = TRUE;
						}
						fclose(fp);
					}
				}
				free(pDesData);
				free(pSrcData);
			}
		}
	}
	return bSuccess;
}


BOOL CSystemResource::lzmaReadHeadFile(FILE *file,_TUCHAR *prop,unsigned __int64 *unPackSize)
{
	int iRet;
	fseek(file,0,SEEK_SET);
	byte header[8+LZMA_PROPS_SIZE]={0};
	iRet = fread(header,1,(LZMA_PROPS_SIZE+8),file);
	if(iRet!=(LZMA_PROPS_SIZE+8))
	{
		LOG::printError(_T("readHeadFile::fread header error\n"));
		return FALSE;
	}
	if(prop)
	{
		for(int i =0;i<LZMA_PROPS_SIZE;i++)
		{
			prop[i] = header[i];
		}
	}
	*unPackSize = 0;
	for(int i = 0;i<8;i++)
	{
		*unPackSize += (unsigned __int64)header[LZMA_PROPS_SIZE+i]<<(i*8);
	}
	return TRUE;
}

BOOL CSystemResource::SafeGetNativeSystem(LPSYSTEM_INFO lpSystemInfo)
{
	BOOL bSucceed = FALSE;
	if(NULL == lpSystemInfo)return FALSE;
	HMODULE hModule;
	hModule = LoadLibrary(_T("Kernel32.dll"));
	if(hModule)
	{
		typedef VOID (WINAPI *LPFN_GetNativeSystemInfo)(LPSYSTEM_INFO lpSystemInfo);
		LPFN_GetNativeSystemInfo fnGetNativeSystemInfo = (LPFN_GetNativeSystemInfo)
			GetProcAddress(hModule,_T("GetNativeSystemInfo"));

		if(fnGetNativeSystemInfo)
		{
			fnGetNativeSystemInfo(lpSystemInfo);
			bSucceed = TRUE;
		}
		else
		{
			GetSystemInfo(lpSystemInfo);
		}
	}
	if(hModule)FreeLibrary(hModule);
	return bSucceed;
}

BOOL CSystemResource::SaveJsonFile(tstring strFilePath)
{
	Json::FastWriter write;
	tstring json_file;
	FILE *file;
	BOOL bSucceed = FALSE;

	json_file = write.write(m_JsonSystem);
	LOG::show("SaveJsonFile::Start");
	

	_tfopen_s(&file,strFilePath.c_str(),TEXT("wb+"));
	if(file)
	{
		fwrite(json_file.c_str(),1,json_file.size(),file);
		bSucceed = TRUE;
		fclose(file);
	}

	if(bSucceed)
	{
		LOG::show("SaveJsonFile::Save file(%s) successfully!",strFilePath.c_str());
	}
	else
	{
		LOG::show("SaveJsonFile::Save file(%s) failed!",strFilePath.c_str());
	}
	return bSucceed;

}



