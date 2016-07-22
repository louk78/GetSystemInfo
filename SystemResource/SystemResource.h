#ifndef C_SYSTEM_RESOURCE_H
#define C_SYSTEM_RESOURCE_H

#include "processtool.h"
#include "autorunstool.h"
#include "include/jsoncpp/json.h"
#include "pugixml.hpp"

class CSystemResource
{
public:
	CSystemResource();
	~CSystemResource();
public:
	typedef int (_stdcall *Func_RtlAdjustPrivilege)(ULONG,BOOLEAN,BOOLEAN,PBOOLEAN);

	typedef int(_stdcall *lzmaCompress)(_TUCHAR*,size_t*,const _TUCHAR*,
		size_t,_TUCHAR*,size_t*,int,
		unsigned int,int,int,int,int,int);

	typedef int(_stdcall *lzmaUncompress)(_TUCHAR *,size_t *,const _TUCHAR *,
		size_t*,const _TUCHAR *,size_t*);

public:
	

public:
	MEMORYSTATUSEX GetTotalMemoryStatus();
	UINT GetTotalCpuStatus();
	BOOL GetProcessList2();
	BOOL GetSystemResource();
	__int64 CompareFileTime ( FILETIME time1, FILETIME time2 );
public:
	VCTMOD GetProcessModules(DWORD dwPID);
	VCTTHREAD GetProcessThreads(DWORD dwOwnerPID);
	__int32 GetProcessCPUUsage(DWORD dwPID);
	UInt64 GetProcessMemoryUsage(DWORD dwPID);
	VCTPID GetProcessId(TCHAR szExeFile[]);
	BOOL TerminateProcess(DWORD dwPid);

public:
	BOOL SaveXmlFile(tstring strFilePath);
	BOOL SaveJsonFile(tstring strFilePath);
	BOOL EnablePrivilege(PCTSTR szPrivilege, BOOL fEnable);
	tstring GetOSName();
	tstring GetOSVerMark();
	BOOL CreateSystemProcess(LPTSTR szProcessName);
	BOOL SafeGetNativeSystem(LPSYSTEM_INFO lpSystemInfo);
public:
	BOOL lzmaCompressfile(const TCHAR desfile[],const TCHAR srcFile[]);	
	BOOL lzmaUncompressfile(const TCHAR desFile[],const TCHAR srcFile[]);
public:



private:
	BOOL Create();
	BOOL Initialize();
	BOOL lzmaReadHeadFile(FILE *file,_TUCHAR *prop,unsigned __int64 *unPackSize);
	BOOL lzmaWriteHeadFile(FILE *file,_TUCHAR *prop,unsigned __int64 unPackSize);
private:
	Json::Value m_JsonSystem;
	pugi::xml_document m_XMLDoc;
	pugi::xml_node m_XMLSystem;
	lzmaCompress m_pLzmaCom;
	lzmaUncompress m_pLzmaUncom;
	HMODULE m_hLzmaMod;
	int m_nProcessorCount;
	CProcessTool m_ProcessTool;
	CAutoRunsTool m_AutoRunsTool;
};

#endif


