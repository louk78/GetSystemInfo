#ifndef _LOG_H_
#define _LOG_H_

class LOG
{

public:
	static void show(const TCHAR *,... );
	static void printError(TCHAR* msg);

private:
	static BOOL LogToFile(tstring strLog);
	static void LogToPrint(tstring strLog);
	static BOOL GetCurTime(tstring &strTime);
	
private:
	static tstring logFilePath;

};
#endif