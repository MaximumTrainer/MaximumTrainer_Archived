/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#include "types.h"
#if defined(DSI_TYPES_WINDOWS)


#include "WinDevice.h"

#include "macros.h"

#include <tchar.h>
#include <ctype.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <stdlib.h>


// Prototypes for functions found in the SetupAPI dll.
typedef BOOL (WINAPI* SetupAPI_CallClassInstaller_t)(DI_FUNCTION,HDEVINFO,PSP_DEVINFO_DATA);
typedef BOOL (WINAPI* SetupAPI_EnumDeviceInfo_t)(HDEVINFO,DWORD,PSP_DEVINFO_DATA);
typedef BOOL (WINAPI* SetupAPI_DestroyDeviceInfoList_t)(HDEVINFO);
typedef BOOL (WINAPI* SetupAPI_GetDeviceRegistryProperty_t)(HDEVINFO,PSP_DEVINFO_DATA,DWORD,PDWORD,PBYTE,DWORD,PDWORD);
typedef BOOL (WINAPI* SetupAPI_SetClassInstallParams_t)(HDEVINFO,PSP_DEVINFO_DATA,PSP_CLASSINSTALL_HEADER,DWORD);

#ifdef UNICODE
typedef BOOL (WINAPI* SetupAPI_GetDeviceInstallParams_t)(HDEVINFO,PSP_DEVINFO_DATA,PSP_DEVINSTALL_PARAMS_W);
typedef BOOL (WINAPI* SetupAPI_ClassGuidsFromNameEx_t)(PCWSTR,LPGUID,DWORD,PDWORD,PCWSTR,PVOID);
typedef HDEVINFO (WINAPI* SetupAPI_GetClassDevsEx_t)(CONST GUID*,PCWSTR,HWND,DWORD,HDEVINFO,PCWSTR,PVOID);
typedef HDEVINFO (WINAPI* SetupAPI_CreateDeviceInfoListEx_t)(CONST GUID*,HWND,PCWSTR,PVOID);
typedef BOOL (WINAPI* SetupAPI_OpenDeviceInfo_t)(HDEVINFO,PCWSTR,HWND,DWORD,PSP_DEVINFO_DATA);
typedef BOOL (WINAPI* SetupAPI_GetDeviceInfoListDetail_t)(HDEVINFO,PSP_DEVINFO_LIST_DETAIL_DATA_W);
#else
typedef BOOL (WINAPI* SetupAPI_GetDeviceInstallParams_t)(HDEVINFO,PSP_DEVINFO_DATA,PSP_DEVINSTALL_PARAMS_A);
typedef BOOL (WINAPI* SetupAPI_ClassGuidsFromNameEx_t)(PCSTR,LPGUID,DWORD,PDWORD,PCSTR,PVOID);
typedef HDEVINFO (WINAPI* SetupAPI_GetClassDevsEx_t)(CONST GUID*,PCSTR,HWND,DWORD,HDEVINFO,PCSTR,PVOID);
typedef HDEVINFO (WINAPI* SetupAPI_CreateDeviceInfoListEx_t)(CONST GUID*,HWND,PCSTR,PVOID);
typedef BOOL (WINAPI* SetupAPI_OpenDeviceInfo_t)(HDEVINFO,PCSTR,HWND,DWORD,PSP_DEVINFO_DATA);
typedef BOOL (WINAPI* SetupAPI_GetDeviceInfoListDetail_t)(HDEVINFO,PSP_DEVINFO_LIST_DETAIL_DATA_A);
#endif


// Function pointers for functions found in the SetupAPI dll.
SetupAPI_SetClassInstallParams_t       SetupAPI_SetClassInstallParams;
SetupAPI_CallClassInstaller_t          SetupAPI_CallClassInstaller;
SetupAPI_GetDeviceInstallParams_t      SetupAPI_GetDeviceInstallParams;
SetupAPI_GetDeviceRegistryProperty_t   SetupAPI_GetDeviceRegistryProperty;
SetupAPI_ClassGuidsFromNameEx_t     SetupAPI_ClassGuidsFromNameEx;
SetupAPI_GetClassDevsEx_t           SetupAPI_GetClassDevsEx;
SetupAPI_CreateDeviceInfoListEx_t      SetupAPI_CreateDeviceInfoListEx;
SetupAPI_OpenDeviceInfo_t           SetupAPI_OpenDeviceInfo;
SetupAPI_GetDeviceInfoListDetail_t     SetupAPI_GetDeviceInfoListDetail;
SetupAPI_EnumDeviceInfo_t           SetupAPI_EnumDeviceInfo;
SetupAPI_DestroyDeviceInfoList_t       SetupAPI_DestroyDeviceInfoList;


// Prototypes for functions found in the cfgmgr32 dll.
#ifdef UNICODE
typedef CONFIGRET (WINAPI* CfgMgr32_Get_Device_ID_Ex_t)(DEVINST  dnDevInst, PWCHAR  Buffer, ULONG  BufferLen, ULONG  ulFlags, HMACHINE  hMachine);
#else
typedef CONFIGRET (WINAPI* CfgMgr32_Get_Device_ID_Ex_t)(DEVINST  dnDevInst, PCHAR  Buffer, ULONG  BufferLen, ULONG  ulFlags, HMACHINE  hMachine);
#endif

