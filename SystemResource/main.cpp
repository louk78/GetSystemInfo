#include "stdafx.h"
#include "SystemResource.h"
#include <fstream>
#include "io.h"
#include "atltime.h"
#include "log.h"
#include "include/jsoncpp/json.h"
#include "atlstr.h"

tstring g_strAppDir;





int main()
{

	tstring str,strdat;
	CSystemResource sys;
	sys.GetSystemResource();
	TCHAR szFilePath[MAX_PATH]={0};
	GetModuleFileName(NULL,szFilePath,MAX_PATH);
	str= szFilePath;
	int iPos = str.rfind('\\');
	str = str.substr(0,iPos+1);
	g_strAppDir = str;
	str +="sysinfo.xml";
	sys.SaveXmlFile(str);
	strdat = g_strAppDir;
	strdat +="sysinfo.dat";
	sys.lzmaCompressfile(strdat.c_str(),str.c_str());
		   

	system("pause");
	return 1;

}
