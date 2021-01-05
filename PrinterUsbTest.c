/* Code to find the device path for a usbprint.sys controlled
 * usb printer and print to it
 */

#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <usb.h>
#include <usbiodef.h>
#include <usbioctl.h>
#include <usbprint.h>
#include <setupapi.h>
#include <devguid.h>
#include <wdmguid.h>
#include <stdio.h>


 /* This define is required so that the GUID_DEVINTERFACE_USBPRINT variable is
  * declared an initialised as a static locally, since windows does not include it in any
  * of its libraries
  */

#define SS_DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
static const GUID name \
= { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

SS_DEFINE_GUID(GUID_DEVINTERFACE_USBPRINT, 0x28d78fad, 0x5a12, 0x11D1, 0xae, 0x5b, 0x00, 0x00, 0xf8, 0x03, 0xa8, 0xc2);

void SomeFunctionToWriteToUSB(char *param)
{
    HDEVINFO devs;
    DWORD devcount;
    SP_DEVINFO_DATA devinfo;
    SP_DEVICE_INTERFACE_DATA devinterface;
    DWORD size;
    GUID intfce;
    PSP_DEVICE_INTERFACE_DETAIL_DATA interface_detail;
    HANDLE usbHandle;
    DWORD dataType;

    intfce = GUID_DEVINTERFACE_USBPRINT;
    devs = SetupDiGetClassDevs(&intfce, 0, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (devs == INVALID_HANDLE_VALUE) {
        return;
    }
    devcount = 0;
    devinterface.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    while (SetupDiEnumDeviceInterfaces(devs, 0, &intfce, devcount, &devinterface)) {
        /* The following buffers would normally be malloced to he correct size
         * but here we just declare them as large stack variables
         * to make the code more readable
         */
        WCHAR driverkey[2048];
        WCHAR interfacename[2048];
        WCHAR location[2048];

        /* If this is not the device we want, we would normally continue onto the next one
         * so something like if (!required_device) continue; would be added here
         */
        devcount++;
        size = 0;
        /* See how large a buffer we require for the device interface details */
        SetupDiGetDeviceInterfaceDetail(devs, &devinterface, 0, 0, &size, 0);
        devinfo.cbSize = sizeof(SP_DEVINFO_DATA);
        interface_detail = calloc(1, size);
        if (interface_detail) {
            interface_detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
            devinfo.cbSize = sizeof(SP_DEVINFO_DATA);
            if (!SetupDiGetDeviceInterfaceDetail(devs, &devinterface, interface_detail, size, 0, &devinfo)) {
                free(interface_detail);
                SetupDiDestroyDeviceInfoList(devs);
                return;
            }
            /* Make a copy of the device path for later use */
            wcscpy(interfacename, interface_detail->DevicePath);
            free(interface_detail);
            /* And now fetch some useful registry entries */
            size = sizeof(driverkey);
            driverkey[0] = 0;
            if (!SetupDiGetDeviceRegistryProperty(devs, &devinfo, SPDRP_DRIVER, &dataType, (LPBYTE)driverkey, size, 0)) {
                SetupDiDestroyDeviceInfoList(devs);
                return;
            }
            size = sizeof(location);
            location[0] = 0;
            if (!SetupDiGetDeviceRegistryProperty(devs, &devinfo, SPDRP_LOCATION_INFORMATION, &dataType, (LPBYTE)location, size, 0)) {
                SetupDiDestroyDeviceInfoList(devs);
                return;
            }
            usbHandle = CreateFile(interfacename, GENERIC_WRITE, FILE_SHARE_READ,
                NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

            FILE *file;
            char buffer[1024];
            DWORD bytesRead, bytesWritted;

            if (usbHandle != INVALID_HANDLE_VALUE) {
                // abre o arquivo
                if ((file = fopen(param, "r")) == NULL)
                {
                    printf("Erro ao abrir o arquivo '%s'\n", param);
                }
                else 
                {
                    while (!feof(file))
                    {
                        if ((bytesRead = fread(buffer, 1, sizeof(buffer) - 1, file)) > 0) 
                        {
                            if (!WriteFile(usbHandle, buffer, bytesRead, &bytesWritted, NULL))
                            {
                                printf("Erro ao escrever na impressora!\n");
                            }
                        }
                    }

                    fclose(file);
                }
                CloseHandle(usbHandle);
            }
        }
    }
    SetupDiDestroyDeviceInfoList(devs);
}

void ajuda(char* nome)
{
    printf("Use:\t%s <nome_arquivo>\n", nome);
    printf("Onde:\t<nome_arquivo> - arquivo a ser impresso.\n\n");
}

int main(int argc, char *argv[])
{
    char* param;
    char* nome = strrchr(argv[0], '\\');

    if (nome == NULL)
        nome = argv[0];
    else
        nome++;

    printf("%s - SOB Software - %s %s\n\n", nome, __DATE__, __TIME__);

    if (argc != 2)
    {
        ajuda(nome);
        return -99;
    }

    param = argv[1];

    SomeFunctionToWriteToUSB(param);

    return 0;
}