// Function pointers for functions found in the cfgmgr32 dll.
CfgMgr32_Get_Device_ID_Ex_t            CfgMgr32_Get_Device_ID_Ex;




typedef int (*CallbackFunc)(HDEVINFO Devs,PSP_DEVINFO_DATA DevInfo,LPVOID Context);

int ControlCallback(HDEVINFO Devs,PSP_DEVINFO_DATA DevInfo,LPVOID Context);

//DLL Definitions
BOOL LoadSetupDLLFunctions();
void FreeSetupDLLFunctions();
HMODULE hSetupDll;
HMODULE hCfgMgrDll;

int EnumerateDevices(LPCTSTR Machine,DWORD Flags,int argc,LPTSTR argv[],CallbackFunc Callback,LPVOID Context);
LPTSTR GetDeviceStringProperty(HDEVINFO Devs,PSP_DEVINFO_DATA DevInfo,DWORD Prop);
LPTSTR GetDeviceDescription(HDEVINFO Devs,PSP_DEVINFO_DATA DevInfo);
LPTSTR * GetDevMultiSz(HDEVINFO Devs,PSP_DEVINFO_DATA DevInfo,DWORD Prop);
LPTSTR * GetRegMultiSz(HKEY hKey,LPCTSTR Val);
LPTSTR * GetMultiSzIndexArray(LPTSTR MultiSz);
void DelMultiSz(LPTSTR * Array);
LPTSTR * CopyMultiSz(LPTSTR * Array);


typedef struct
{
    LPCTSTR String;     // string looking for
    LPCTSTR Wild;       // first wild character if any
    BOOL    InstanceId;
} IdEntry;

typedef struct
{
    DWORD count;
    DWORD control;
    BOOL  reboot;
} GenericContext;


typedef struct
{
    int argc_right;
    LPTSTR * argv_right;
    DWORD prop;
    int skipped;
    int modified;
} SetHwidContext;

int WinDevice_Enable(int argc,TCHAR* argv[])
/*++

Routine Description:

    ENABLE <id> ...
    use EnumerateDevices to do hardwareID matching
    for each match, attempt to enable global, and if needed, config specific

Arguments:

    argc/argv - remaining parameters - passed into EnumerateDevices

Return Value:

    EXIT_xxxx (EXIT_REBOOT if reboot is required)

--*/
{
    GenericContext context;
    int failcode = EXIT_FAIL;

   if(!LoadSetupDLLFunctions())
      return EXIT_FAIL;


    if(!argc)
   {
        //
        // arguments required
        //
      FreeSetupDLLFunctions();
        return EXIT_USAGE;
    }

    context.control = DICS_ENABLE; // DICS_PROPCHANGE DICS_ENABLE DICS_DISABLE
    context.reboot = FALSE;
    context.count = 0;

    failcode = EnumerateDevices(NULL,DIGCF_PRESENT,argc,argv,ControlCallback,&context);

    if(failcode == EXIT_OK)
   {
      if((context.count) && (context.reboot))
         failcode = EXIT_REBOOT;
    }

    FreeSetupDLLFunctions();
    return failcode;
}

int WinDevice_Disable(int argc,TCHAR* argv[])
/*++

Routine Description:

    DISABLE <id> ...
    use EnumerateDevices to do hardwareID matching
    for each match, attempt to disable global

Arguments:

    argc/argv - remaining parameters - passed into EnumerateDevices

Return Value:

    EXIT_xxxx (EXIT_REBOOT if reboot is required)

--*/
{
    GenericContext context;
    int failcode = EXIT_FAIL;

   if(!LoadSetupDLLFunctions())
      return EXIT_FAIL;


    if(!argc)
   {
        //
        // arguments required
        //
      FreeSetupDLLFunctions();
        return EXIT_USAGE;
    }

    context.control = DICS_DISABLE; // DICS_PROPCHANGE DICS_ENABLE DICS_DISABLE
    context.reboot = FALSE;
    context.count = 0;

    failcode = EnumerateDevices(NULL,DIGCF_PRESENT,argc,argv,ControlCallback,&context);

    if(failcode == EXIT_OK)
   {
      if((context.count) && (context.reboot))
         failcode = EXIT_REBOOT;
    }

    FreeSetupDLLFunctions();
    return failcode;
}

int WinDevice_Restart(int argc,TCHAR* argv[])
/*++

Routine Description:

    RESTART <id> ...
    use EnumerateDevices to do hardwareID matching
    for each match, attempt to restart by issueing a PROPCHANGE

Arguments:

    argc/argv - remaining parameters - passed into EnumerateDevices

Return Value:

    EXIT_xxxx (EXIT_REBOOT if reboot is required)

--*/
{
    GenericContext context;
    int failcode = EXIT_FAIL;

   if(!LoadSetupDLLFunctions())
      return EXIT_FAIL;


    if(!argc)
   {
        //
        // arguments required
        //
      FreeSetupDLLFunctions();
        return EXIT_USAGE;
    }

    context.control = DICS_PROPCHANGE;
    context.reboot = FALSE;
    context.count = 0;

    failcode = EnumerateDevices(NULL,DIGCF_PRESENT,argc,argv,ControlCallback,&context);

    if(failcode == EXIT_OK)
   {
      if((context.count) && (context.reboot))
         failcode = EXIT_REBOOT;
    }

    FreeSetupDLLFunctions();
    return failcode;
}


