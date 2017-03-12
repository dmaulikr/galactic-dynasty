#include <sys/stat.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <unistd.h>

#include "interbbs2.h"

#define NUM_KEYWORDS 4

char *apszKeyWord[NUM_KEYWORDS] = {"SystemName",
                                   "FileInbox",
                                   "LinkName",
                                   "LinkFileOutbox"};

tIBResult ProcessFile(tIBInfo *pInfo, char *filename, void *pBuffer, int nBufferSize) {
    char version[6];
    FILE *fptr;
    memset(version, 0, 6);
    unsigned char destlen;
    char *destination;
    uint32_t memsize;

    fptr = fopen(filename, "rb");

    fread(version, 5, 1, fptr);

    if (strncmp(version, VERSION, 5) != 0) {
        return eBadParameter;
    }

    fread(&destlen, sizeof(unsigned char), 1, fptr);

    destination = (char *)malloc(destlen + 1);
    if (!destination) {
        fclose(fptr);
        return eNoMemory;
    }
    memset(destination, 0, destlen + 1);

    fread(destination, 1, destlen, fptr);

    if (strcasecmp(destination, pInfo->myNode->name) != 0) {
        free(destination);
        fclose(fptr);
        return eBadParameter;
    }
    fread(&memsize, sizeof(uint32_t), 1, fptr);
    memsize = ntohl(memsize);

    if (nBufferSize < memsize) {
        free(destination);
        fclose(fptr);        
        return eBadParameter;
    } 
    fread(pBuffer, memsize, 1, fptr);

    fclose(fptr);
    free(destination);
    unlink(filename);

    return eSuccess;
}

tIBResult IBGet(tIBInfo *pInfo, void *pBuffer, uint32_t nBufferSize) {
    DIR *dirp;
    struct dirent *dp;
    char filename[PATH_MAX];

    dirp = opendir(pInfo->myNode->filebox);
    if (!dirp) {
        return eMissingDir;
    }

    while ((dp = readdir(dirp)) != NULL) {
        if (strncmp(dp->d_name, "GALACTIC", 8) == 0) {
            snprintf(filename, PATH_MAX, "%s%s%s", pInfo->myNode->filebox, PATH_SEP, dp->d_name);
            closedir(dirp);
            if (ProcessFile(pInfo, filename, pBuffer, nBufferSize) == eSuccess) {
                return eSuccess;
            }
        }
    }

    closedir(dirp);
    return eNoMoreMessages;
}

tIBResult IBSendAll(tIBInfo *pInfo, void *pBuffer, uint32_t nBufferSize) {
    int i;
    int result;

    for (i=0;i<pInfo->otherNodeCount;i++) {
        result = IBSend(pInfo, pInfo->otherNodes[i]->name, pBuffer, nBufferSize);
        if (result != eSuccess) {
            return result;
        }
    }

    return eSuccess;
}

tIBResult IBSend(tIBInfo *pInfo, char *pszDestNode, void *pBuffer, uint32_t nBufferSize) {
    struct stat s;
    int i;
    char filename[PATH_MAX];
    tOtherNode *dest = NULL;
    FILE *fptr;

    for (i=0;i<pInfo->otherNodeCount;i++) {
        if (!strcmp(pInfo->otherNodes[i]->name, pszDestNode)) {
            dest = pInfo->otherNodes[i];
            break;
        }
    }

    if (dest == NULL) {
        return eBadParameter;
    }

    i = -1;

    do {  
        i++;      
        snprintf(filename, PATH_MAX, "%s%s%s.%03d", dest->filebox, PATH_SEP, FILENAME, i);
        if (i==999) {
            return eBadParameter;
        }
    } while (!stat(filename, &s));

    fptr = fopen(filename, "wb");
    if (!fptr) {
        return eFileOpenError;
    }

    unsigned char name_size = strlen(pInfo->myNode->name);
    uint32_t nwNbufferSize = htonl(nBufferSize);
    fwrite(VERSION, 5, 1, fptr);
    fwrite(&name_size, sizeof(unsigned char), 1, fptr);
    fwrite(pInfo->myNode->name, name_size, 1, fptr);
    fwrite(&nwNbufferSize, sizeof(uint32_t), 1, fptr);
    fwrite(pBuffer, nBufferSize, 1, fptr);
    fclose(fptr);

    return eSuccess;
}



