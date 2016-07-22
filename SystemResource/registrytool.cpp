#include "stdafx.h"
#include "registrytool.h"
#include "shlwapi.h"
#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383



CRegistryTool::CRegistryTool()
{

}

CRegistryTool::~CRegistryTool()
{

}


BOOL CRegistryTool::GetKeyValueData(HKEY hKey,MAPKEYVALUEDATA &mapKeyValueData)
{
	mapKeyValueData.clear();

	TCHAR strClass[MAX_KEY_LENGTH] ={0};
	DWORD dwClass = MAX_KEY_LENGTH;
	DWORD dwSubKeys;
	DWORD dwMaxSubKeyLen;
	DWORD dwMaxClassLen;
	DWORD dwValues;
	DWORD dwMaxValueNameLen;
	DWORD dwMaxValueLen;
	DWORD dwSecurityDescriptor;
	FILETIME lpftLastWriteTime;

	LONG iRetCode;

	iRetCode = RegQueryInfoKey(
		hKey,
		strClass,
		&dwClass,
		NULL,
		&dwSubKeys,
		&dwMaxSubKeyLen,
		&dwMaxClassLen,
		&dwValues,
		&dwMaxValueNameLen,
		&dwMaxValueLen,
		&dwSecurityDescriptor,
		&lpftLastWriteTime
		);
	if(iRetCode !=ERROR_SUCCESS)return FALSE;
		

		TCHAR szValueName[MAX_VALUE_NAME]={0};
		DWORD cchValueName;
		BYTE byData[MAX_PATH]={0};
		DWORD cbData;
		DWORD dwType;
		tstring strData;
		if(dwValues)
		{
			for(int i =0;i<dwValues;i++)
			{
				cchValueName = MAX_VALUE_NAME;
				cbData = MAX_PATH;
				memset(szValueName,0,MAX_VALUE_NAME);
				memset(byData,0,MAX_PATH);
				RegEnumValue(
					hKey,
					i,
					szValueName,
					&cchValueName,
					NULL,
					&dwType,
					byData,
					&cbData
					);
				if(cchValueName&&cbData)
				{
					if(_tcslen(szValueName))
					{
						strData.clear();
						strData.append((TCHAR*)byData,cbData);
						mapKeyValueData.insert(PAIRKEYVALUEDATA(szValueName,strData));
					}
				}
				
				
			}
		}


		return TRUE;
}

BOOL CRegistryTool::StringToRootKey(tstring strKey,HKEY *hKey)
{
	BOOL bSucceed = FALSE;

	transform(strKey.begin(),strKey.end(),strKey.begin(),_totupper);
	if(strKey.compare(_T("HKEY_CLASSES_ROOT"))==0)
	{
		*hKey = HKEY_CLASSES_ROOT;
		bSucceed = TRUE;
	}
	else if(strKey.compare(_T("HKEY_CURRENT_CONFIG"))==0)
	{
		*hKey = HKEY_CURRENT_CONFIG;
		bSucceed = TRUE;
	}
	else if(strKey.compare(_T("HKEY_CURRENT_USER"))==0)
	{
		*hKey = HKEY_CURRENT_USER;
		bSucceed = TRUE;
	}
	else if(strKey.compare(_T("HKEY_LOCAL_MACHINE"))==0)
	{
		*hKey = HKEY_LOCAL_MACHINE;
		bSucceed = TRUE;
	}
	else if(strKey.compare(_T("HKEY_PERFORMANCE_DATA")) ==0)
	{
		*hKey = HKEY_PERFORMANCE_DATA;
		bSucceed = TRUE;
	}
	else if(strKey.compare(_T("HKEY_USERS")) == 0)
	{
		*hKey = HKEY_USERS;
		bSucceed = TRUE;
	}

	return bSucceed;
}

BOOL CRegistryTool::RootKeyToString(_In_ HKEY hKey,_Out_ tstring *strKey)
{
	BOOL bSucceed = FALSE;
	if(hKey ==HKEY_CLASSES_ROOT)
	{
		*strKey = TEXT("HKEY_CLASSES_ROOT");
		bSucceed = TRUE;
	}
	else if(hKey == HKEY_CURRENT_CONFIG)
	{
		*strKey = TEXT("HKEY_CURRENT_CONFIG");
		bSucceed = TRUE;
	}
	else if(hKey == HKEY_CURRENT_USER)
	{
		*strKey = TEXT("HKEY_CURRENT_USER");
		bSucceed = TRUE;
	}
	else if(hKey == HKEY_LOCAL_MACHINE)
	{
		*strKey = TEXT("HKEY_LOCAL_MACHINE");
		bSucceed = TRUE;
	}
	else if(hKey == HKEY_PERFORMANCE_DATA)
	{
		*strKey = TEXT("HKEY_PERFORMANCE_DATA");
		bSucceed = TRUE;
	}
	else if(hKey == HKEY_USERS)
	{
		*strKey = TEXT("HKEY_USERS");
		bSucceed = TRUE;
	}

	return bSucceed;
}