int ControlCallback(HDEVINFO Devs,PSP_DEVINFO_DATA DevInfo,LPVOID Context)
/*++

Routine Description:

    Callback for use by Enable/Disable/Restart
    Invokes DIF_PROPERTYCHANGE with correct parameters
    uses SetupDiCallClassInstaller so cannot be done for remote devices
    Don't use CM_xxx API's, they bypass class/co-installers and this is bad.

    In Enable case, we try global first, and if still disabled, enable local

Arguments:

    Devs    )_ uniquely identify the device
    DevInfo )
    Context  - GenericContext

Return Value:

    EXIT_xxxx

--*/
{
    SP_PROPCHANGE_PARAMS pcp;
    GenericContext *pControlContext = (GenericContext*)Context;
    SP_DEVINSTALL_PARAMS devParams;

    switch(pControlContext->control)
   {
        case DICS_ENABLE:
            //
            // enable both on global and config-specific profile
            // do global first and see if that succeeded in enabling the device
            // (global enable doesn't mark reboot required if device is still
            // disabled on current config whereas vice-versa isn't true)
            //
            pcp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
            pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
            pcp.StateChange = pControlContext->control;
            pcp.Scope = DICS_FLAG_GLOBAL;
            pcp.HwProfile = 0;
            //
            // don't worry if this fails, we'll get an error when we try config-
            // specific.
            if(SetupAPI_SetClassInstallParams(Devs,DevInfo,&pcp.ClassInstallHeader,sizeof(pcp)))
         {
               SetupAPI_CallClassInstaller(DIF_PROPERTYCHANGE,Devs,DevInfo);
            }
            //
            // now enable on config-specific
            //
            pcp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
            pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
            pcp.StateChange = pControlContext->control;
            pcp.Scope = DICS_FLAG_CONFIGSPECIFIC;
            pcp.HwProfile = 0;
            break;

        default:
            //
            // operate on config-specific profile
            //
            pcp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
            pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
            pcp.StateChange = pControlContext->control;
            pcp.Scope = DICS_FLAG_CONFIGSPECIFIC;
            pcp.HwProfile = 0;
            break;

    }

    if(!SetupAPI_SetClassInstallParams(Devs,DevInfo,&pcp.ClassInstallHeader,sizeof(pcp)) ||
       !SetupAPI_CallClassInstaller(DIF_PROPERTYCHANGE,Devs,DevInfo))
   {
        //
        // failed to invoke DIF_PROPERTYCHANGE
        //
    }
   else
   {
        //
        // see if device needs reboot
        //
        devParams.cbSize = sizeof(devParams);
        if(SetupAPI_GetDeviceInstallParams(Devs,DevInfo,&devParams) && (devParams.Flags & (DI_NEEDRESTART|DI_NEEDREBOOT)))
      {
                pControlContext->reboot = TRUE;
        }
      else
      {
            //
            // appears to have succeeded
            //
        }
        pControlContext->count++;
    }
    return EXIT_OK;
}

