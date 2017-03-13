#ifndef __IBBS_2
#define __IBBS_2

#ifdef WIN32
#define _MSC_VER 1
#endif // WIN32

typedef enum
   {
   eSuccess,
   eForwarded,
   eNoMoreMessages,
   eGeneralFailure,
   eBadParameter,
   eNoMemory,
   eMissingDir,
   eFileOpenError
   } tIBResult;

#ifndef tBool
typedef int tBool;
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#define SYSTEM_NAME_CHARS 39
#define FILENAME "GALACTIC"
#ifdef _MSC_VER
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif
#define VERSION "00001"

typedef struct {
    char filebox[PATH_MAX + 1];
    char name[SYSTEM_NAME_CHARS + 1];
} tOtherNode;

typedef struct {
    tOtherNode *myNode;
    tOtherNode **otherNodes;
    int otherNodeCount;
} tIBInfo;

tIBResult IBSend(tIBInfo *pInfo, char *pszDestNode, void *pBuffer, uint32_t nBufferSize);
tIBResult IBSendAll(tIBInfo *pInfo, void *pBuffer, uint32_t nBufferSize);
tIBResult IBGet(tIBInfo *pInfo, void *pBuffer, uint32_t nMaxBufferSize);
tIBResult IBReadConfig(tIBInfo *pInfo, char *pszConfigFile);

#endif