void ProcessConfigLine(int nKeyword, char *pszParameter, void *pCallbackData)
   {
   tIBInfo *pInfo = (tIBInfo *)pCallbackData;
   tOtherNode **paNewNodeArray;

   switch(nKeyword)
      {
      case 0:
         strncpy(pInfo->myNode->name, pszParameter, SYSTEM_NAME_CHARS);
         pInfo->myNode->name[SYSTEM_NAME_CHARS] = '\0';
         break;

      case 1:
         strncpy(pInfo->myNode->filebox, pszParameter, PATH_MAX);
         pInfo->myNode->filebox[PATH_MAX] = '\0';
         break;

      case 2:
         if(pInfo->otherNodeCount == 0)
            {
            pInfo->otherNodes = (tOtherNode **)malloc(sizeof(tOtherNode *));
            if(pInfo->otherNodes == NULL)
               {
               break;
               }
            }
         else
            {
            if((paNewNodeArray = (tOtherNode **)realloc(pInfo->otherNodes, sizeof(tOtherNode *) * (pInfo->otherNodeCount + 1))) == NULL)
               {
               break;
               } else {
                   pInfo->otherNodes = paNewNodeArray;
               }
            }
         pInfo->otherNodes[pInfo->otherNodeCount] = (tOtherNode *)malloc(sizeof(tOtherNode));
         if (!pInfo->otherNodes[pInfo->otherNodeCount]) {
             break;
         }
         strncpy(pInfo->otherNodes[pInfo->otherNodeCount]->name, pszParameter, SYSTEM_NAME_CHARS);
         pInfo->otherNodes[pInfo->otherNodeCount]->name[SYSTEM_NAME_CHARS] = '\0';
         ++pInfo->otherNodeCount;
         break;

      case 3:
         if(pInfo->otherNodeCount != 0)
            {
            strncpy(pInfo->otherNodes[pInfo->otherNodeCount - 1]->filebox, pszParameter, PATH_MAX);
            pInfo->otherNodes[pInfo->otherNodeCount - 1]->filebox[PATH_MAX] = '\0';
            }
         break;
      }
   }


/* Configuration file reader settings */
#define CONFIG_LINE_SIZE 128
#define MAX_TOKEN_CHARS 32

tBool ProcessConfigFile(char *pszFileName, int nKeyWords, char **papszKeyWord,
                  void (*pfCallBack)(int, char *, void *), void *pCallBackData)
   {
   FILE *pfConfigFile;
   char szConfigLine[CONFIG_LINE_SIZE + 1];
   char *pcCurrentPos;
   unsigned int uCount;
   char szToken[MAX_TOKEN_CHARS + 1];
   int iKeyWord;
   
   /* Attempt to open configuration file */
   if((pfConfigFile = fopen(pszFileName, "rt")) == NULL)
      {
      return(FALSE);
      }

   /* While not at end of file */
   while(!feof(pfConfigFile))
      {
      /* Get the next line */
      if(fgets(szConfigLine, CONFIG_LINE_SIZE + 1 ,pfConfigFile) == NULL) break;

      /* Ignore all of line after comments or CR/LF char */
      pcCurrentPos=(char *)szConfigLine;
      while(*pcCurrentPos)
         {
         if(*pcCurrentPos=='\n' || *pcCurrentPos=='\r' || *pcCurrentPos==';')
            {
            *pcCurrentPos='\0';
            break;
            }
         ++pcCurrentPos;
         }

      /* Search for beginning of first token on line */
      pcCurrentPos=(char *)szConfigLine;
      while(*pcCurrentPos && isspace(*pcCurrentPos)) ++pcCurrentPos;

      /* If no token was found, proceed to process the next line */
      if(!*pcCurrentPos) continue;

      /* Get first token from line */
      uCount=0;
      while(*pcCurrentPos && !isspace(*pcCurrentPos))
         {
         if(uCount<MAX_TOKEN_CHARS) szToken[uCount++]=*pcCurrentPos;
         ++pcCurrentPos;
         }
      if(uCount<=MAX_TOKEN_CHARS)
         szToken[uCount]='\0';
      else
         szToken[MAX_TOKEN_CHARS]='\0';

      /* Find beginning of configuration option parameters */
      while(*pcCurrentPos && isspace(*pcCurrentPos)) ++pcCurrentPos;

       /* Trim trailing spaces from setting string */
      for(uCount=strlen(pcCurrentPos)-1;uCount>0;--uCount)
         {
         if(isspace(pcCurrentPos[uCount]))
            {
            pcCurrentPos[uCount]='\0';
            }
         else
            {
            break;
            }
         }

      /* Loop through list of keywords */
      for(iKeyWord = 0; iKeyWord < nKeyWords; ++iKeyWord)
         {
         /* If keyword matches */
         if(strcasecmp(szToken, papszKeyWord[iKeyWord]) == 0)
            {
            /* Call keyword processing callback function */
            (*pfCallBack)(iKeyWord, pcCurrentPos, pCallBackData);
            }
         }
      }

   /* Close the configuration file */
   fclose(pfConfigFile);

   /* Return with success */
   return(TRUE);
   }

   tIBResult IBReadConfig(tIBInfo *pInfo, char *pszConfigFile)
   {
   /* Set default values for pInfo settings */
   pInfo->otherNodeCount = 0;
   pInfo->otherNodes = NULL;
   pInfo->myNode = (tOtherNode *)malloc(sizeof(tOtherNode));
   if (!pInfo->myNode) {
       return eNoMemory;
   }
   /* Process configuration file */
   if(!ProcessConfigFile(pszConfigFile, NUM_KEYWORDS, apszKeyWord,
                         ProcessConfigLine, (void *)pInfo))
      {
      return(eFileOpenError);
      }

   /* else */
   return(eSuccess);
   }