///////////////////////////////////////////////////////////////////////
// Loads Setup API functions from the DLL.
///////////////////////////////////////////////////////////////////////
BOOL LoadSetupDLLFunctions(void)
{
   BOOL bStatus = TRUE;

   FreeSetupDLLFunctions();                                    // Guarantee the library functions are freed.

   char acSystemDirectory[MAX_PATH];
   char asSetupAPI[255];
   char asConfig[255];

   if(GetSystemDirectory(acSystemDirectory, MAX_PATH) == 0)
      return FALSE;

   if(acSystemDirectory == NULL)
      return FALSE;

   SNPRINTF(asSetupAPI, 255, "%s/setupapi.dll", acSystemDirectory);
   SNPRINTF(asConfig, 255, "%s/cfgmgr32.dll", acSystemDirectory);

   hSetupDll = LoadLibrary(asSetupAPI);
   hCfgMgrDll = LoadLibrary(asConfig);

   if(hSetupDll == NULL || hCfgMgrDll == NULL)
   {
      bStatus = FALSE;
   }
   else
   {

      //Functions from SetupAPI DLL.
      SetupAPI_CallClassInstaller = (SetupAPI_CallClassInstaller_t) GetProcAddress(hSetupDll, "SetupDiCallClassInstaller");
      if(SetupAPI_CallClassInstaller == NULL)
         bStatus = FALSE;

      SetupAPI_EnumDeviceInfo = (SetupAPI_EnumDeviceInfo_t) GetProcAddress(hSetupDll, "SetupDiEnumDeviceInfo");
      if(SetupAPI_EnumDeviceInfo == NULL)
         bStatus = FALSE;

      SetupAPI_DestroyDeviceInfoList = (SetupAPI_DestroyDeviceInfoList_t) GetProcAddress(hSetupDll, "SetupDiDestroyDeviceInfoList");
      if(SetupAPI_DestroyDeviceInfoList == NULL)
         bStatus = FALSE;

   #ifdef UNICODE
      SetupAPI_SetClassInstallParams = (SetupAPI_SetClassInstallParams_t) GetProcAddress(hSetupDll, "SetupDiSetClassInstallParamsW");
      if(SetupAPI_SetClassInstallParams == NULL)
         bStatus = FALSE;

      SetupAPI_GetDeviceInstallParams = (SetupAPI_GetDeviceInstallParams_t) GetProcAddress(hSetupDll, "SetupDiGetDeviceInstallParamsW");
      if(SetupAPI_GetDeviceInstallParams == NULL)
         bStatus = FALSE;

      SetupAPI_GetDeviceRegistryProperty = (SetupAPI_GetDeviceRegistryProperty_t) GetProcAddress(hSetupDll, "SetupDiGetDeviceRegistryPropertyW");
      if(SetupAPI_GetDeviceRegistryProperty == NULL)
         bStatus = FALSE;

      SetupAPI_ClassGuidsFromNameEx = (SetupAPI_ClassGuidsFromNameEx_t) GetProcAddress(hSetupDll, "SetupDiClassGuidsFromNameExW");
      if(SetupAPI_ClassGuidsFromNameEx == NULL)
         bStatus = FALSE;

      SetupAPI_GetClassDevsEx = (SetupAPI_GetClassDevsEx_t) GetProcAddress(hSetupDll, "SetupDiGetClassDevsExW");
      if(SetupAPI_GetClassDevsEx == NULL)
         bStatus = FALSE;

      SetupAPI_CreateDeviceInfoListEx = (SetupAPI_CreateDeviceInfoListEx_t) GetProcAddress(hSetupDll, "SetupDiCreateDeviceInfoListExW");
      if(SetupAPI_CreateDeviceInfoListEx == NULL)
         bStatus = FALSE;

      SetupAPI_OpenDeviceInfo = (SetupAPI_OpenDeviceInfo_t) GetProcAddress(hSetupDll, "SetupDiOpenDeviceInfoW");
      if(SetupAPI_OpenDeviceInfo == NULL)
         bStatus = FALSE;

      SetupAPI_GetDeviceInfoListDetail = (SetupAPI_GetDeviceInfoListDetail_t) GetProcAddress(hSetupDll, "SetupDiGetDeviceInfoListDetailW");
      if(SetupAPI_GetDeviceInfoListDetail == NULL)
         bStatus = FALSE;

   #else

      SetupAPI_SetClassInstallParams = (SetupAPI_SetClassInstallParams_t) GetProcAddress(hSetupDll, "SetupDiSetClassInstallParamsA");
      if(SetupAPI_SetClassInstallParams == NULL)
         bStatus = FALSE;

      SetupAPI_GetDeviceInstallParams = (SetupAPI_GetDeviceInstallParams_t) GetProcAddress(hSetupDll, "SetupDiGetDeviceInstallParamsA");
      if(SetupAPI_GetDeviceInstallParams == NULL)
         bStatus = FALSE;

      SetupAPI_GetDeviceRegistryProperty = (SetupAPI_GetDeviceRegistryProperty_t) GetProcAddress(hSetupDll, "SetupDiGetDeviceRegistryPropertyA");
      if(SetupAPI_GetDeviceRegistryProperty == NULL)
         bStatus = FALSE;

      SetupAPI_ClassGuidsFromNameEx = (SetupAPI_ClassGuidsFromNameEx_t) GetProcAddress(hSetupDll, "SetupDiClassGuidsFromNameExA");
      if(SetupAPI_ClassGuidsFromNameEx == NULL)
         bStatus = FALSE;

      SetupAPI_GetClassDevsEx = (SetupAPI_GetClassDevsEx_t) GetProcAddress(hSetupDll, "SetupDiGetClassDevsExA");
      if(SetupAPI_GetClassDevsEx == NULL)
         bStatus = FALSE;

      SetupAPI_CreateDeviceInfoListEx = (SetupAPI_CreateDeviceInfoListEx_t) GetProcAddress(hSetupDll, "SetupDiCreateDeviceInfoListExA");
      if(SetupAPI_CreateDeviceInfoListEx == NULL)
         bStatus = FALSE;

      SetupAPI_OpenDeviceInfo = (SetupAPI_OpenDeviceInfo_t) GetProcAddress(hSetupDll, "SetupDiOpenDeviceInfoA");
      if(SetupAPI_OpenDeviceInfo == NULL)
         bStatus = FALSE;

      SetupAPI_GetDeviceInfoListDetail = (SetupAPI_GetDeviceInfoListDetail_t) GetProcAddress(hSetupDll, "SetupDiGetDeviceInfoListDetailA");
      if(SetupAPI_GetDeviceInfoListDetail == NULL)
         bStatus = FALSE;

   #endif

      //Functions from CfgMgr32 DLL.
   #ifdef UNICODE
      SetupAPI_Get_Device_ID_Ex = (SetupAPI_Get_Device_ID_Ex_t) GetProcAddress(hSetupDll, "CM_Get_Device_ID_ExW");
      if(SetupAPI_Get_Device_ID_Ex == NULL)
         bStatus = FALSE;
   #else
      CfgMgr32_Get_Device_ID_Ex = (CfgMgr32_Get_Device_ID_Ex_t) GetProcAddress(hCfgMgrDll, "CM_Get_Device_ID_ExA");
      if(CfgMgr32_Get_Device_ID_Ex == NULL)
         bStatus = FALSE;
   #endif


   }

   if (!bStatus)
      FreeSetupDLLFunctions();

   return bStatus;
}

