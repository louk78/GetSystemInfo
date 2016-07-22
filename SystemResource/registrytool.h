#ifndef _REGISTRY_TOOL_H_
#define _REGISTRY_TOOL_H_

class CRegistryTool
{



public:
	CRegistryTool();
	virtual ~CRegistryTool();

public:
	BOOL GetKeyValueData(HKEY hKey,MAPKEYVALUEDATA &mapKeyValueData);
	BOOL StringToRootKey(_In_ tstring strKey,_Out_ HKEY *hKey);
	BOOL RootKeyToString(_In_ HKEY hKey,_Out_ tstring *strKey);
	BOOL GetRegPathValueData(tstring regPath,MAPKEYVALUEDATA &mapKeyValueData);
	BOOL GetAllSubKey(tstring regPath,VCTSTRING *vctStr);
	BOOL RegPathToHkeyAndSubKey(tstring regPath,HKEY *hKey,tstring *strSubKey);


};




#endif