BOOL CRegistryTool::GetRegPathValueData(tstring regPath,MAPKEYVALUEDATA &mapKeyValueData)
{
	mapKeyValueData.clear();
	int iRetCode;
	int iPos;
	HKEY hSubKey;
	HKEY hRootKey;
	tstring strSubKey;
	
	if(!RegPathToHkeyAndSubKey(regPath,&hRootKey,&strSubKey))
		return FALSE;

	iRetCode = RegOpenKeyEx(
		hRootKey,
		strSubKey.c_str(),
		0,
		KEY_ALL_ACCESS|KEY_WOW64_64KEY,
		&hSubKey
		);
	if(iRetCode!=ERROR_SUCCESS)return FALSE;
	
	GetKeyValueData(hSubKey,mapKeyValueData);

	RegCloseKey(hSubKey);
	
	return TRUE;
}

BOOL CRegistryTool::GetAllSubKey(tstring regPath,VCTSTRING *vctStr)
{
	HKEY hRootKey;
	HKEY hSubKey;
	tstring strSubKey;
	tstring strSubKeyPath;
	int iRetCode;
	BOOL bResult =FALSE;

	TCHAR strClass[MAX_KEY_LENGTH]={0};
	DWORD dwClass = MAX_KEY_LENGTH;
	DWORD dwSubKey;
	DWORD dwMaxSubKeyLen;
	DWORD dwMaxClassLen;
	DWORD dwValues;
	DWORD dwMaxValueNameLen;
	DWORD dwMaxValueLen;
	DWORD dwSecurityDescriptor;
	FILETIME ftLastWriteTime;

	TCHAR strName[MAX_KEY_LENGTH]={0};
	DWORD dwName;



	if(!RegPathToHkeyAndSubKey(regPath,&hRootKey,&strSubKey))
		return FALSE;

	iRetCode = RegOpenKeyEx(
		hRootKey,
		strSubKey.c_str(),
		0,
		KEY_ALL_ACCESS|KEY_WOW64_64KEY,
		&hSubKey
		);
	if(iRetCode==ERROR_SUCCESS)
	{
		iRetCode = RegQueryInfoKey(
			hSubKey,
			strClass,
			&dwClass,
			NULL,
			&dwSubKey,
			&dwMaxSubKeyLen,
			&dwMaxClassLen,
			&dwValues,
			&dwMaxValueNameLen,
			&dwMaxValueLen,
			&dwSecurityDescriptor,
			&ftLastWriteTime
			);

		if(dwSubKey)
		{
			for(int i =0;i<dwSubKey;i++)
			{
				strName[0]='\0';
				dwName = MAX_KEY_LENGTH;
				iRetCode = RegEnumKeyEx(
					hSubKey,
					i,
					strName,
					&dwName,
					NULL,
					NULL,
					NULL,
					&ftLastWriteTime
					);
				strSubKeyPath = regPath;
				strSubKeyPath +="\\";
				strSubKeyPath += strName;
				vctStr->push_back(strSubKeyPath);
			}
		}
		bResult = TRUE;
	}

	RegCloseKey(hSubKey);
	return bResult;
}

BOOL CRegistryTool::RegPathToHkeyAndSubKey(tstring regPath,HKEY *hKey,tstring *strSubKey)
{
	int iPos;
	tstring strRootKey;
	BOOL bResult = FALSE;

	if(regPath.size()==0)
		return FALSE;

	iPos = regPath.find("\\");

	if(iPos == tstring::npos)
		return FALSE;

	strRootKey = regPath.substr(0,iPos);
	if(strRootKey.size()==0)
		return FALSE;

	if(StringToRootKey(strRootKey,hKey))
		bResult = TRUE;

	*strSubKey = regPath.substr(iPos+1,regPath.size()-iPos-1);
	

	return bResult;
}