///////////////////////////////////////////////////////////////////////
// Unloads Setup API DLL.
///////////////////////////////////////////////////////////////////////
void FreeSetupDLLFunctions(void)
{
   if (hSetupDll != NULL)
   {
      FreeLibrary(hSetupDll);
      hSetupDll = NULL;
   }
   if(hCfgMgrDll != NULL)
   {
      FreeLibrary(hCfgMgrDll);
      hCfgMgrDll = NULL;
   }
}


LPTSTR GetDeviceStringProperty(HDEVINFO Devs,PSP_DEVINFO_DATA DevInfo,DWORD Prop)
/*++

Routine Description:

    Return a string property for a device, otherwise NULL

Arguments:

    Devs    )_ uniquely identify device
    DevInfo )
    Prop     - string property to obtain

Return Value:

    string containing description

--*/
{
    LPTSTR buffer;
    DWORD size;
    DWORD reqSize;
    DWORD dataType;
    DWORD szChars;

    size = 1024; // initial guess
    buffer = new TCHAR[(size/sizeof(TCHAR))+1];
    if(!buffer)
   {
        return NULL;
    }
    while(!SetupAPI_GetDeviceRegistryProperty(Devs,DevInfo,Prop,&dataType,(LPBYTE)buffer,size,&reqSize))
   {
        if(GetLastError() != ERROR_INSUFFICIENT_BUFFER)
      {
            goto failed;
        }
        if(dataType != REG_SZ)
      {
            goto failed;
        }
        size = reqSize;
        delete [] buffer;
        buffer = new TCHAR[(size/sizeof(TCHAR))+1];
        if(!buffer)
      {
            goto failed;
        }
    }
    szChars = reqSize/sizeof(TCHAR);
    buffer[szChars] = TEXT('\0');
    return buffer;

failed:
    if(buffer)
   {
        delete [] buffer;
    }
    return NULL;
}

LPTSTR GetDeviceDescription(HDEVINFO Devs,PSP_DEVINFO_DATA DevInfo)
/*++

Routine Description:

    Return a string containing a description of the device, otherwise NULL
    Always try friendly name first

Arguments:

    Devs    )_ uniquely identify device
    DevInfo )

Return Value:

    string containing description

--*/
{
    LPTSTR desc;
    desc = GetDeviceStringProperty(Devs,DevInfo,SPDRP_FRIENDLYNAME);
    if(!desc)
   {
        desc = GetDeviceStringProperty(Devs,DevInfo,SPDRP_DEVICEDESC);
    }
    return desc;
}

IdEntry GetIdType(LPCTSTR Id)
/*++

Routine Description:

    Determine if this is instance id or hardware id and if there's any wildcards
    instance ID is prefixed by '@'
    wildcards are '*'


Arguments:

    Id - ptr to string to check

Return Value:

    IdEntry

--*/
{
    IdEntry Entry;

    Entry.InstanceId = FALSE;
    Entry.Wild = NULL;
    Entry.String = Id;

    if(Entry.String[0] == INSTANCEID_PREFIX_CHAR)
   {
        Entry.InstanceId = TRUE;
        Entry.String = CharNext(Entry.String);
    }
    if(Entry.String[0] == QUOTE_PREFIX_CHAR)
   {
        //
        // prefix to treat rest of string literally
        //
        Entry.String = CharNext(Entry.String);
    }
   else
   {
        //
        // see if any wild characters exist
        //
        Entry.Wild = _tcschr(Entry.String,WILD_CHAR);
    }
    return Entry;
}

LPTSTR * GetMultiSzIndexArray(LPTSTR MultiSz)
/*++

Routine Description:

    Get an index array pointing to the MultiSz passed in

Arguments:

    MultiSz - well formed multi-sz string

Return Value:

    array of strings. last entry+1 of array contains NULL
    returns NULL on failure

--*/
{
    LPTSTR scan;
    LPTSTR * array;
    int elements;

    for(scan = MultiSz, elements = 0; scan[0] ;elements++)
   {
        scan += lstrlen(scan)+1;
    }
    array = new LPTSTR[elements+2];
    if(!array) {
        return NULL;
    }
    array[0] = MultiSz;
    array++;
    if(elements)
   {
        for(scan = MultiSz, elements = 0; scan[0]; elements++)
      {
            array[elements] = scan;
            scan += lstrlen(scan)+1;
        }
    }
    array[elements] = NULL;
    return array;
}

