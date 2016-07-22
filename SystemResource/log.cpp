#include "stdafx.h"
#include "log.h"
#include "atltime.h"
tstring LOG::logFilePath = _T("");
extern tstring g_strAppDir;

#define LOGTOFILE

void LOG::show(const TCHAR *format,... )
{
	const TCHAR *pStr;
	va_list  valist;
	tstring strBuf;

	va_start(valist,format);

	pStr= format;
	TCHAR ch = *pStr;
	while(ch)
	{
		if(ch=='%')
		{
			ch = *(++pStr);
			if(ch==0)
			{
				break;
			}
			if(ch=='d'||ch=='D')
			{
				TCHAR buf[100]={0};
				_stprintf_s(buf,sizeof(buf),"%d",va_arg(valist,int));
				strBuf += buf;
			}
			else if(ch=='s'||ch=='S')
			{
				strBuf += va_arg(valist,TCHAR*);
			}
			else if(ch=='f'||ch=='F')
			{
				TCHAR buf[100]={0};
				_stprintf_s(buf,sizeof(buf),"%f",va_arg(valist,float));
				strBuf += buf;
			}
			else if((*pStr>='0'&&*pStr<='9')&&(*(pStr+1)>='0'&&*(pStr+1)<='9')
				&&(*(pStr+2)=='x'||*(pStr+2)=='X'))
			{
				TCHAR buf[10]={0};
				tstring control;
				control = "%";
				control.append(pStr,3);
				_stprintf_s(buf,sizeof(buf),control.c_str(),va_arg(valist,int));
				strBuf += buf;
				pStr+=3;
			}
		}
		else
		{
			strBuf +=ch;
		}

			ch = *(++pStr);
	}
	tstring strLog;
	tstring strTime;
	GetCurTime(strTime);
	strLog = strTime;
	strLog += strBuf;
	strLog += "\n";

#ifdef LOGTOFILE
	LogToFile(strLog);
#else
	LogToPrint(strLog);
#endif
}


BOOL LOG::LogToFile(tstring strLog)
{
	FILE *file;
	logFilePath = g_strAppDir;
	logFilePath += _T("log.log");
	_tfopen_s(&file,logFilePath.c_str(),_T("ab+"));

	if(!file)
	{
		return FALSE;
	}
	fwrite(strLog.c_str(),1,strLog.size(),file);
	fclose(file);
	return TRUE;

}

void LOG::LogToPrint(tstring strLog)
{
	_tprintf("%s",strLog.c_str());
}

BOOL LOG::GetCurTime(tstring &strTime)
{
	TCHAR buf[100]={0};
	CTime time;
	time = CTime::GetCurrentTime();
	strTime =_T("");

	_stprintf_s(buf,sizeof(buf),_T("[%d/%d/%d-%d:%d:%d]"),
		time.GetYear(),time.GetMonth(),time.GetDay(),
					time.GetHour(),time.GetMinute(),time.GetSecond());

	strTime = buf;
	return TRUE;
}

void LOG::printError(TCHAR* msg)
{
	DWORD eNum;
	TCHAR sysMsg[256];
	TCHAR* p;
	eNum = GetLastError();
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,eNum,
		MAKELANGID(LANG_NEUTRAL,SUBLANG_ENGLISH_AUS),
		sysMsg,256,NULL);

	p = sysMsg;
	while((*p >31) || (*p == 9))
		++p;

	do
	{
		*p-- = 0;
	}while((p >= sysMsg)&&((*p == '.') || (*p < 33)));

	show(TEXT("WARING: %s failed with error %d (%s)"),msg,eNum,sysMsg);
}