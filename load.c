/* loader.c */
//
//      Notes:
//          Define WIN95 to get Windows95 code otherwise NT code is produced.
//          Under NT the Driver is loaded or unloaded, affecting all devices.
//          Under Windows95 the individual device is loaded or unloaded. use the
//              LoadDriver/UnLoadDriver functions in a loop to start or stop
//              all Windows95 WinRT devices.
//              Windows95 loads are cumulative, i.e. load it twice,
//                 you need to unload it twice.
//
//

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <WinRTctl.h>

#define WIN95

#if defined(WIN95)
    // defines extracted for VXDLDR.H (subject to change)
struct _W32IoctlPkt {
 USHORT W32IO_ErrorCode ;
 USHORT W32IO_DeviceID ;
 UCHAR  W32IO_ModuleName[1] ;
} ;
    // VxDLoader Function Requests
#define VXDLDR_APIFUNC_LOADDEVICE    1
#define VXDLDR_APIFUNC_UNLOADDEVICE  2
    // VxDLoader Error codes
#define VXDLDR_ERR_OUT_OF_MEMORY     1
#define VXDLDR_ERR_IN_DOS            2
#define VXDLDR_ERR_FILE_OPEN_ERROR   3
#define VXDLDR_ERR_FILE_READ         4
#define VXDLDR_ERR_DUPLICATE_DEVICE  5
#define VXDLDR_ERR_BAD_DEVICE_FILE   6
#define VXDLDR_ERR_DEVICE_REFUSED    7
#define VXDLDR_ERR_NO_SUCH_DEVICE    8
#define VXDLDR_ERR_DEVICE_UNLOADABLE 9
#define VXDLDR_ERR_ALLOC_V86_AREA   10
#define VXDLDR_ERR_BAD_API_FUNCTION 11

#endif      // WIN95

#if defined(WIN95)
    // define this structure to reserve space at the end of _W32IoctlPkt struc
    // for 31 more characters
typedef struct {
    struct _W32IoctlPkt Res_DL_Pkt;
    UCHAR               Res_ModName[31];
} IOCTL_PKT;
#endif      // WIN95

    // prototypes
LONG LoadDriver(LONG);