LPTSTR * CopyMultiSz(LPTSTR * Array)
/*++

Routine Description:

    Creates a new array from old
    old array need not have been allocated by GetMultiSzIndexArray

Arguments:

    Array - array of strings, last entry is NULL

Return Value:

    MultiSz array allocated by GetMultiSzIndexArray

--*/
{
    LPTSTR multiSz = NULL;
    int len = 0;
    int c;
    if(Array)
   {
        for(c=0;Array[c];c++)
      {
            len+=lstrlen(Array[c])+1;
        }
    }
    len+=1; // final Null
    multiSz = new TCHAR[len];
    if(!multiSz)
   {
        return NULL;
    }
    len = 0;
    if(Array)
   {
        for(c=0;Array[c];c++)
      {
            lstrcpy(multiSz+len,Array[c]);
            len+=lstrlen(multiSz+len)+1;
        }
    }
    multiSz[len] = TEXT('\0');
    LPTSTR * pRes = GetMultiSzIndexArray(multiSz);
    if(pRes)
   {
        return pRes;
    }
    delete [] multiSz;
    return NULL;
}

void DelMultiSz(LPTSTR * Array)
/*++

Routine Description:

    Deletes the string array allocated by GetDevMultiSz/GetRegMultiSz/GetMultiSzIndexArray

Arguments:

    Array - pointer returned by GetMultiSzIndexArray

Return Value:

    None

--*/
{
    if(Array)
   {
        Array--;
        if(Array[0])
      {
            delete [] Array[0];
        }
        delete [] Array;
    }
}

LPTSTR * GetDevMultiSz(HDEVINFO Devs,PSP_DEVINFO_DATA DevInfo,DWORD Prop)
/*++

Routine Description:

    Get a multi-sz device property
    and return as an array of strings

Arguments:

    Devs    - HDEVINFO containing DevInfo
    DevInfo - Specific device
    Prop    - SPDRP_HARDWAREID or SPDRP_COMPATIBLEIDS

Return Value:

    array of strings. last entry+1 of array contains NULL
    returns NULL on failure

--*/
{
    LPTSTR buffer;
    DWORD size;
    DWORD reqSize;
    DWORD dataType;
    LPTSTR * array;
    DWORD szChars;

    size = 8192; // initial guess, nothing magic about this
    buffer = new TCHAR[(size/sizeof(TCHAR))+2];
    if(!buffer)
   {
        return NULL;
    }
    while(!SetupAPI_GetDeviceRegistryProperty(Devs,DevInfo,Prop,&dataType,(LPBYTE)buffer,size,&reqSize))
   {
        if(GetLastError() != ERROR_INSUFFICIENT_BUFFER)
      {
            goto failed;
        }
        if(dataType != REG_MULTI_SZ)
      {
            goto failed;
        }
        size = reqSize;
        delete [] buffer;
        buffer = new TCHAR[(size/sizeof(TCHAR))+2];
        if(!buffer)
      {
            goto failed;
        }
    }
    szChars = reqSize/sizeof(TCHAR);
    buffer[szChars] = TEXT('\0');
    buffer[szChars+1] = TEXT('\0');
    array = GetMultiSzIndexArray(buffer);
    if(array)
   {
        return array;
    }

failed:
    if(buffer)
   {
        delete [] buffer;
    }
    return NULL;
}

LPTSTR * GetRegMultiSz(HKEY hKey,LPCTSTR Val)
/*++

Routine Description:

    Get a multi-sz from registry
    and return as an array of strings

Arguments:

    hKey    - Registry Key
    Val     - Value to query

Return Value:

    array of strings. last entry+1 of array contains NULL
    returns NULL on failure

--*/
{
    LPTSTR buffer;
    DWORD size;
    DWORD reqSize;
    DWORD dataType;
    LPTSTR * array;
    DWORD szChars;

    size = 8192; // initial guess, nothing magic about this
    buffer = new TCHAR[(size/sizeof(TCHAR))+2];
    if(!buffer)
   {
        return NULL;
    }
    reqSize = size;
    while(RegQueryValueEx(hKey,Val,NULL,&dataType,(PBYTE)buffer,&reqSize) != NO_ERROR)
   {
        if(GetLastError() != ERROR_MORE_DATA)
      {
            goto failed;
        }
        if(dataType != REG_MULTI_SZ)
      {
            goto failed;
        }
        size = reqSize;
        delete [] buffer;
        buffer = new TCHAR[(size/sizeof(TCHAR))+2];
        if(!buffer)
      {
            goto failed;
        }
    }
    szChars = reqSize/sizeof(TCHAR);
    buffer[szChars] = TEXT('\0');
    buffer[szChars+1] = TEXT('\0');

    array = GetMultiSzIndexArray(buffer);
    if(array)
   {
        return array;
    }

failed:
    if(buffer)
   {
        delete [] buffer;
    }
    return NULL;
}

