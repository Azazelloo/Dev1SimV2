
/****************************************************************************/
/*      TmkInit v4.08 for Windows. ELCUS, 1995,2011.                        */
/****************************************************************************/

#include <stdio.h>
#include <string.h>

#define _TMK1553B_MRT
#include "wdmtmkv2.cpp"

#ifdef TMK_CONFIGURATION_TABLE
TTmkConfigData aTmkConfig[MAX_TMK_NUMBER+1];
#endif

#define TMK_FILE_OPEN_ERROR 21
#define TMK_FILE_READ_ERROR 22
#define TMK_FILE_FORMAT_ERROR 23
#define TMK_UNKNOWN_TYPE 24

int TmkInit(char *pszTMKFileName)
{
 int nResult;
 int hTMK;
 char achParams[81];
 FILE *hTMKFile;

#ifdef TMK_CONFIGURATION_TABLE
 for (hTMK = 0; hTMK <= MAX_TMK_NUMBER; hTMK++)
 {
  aTmkConfig[hTMK].nType = -1;
  aTmkConfig[hTMK].szName[0] = '\0';
  aTmkConfig[hTMK].wPorts1 = aTmkConfig[hTMK].wPorts2 = 0;
  aTmkConfig[hTMK].wIrq1 = aTmkConfig[hTMK].wIrq2 = 0;
 }
#endif
// if (TmkOpen())
//  return TMK_FILE_OPEN_ERROR;
#ifdef _WIN64
 if (fopen_s(&hTMKFile, pszTMKFileName, "r") != 0)
#else
 if ((hTMKFile = fopen(pszTMKFileName, "r")) == NULL)
#endif
  return TMK_FILE_OPEN_ERROR;
 nResult = TMK_FILE_READ_ERROR;
 while (1)
 {
  if (fgets(achParams, 80, hTMKFile) == NULL)
  {
   if (feof(hTMKFile))
    break;
   else
   {
    nResult = TMK_FILE_READ_ERROR;
    goto ExitTmkInit;
   }
  }
  if (achParams[0] == '*')
   break;
#ifdef _WIN64
 if (sscanf_s(achParams, "%u", &hTMK) != 1)
#else
 if (sscanf(achParams, "%u", &hTMK) != 1)
#endif
   continue;
  if (hTMK > tmkgetmaxn())
  {
   nResult = TMK_FILE_FORMAT_ERROR;
   goto ExitTmkInit;
  }
  nResult = tmkconfig(hTMK);
#ifdef TMK_CONFIGURATION_TABLE
  tmkgetinfo(&(aTmkConfig[hTMK]));
#endif
  if (nResult)
   break;
 } /* endwhile(!feof()) */
 ExitTmkInit:
 fclose(hTMKFile);
 return nResult;
}