//////////////////////////////////////////////////////////////////////////////
//   LoadDriver() - load driver
//   Inputs:  deviceNumber - device number to load (Windows95 only)
//   Outputs: return() - 0 OK, otherwise exit code
//   Notes: Define WIN95 for Windows95 version
//////////////////////////////////////////////////////////////////////////////
LONG LoadDriver(LONG deviceNumber)
{
#if defined(WIN95)
        // Windows95 Dynanmic VxD Load

    HANDLE  hVxDLdr;    // Handle to VxD Loader
    CHAR    szCVxDName[MAX_PATH];
    CHAR    szTemp[MAX_PATH];
    USHORT  usErrorCode;
    IOCTL_PKT DL_IOCTL_Pkt;
    DWORD   cbBytesReturned;

        // Get file handle to VXDLDR VxD
    hVxDLdr = CreateFile("\\\\.\\VXDLDR", 0,0,0,0,0,0);

    if ( hVxDLdr == INVALID_HANDLE_VALUE )
    {
        printf("Unable to open VXDLDR\n");
        return(-1);
    }

        // Build path & name of VxD. Assumes that VxD is in
        // the system directory VMM32 (e.g. c:\WINDOWS\SYSTEM\VMM32)
    GetSystemDirectory(szCVxDName, MAX_PATH);
    sprintf(szTemp, "\\VMM32\\WRTDEV%d.VXD", deviceNumber);
    lstrcat(szCVxDName, szTemp);

    printf("Dynamically loading %s\n", szCVxDName);

        // call the Vxd Loader to load our Vxd
    DeviceIoControl(hVxDLdr, VXDLDR_APIFUNC_LOADDEVICE,
        (LPVOID)szCVxDName, lstrlen(szCVxDName)+1,
        (LPVOID)&DL_IOCTL_Pkt, sizeof(IOCTL_PKT),
        &cbBytesReturned, NULL);
    usErrorCode = DL_IOCTL_Pkt.Res_DL_Pkt.W32IO_ErrorCode;

        // Done with the Loader
    CloseHandle(hVxDLdr);

        // Check for Errors
    if (usErrorCode != 0)
    {
        printf("Error while dynamically loading %s,\n", szCVxDName);
        switch( usErrorCode )
        {
            case VXDLDR_ERR_OUT_OF_MEMORY:
                printf( "Not enough memory\n" );
                break;
            case VXDLDR_ERR_IN_DOS:
                printf( "Loader could not reenter DOS\n" );
                break;
            case VXDLDR_ERR_FILE_OPEN_ERROR:
                printf( "Could not find device\n" );
                break;
            case VXDLDR_ERR_FILE_READ:
                printf( "Error reading device\n" );
                break;
            case VXDLDR_ERR_DUPLICATE_DEVICE:
                printf( "Device already loaded\n" );
                break;
            case VXDLDR_ERR_BAD_DEVICE_FILE:
                printf( "Not a valid device\n" );
                break;
            case VXDLDR_ERR_DEVICE_REFUSED:
                printf( "Device refuses to load\n" );
                break;
            case VXDLDR_ERR_NO_SUCH_DEVICE:
                printf( "Device not found\n" );
                break;
            default:
                printf( "Error code %d\n", usErrorCode);
        }
        return(-1);
    }

        // Print the VxD's Module Name, which may be different than the filename.
        // (If the module name is NULL, the device was probably loaded at
        //  boot time and is not dynamically load or unloadable.)
    printf("%s Module Name is %s\n\n", szCVxDName,
            DL_IOCTL_Pkt.Res_DL_Pkt.W32IO_ModuleName);

    return(0);
#else   // WIN95

        // NT Service Manager Driver Load
    SC_HANDLE hManager, hService;       // For Service handles
    DWORD error;                        // For error code

       // Open the System Service Manager
    if ( (hManager = OpenSCManager(NULL, NULL,
                    GENERIC_READ|GENERIC_WRITE|GENERIC_EXECUTE)) == NULL )
    {   // failed getting to Service Manager
      switch ((error = GetLastError()))
      {
        case ERROR_ACCESS_DENIED:
            printf("ERROR_ACCESS_DENIED (OM).\n");
            break;
        case ERROR_DATABASE_DOES_NOT_EXIST:
            printf("ERROR_DATABASE_DOES_NOT_EXIST (OM).\n");
            break;
        case ERROR_INVALID_PARAMETER:
            printf("ERROR_INVALID_PARAMETER (OM).\n)");
            break;
        default:
            printf("Open Service Manager error: %d (OM)\n", error);
            break;
      }
      return(-1);
    }

        // Get a handle to the WinRT Driver service
    if ( (hService = OpenService(hManager, "WinRT",
                    GENERIC_READ|GENERIC_WRITE|GENERIC_EXECUTE)) == NULL )
    {   // failed getting to WinRT service
      switch ( (error = GetLastError()) )      // - error
      {
        case ERROR_ACCESS_DENIED:
            printf("ERROR_ACCESS_DENIED (OS)\n");
            break;
        case ERROR_INVALID_HANDLE:
            printf("ERROR_INVALID_HANDLE (OS)\n");
            break;
        case ERROR_INVALID_NAME:
            printf("ERROR_INVALID_NAME (OS)\n");
            break;
        case ERROR_SERVICE_DOES_NOT_EXIST:
            printf("ERROR_SERVICE_DOES_NOT_EXIST (OS)\n");
            break;
        default:
            printf("Service Manager Open error: %d (OS)", error);
            break;
      }
      CloseServiceHandle(hManager);
      return(-1);
    }

    // Start the WinRT driver
    if ( !StartService(hService, 0, NULL) )  // Start the service
    {   // WinRT failed to start
      switch ( (error = GetLastError()) )      // - error
      {
         case ERROR_ACCESS_DENIED:
            printf("ERROR_ACCESS_DENIED (SS)\n");
            break;
         case ERROR_INVALID_HANDLE:
            printf("ERROR_INVALID_HANDLE (SS)\n");
            break;
         case ERROR_PATH_NOT_FOUND:
            printf("ERROR_PATH_NOT_FOUND (SS)\n");
            break;
         case ERROR_SERVICE_ALREADY_RUNNING:
            printf("The WinRT driver is already running.\n");
            break;
         case ERROR_SERVICE_DATABASE_LOCKED:
            printf("ERROR_SERVICE_DATABASE_LOCKED (SS)\n");
            break;
         case ERROR_SERVICE_DEPENDENCY_DELETED:
            printf("ERROR_SERVICE_DEPENDENCY_DELETED (SS)\n");
            break;
         case ERROR_SERVICE_DEPENDENCY_FAIL:
            printf("ERROR_SERVICE_DEPENDENCY_FAIL (SS)\n");
            break;
         case ERROR_SERVICE_DISABLED:
            printf("ERROR_SERVICE_DISABLED (SS)\n");
            break;
         case ERROR_SERVICE_LOGON_FAILED:
            printf("ERROR_SERVICE_LOGON_FAILED (SS)\n");
            break;
         case ERROR_SERVICE_MARKED_FOR_DELETE:
            printf("ERROR_SERVICE_MARKED_FOR_DELETE (SS)\n");
            break;
         case ERROR_SERVICE_NO_THREAD:
            printf("ERROR_SERVICE_NO_THREAD (SS)\n");
            break;
         case ERROR_SERVICE_REQUEST_TIMEOUT:
            printf("ERROR_SERVICE_REQUEST_TIMEOUT (SS)\n");
            break;
         default:
            printf("Error startin WinRT: %d (SS)\n", error);
            break;
      }
      CloseServiceHandle(hManager);    // Close out handles
      CloseServiceHandle(hService);
      return(-1);
    }

    printf("WinRT Driver has been started.\n");
    CloseServiceHandle(hManager);       // Close out handles
    CloseServiceHandle(hService);
    return(0);

#endif   // WIN95
}