BOOL WildCardMatch(LPCTSTR Item,const IdEntry & MatchEntry)
/*++

Routine Description:

    Compare a single item against wildcard
    I'm sure there's better ways of implementing this
    Other than a command-line management tools
    it's a bad idea to use wildcards as it implies
    assumptions about the hardware/instance ID
    eg, it might be tempting to enumerate root\* to
    find all root devices, however there is a CfgMgr
    API to query status and determine if a device is
    root enumerated, which doesn't rely on implementation
    details.

Arguments:

    Item - item to find match for eg a\abcd\c
    MatchEntry - eg *\*bc*\*

Return Value:

    TRUE if any match, otherwise FALSE

--*/
{
    LPCTSTR scanItem;
    LPCTSTR wildMark;
    LPCTSTR nextWild;
    size_t matchlen;

    //
    // before attempting anything else
    // try and compare everything up to first wild
    //
    if(!MatchEntry.Wild)
   {
        return _tcsicmp(Item,MatchEntry.String) ? FALSE : TRUE;
    }
    if(_tcsnicmp(Item,MatchEntry.String,MatchEntry.Wild-MatchEntry.String) != 0)
   {
        return FALSE;
    }
    wildMark = MatchEntry.Wild;
    scanItem = Item + (MatchEntry.Wild-MatchEntry.String);

    for(;wildMark[0];)
   {
        //
        // if we get here, we're either at or past a wildcard
        //
        if(wildMark[0] == WILD_CHAR)
      {
            //
            // so skip wild chars
            //
            wildMark = CharNext(wildMark);
            continue;
        }
        //
        // find next wild-card
        //
        nextWild = _tcschr(wildMark,WILD_CHAR);
        if(nextWild)
      {
            //
            // substring
            //
            matchlen = nextWild-wildMark;
        }
      else
      {
            //
            // last portion of match
            //
            size_t scanlen = lstrlen(scanItem);
            matchlen = lstrlen(wildMark);
            if(scanlen < matchlen)
         {
                return FALSE;
            }
            return _tcsicmp(scanItem+scanlen-matchlen,wildMark) ? FALSE : TRUE;
        }
        if(_istalpha(wildMark[0]))
      {
            //
            // scan for either lower or uppercase version of first character
            //
            TCHAR u = (TCHAR)_totupper(wildMark[0]);
            TCHAR l = (TCHAR)_totlower(wildMark[0]);
            while(scanItem[0] && scanItem[0]!=u && scanItem[0]!=l)
         {
                scanItem = CharNext(scanItem);
            }
            if(!scanItem[0])
         {
                //
                // ran out of string
                //
                return FALSE;
            }
        }
      else
      {
            //
            // scan for first character (no case)
            //
            scanItem = _tcschr(scanItem,wildMark[0]);
            if(!scanItem)
         {
                //
                // ran out of string
                //
                return FALSE;
            }
        }
        //
        // try and match the sub-string at wildMark against scanItem
        //
        if(_tcsnicmp(scanItem,wildMark,matchlen)!=0)
      {
            //
            // nope, try again
            //
            scanItem = CharNext(scanItem);
            continue;
        }
        //
        // substring matched
        //
        scanItem += matchlen;
        wildMark += matchlen;
    }
    return (wildMark[0] ? FALSE : TRUE);
}

BOOL WildCompareHwIds(LPTSTR * Array,const IdEntry & MatchEntry)
/*++

Routine Description:

    Compares all strings in Array against Id
    Use WildCardMatch to do real compare

Arguments:

    Array - pointer returned by GetDevMultiSz
    MatchEntry - string to compare against

Return Value:

    TRUE if any match, otherwise FALSE

--*/
{
    if(Array)
   {
        while(Array[0])
      {
            if(WildCardMatch(Array[0],MatchEntry))
         {
                return TRUE;
            }
            Array++;
        }
    }
    return FALSE;
}



