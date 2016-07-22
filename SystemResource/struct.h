
#define SZ_OK 0  

#define SZ_ERROR_DATA 1  
#define SZ_ERROR_MEM 2  
#define SZ_ERROR_CRC 3  
#define SZ_ERROR_UNSUPPORTED 4  
#define SZ_ERROR_PARAM 5  
#define SZ_ERROR_INPUT_EOF 6  
#define SZ_ERROR_OUTPUT_EOF 7  
#define SZ_ERROR_READ 8  
#define SZ_ERROR_WRITE 9  
#define SZ_ERROR_PROGRESS 10  
#define SZ_ERROR_FAIL 11  
#define SZ_ERROR_THREAD 12  

#define SZ_ERROR_ARCHIVE 16  
#define SZ_ERROR_NO_ARCHIVE 17

#define LZMA_PROPS_SIZE 5

#ifdef _UNICODE
#define tstring std::wstring
#else
#define  tstring std::string
#endif
typedef std::map<tstring,tstring> MAPKEYVALUEDATA;


typedef struct _SYSTEM_AUTOSTARTUP_INFO
{
	MAPKEYVALUEDATA mapKeyValueData;
	tstring strRegPath;

}SYSTEM_AUTOSTARTUP_INFO,*PSYSTEM_AUTOSTARTUP_INFO;





typedef unsigned __int64 UInt64;

typedef std::vector<MODULEENTRY32> VCTMOD;
typedef std::vector<THREADENTRY32> VCTTHREAD;
typedef std::vector<DWORD> VCTPID;

typedef std::pair<tstring,tstring> PAIRKEYVALUEDATA;
typedef std::vector<SYSTEM_AUTOSTARTUP_INFO> VCTAUTOSTARTINFO;
typedef std::vector<tstring> VCTSTRING;



