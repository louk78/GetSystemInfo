#include "stdafx.h"
#include "autorunstool.h"


CAutoRunsTool::CAutoRunsTool()
{

}

CAutoRunsTool::~CAutoRunsTool()
{

}


BOOL CAutoRunsTool::GetRegRun(VCTAUTOSTARTINFO &vctAutoStartInfo)
{
	MAPKEYVALUEDATA mapKeyValueData;
	SYSTEM_AUTOSTARTUP_INFO autoStartInfo;
	int iTotal;
	tstring strMainKey;

	vctAutoStartInfo.clear();
	tstring regKeyPath[]={
		TEXT("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
		TEXT("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
		TEXT("HKEY_LOCAL_MACHINE\\Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Run")
	};


	iTotal = sizeof(regKeyPath)/sizeof(tstring);
	for(int i =0;i<iTotal;i++)
	{
		m_RegistryTool.GetRegPathValueData(regKeyPath[i],mapKeyValueData);
		autoStartInfo.strRegPath = regKeyPath[i];
		autoStartInfo.mapKeyValueData = mapKeyValueData;
		vctAutoStartInfo.push_back(autoStartInfo);
	}

	return TRUE;

}


BOOL CAutoRunsTool::GetServicesDrives()
{

	tstring strSubKey;
	int iRetCode;
	HKEY hSubKey;
	MAPKEYVALUEDATA mapKeyValueData;
	tstring strValue;
	
	VCTSTRING vctStr;
	tstring strRegPath = 
	TEXT("HKEY_LOCAL_MACHINE\\System\\CurrentControlSet\\Services");

	m_RegistryTool.GetAllSubKey(strRegPath,&vctStr);

	for(int i =0;i<vctStr.size();i++)
	{
		m_RegistryTool.GetRegPathValueData(vctStr[i],mapKeyValueData);
		MAPKEYVALUEDATA::iterator iter;
		for(iter = mapKeyValueData.begin();iter!=mapKeyValueData.end();++iter)
		{
			if(_tcscmp(TEXT("Type"),iter->first.c_str())==0)
			{
				
			}
		}
	}
	


	return TRUE;
}