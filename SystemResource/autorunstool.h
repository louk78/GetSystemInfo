#ifndef _AUTO_RUN_S_TOOL_H_
#define _AUTO_RUN_S_TOOL_H_
#include "registrytool.h"


class CAutoRunsTool
{
public:
	CAutoRunsTool();
	virtual ~CAutoRunsTool();

public:
	BOOL GetRegRun(VCTAUTOSTARTINFO &vctAutoStartInfo);
	BOOL GetServicesDrives();

private:
	CRegistryTool m_RegistryTool;

};





#endif  