int EnumerateDevices(LPCTSTR Machine,DWORD Flags,int argc,LPTSTR argv[],CallbackFunc Callback,LPVOID Context)
/*++

Routine Description:

    Generic enumerator for devices that will be passed the following arguments:
    <id> [<id>...]
    =<class> [<id>...]
    where <id> can either be @instance-id, or hardware-id and may contain wildcards
    <class> is a class name

Arguments:

    Machine  - name of machine to enumerate
    Flags    - extra enumeration flags (eg DIGCF_PRESENT)
    argc/argv - remaining arguments on command line
    Callback - function to call for each hit
    Context  - data to pass function for each hit

Return Value:

    EXIT_xxxx

--*/
{
    HDEVINFO devs = INVALID_HANDLE_VALUE;
    IdEntry * templ = NULL;
    int failcode = EXIT_FAIL;
    int retcode;
    int argIndex;
    DWORD devIndex;
    SP_DEVINFO_DATA devInfo;
    SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail;
    BOOL doSearch = FALSE;
    BOOL match;
    BOOL all = FALSE;
    GUID cls;
    DWORD numClass = 0;
    int skip = 0;


    if(!argc)
   {
        return EXIT_USAGE;
    }

    templ = new IdEntry[argc];
    if(!templ)
   {
        goto final;
    }

    //
    // determine if a class is specified
    //
    if(argc>skip && argv[skip][0]==CLASS_PREFIX_CHAR && argv[skip][1])
   {
        if(!SetupAPI_ClassGuidsFromNameEx(argv[skip]+1,&cls,1,&numClass,Machine,NULL) &&
            GetLastError() != ERROR_INSUFFICIENT_BUFFER)
      {
            goto final;
        }
        if(!numClass)
      {
            failcode = EXIT_OK;
            goto final;
        }
        skip++;
    }
    if(argc>skip && argv[skip][0]==WILD_CHAR && !argv[skip][1])
   {
        //
        // catch convinient case of specifying a single argument '*'
        //
        all = TRUE;
        skip++;
    }
   else if(argc<=skip)
   {
        //
        // at least one parameter, but no <id>'s
        //
        all = TRUE;
    }

    //
    // determine if any instance id's were specified
    //
    // note, if =<class> was specified with no id's
    // we'll mark it as not doSearch
    // but will go ahead and add them all
    //
    for(argIndex=skip;argIndex<argc;argIndex++)
   {
        templ[argIndex] = GetIdType(argv[argIndex]);
        if(templ[argIndex].Wild || !templ[argIndex].InstanceId)
      {
            //
            // anything other than simple InstanceId's require a search
            //
            doSearch = TRUE;
        }
    }
    if(doSearch || all)
   {
        //
        // add all id's to list
        // if there's a class, filter on specified class
        //
        devs = SetupAPI_GetClassDevsEx(numClass ? &cls : NULL,
                                     NULL,
                                     NULL,
                                     (numClass ? 0 : DIGCF_ALLCLASSES) | Flags,
                                     NULL,
                                     Machine,
                                     NULL);

    }
   else
   {
        //
        // blank list, we'll add instance id's by hand
        //
        devs = SetupAPI_CreateDeviceInfoListEx(numClass ? &cls : NULL,
                                             NULL,
                                             Machine,
                                             NULL);
    }
    if(devs == INVALID_HANDLE_VALUE)
   {
        goto final;
    }
    for(argIndex=skip;argIndex<argc;argIndex++)
   {
        //
        // add explicit instances to list (even if enumerated all,
        // this gets around DIGCF_PRESENT)
        // do this even if wildcards appear to be detected since they
        // might actually be part of the instance ID of a non-present device
        //
        if(templ[argIndex].InstanceId)
      {
            SetupAPI_OpenDeviceInfo(devs,templ[argIndex].String,NULL,0,NULL);
        }
    }

    devInfoListDetail.cbSize = sizeof(devInfoListDetail);
    if(!SetupAPI_GetDeviceInfoListDetail(devs,&devInfoListDetail))
   {
        goto final;
    }

    //
    // now enumerate them
    //
    if(all)
   {
        doSearch = FALSE;
    }

    devInfo.cbSize = sizeof(devInfo);
    for(devIndex=0;SetupAPI_EnumDeviceInfo(devs,devIndex,&devInfo);devIndex++)
   {

        if(doSearch)
      {
            for(argIndex=skip,match=FALSE;(argIndex<argc) && !match;argIndex++)
         {
                TCHAR devID[MAX_DEVICE_ID_LEN];
                LPTSTR *hwIds = NULL;
                LPTSTR *compatIds = NULL;
                //
                // determine instance ID
                //
                if(CfgMgr32_Get_Device_ID_Ex(devInfo.DevInst,devID,MAX_DEVICE_ID_LEN,0,devInfoListDetail.RemoteMachineHandle)!=CR_SUCCESS)
            {
                    devID[0] = TEXT('\0');
                }

                if(templ[argIndex].InstanceId)
            {
                    //
                    // match on the instance ID
                    //
                    if(WildCardMatch(devID,templ[argIndex]))
               {
                        match = TRUE;
                    }
                }
            else
            {
                    //
                    // determine hardware ID's
                    // and search for matches
                    //
                    hwIds = GetDevMultiSz(devs,&devInfo,SPDRP_HARDWAREID);
                    compatIds = GetDevMultiSz(devs,&devInfo,SPDRP_COMPATIBLEIDS);

                    if(WildCompareHwIds(hwIds,templ[argIndex]) ||
                        WildCompareHwIds(compatIds,templ[argIndex]))
               {
                        match = TRUE;
                    }
                }
                DelMultiSz(hwIds);
                DelMultiSz(compatIds);
            }
        }
      else
      {
            match = TRUE;
        }
        if(match)
      {
            retcode = Callback(devs,&devInfo,Context);
            if(retcode)
         {
                failcode = retcode;
                goto final;
            }
        }
    }

    failcode = EXIT_OK;

final:
    if(templ)
   {
        delete [] templ;
    }
    if(devs != INVALID_HANDLE_VALUE)
   {
        SetupAPI_DestroyDeviceInfoList(devs);
    }
    return failcode;

}


#endif //defined(DSI_TYPES_WINDOWS)