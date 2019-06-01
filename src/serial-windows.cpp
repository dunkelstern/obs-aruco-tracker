#include "serial-windows.h"

#include <atlbase.h>
#include <vector>
#include <string>
#include <utility>
#include <atlstr.h>
#include <comdef.h>
#include <winioctl.h>
#include <setupapi.h>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "advapi32.lib")

serial_handle_t serial_open(const char *file, int baudrate) {
    serial_handle_t handle = CreateFile(file, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (handle == INVALID_HANDLE_VALUE) {
        if(GetLastError() == ERROR_FILE_NOT_FOUND) {
            blog(LOG_ERROR, "WinSerial: Can not open COM port, device does not exist");
        }
        return INVALID_HANDLE_VALUE;
    }
    
    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(handle, &dcbSerialParams)) {
        blog(LOG_ERROR, "WinSerial: Can not fetch COM port state");
        serial_close(handle);
        return INVALID_HANDLE_VALUE;
    }
    
    dcbSerialParams.BaudRate = baudrate;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    
    if(!SetCommState(handle, &dcbSerialParams)) {
        //error setting serial port state
        blog(LOG_ERROR, "WinSerial: Can not set COM port state");
        serial_close(handle);
        return INVALID_HANDLE_VALUE;
    }

    return handle;
}

void serial_close(serial_handle_t fd) {
    CloseHandle(fd);
}

void serial_write(serial_handle_t fd, uint64_t timestamp, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    char buffer[32];

    size_t len = snprintf(
        buffer,
        32,
        "%d,%d\n",
        (int)((((double)width / 2.0) - (double)x) / ((double)width / 8.0)),
        (int)(((double)y - ((double)height / 2.0)) / ((double)height / 8.0))
    );

    DWORD bytes_written = 0;
    WriteFile(fd, buffer, (DWORD)len, &bytes_written, NULL);
    if ((size_t)bytes_written != len) {
        blog(LOG_ERROR, "WinSerial: Error writing bytes");
    }
}

static bool regQueryValueString(ATL::CRegKey &key, LPCTSTR lpValueName, std::string &sValue) {
    // Initialize the output parameter
    sValue.clear();

    //First query for the size of the registry value
    ULONG nChars = 0;
    LSTATUS nStatus = key.QueryStringValue(lpValueName, nullptr, &nChars);
    if (nStatus != ERROR_SUCCESS) {
        SetLastError(nStatus);
        return false;
    }

    // Allocate enough bytes for the return value
    sValue.resize(static_cast<size_t>(nChars) + 1); //+1 is to allow us to null terminate the data if required
    const DWORD dwAllocatedSize = ((nChars + 1)*sizeof(TCHAR));

    // We will use RegQueryValueEx directly here because ATL::CRegKey::QueryStringValue does not handle non-null terminated data
    DWORD dwType = 0;
    ULONG nBytes = dwAllocatedSize;
    nStatus = RegQueryValueEx(key, lpValueName, nullptr, &dwType, reinterpret_cast<LPBYTE>(&(sValue[0])), &nBytes);
    if (nStatus != ERROR_SUCCESS) {
        SetLastError(nStatus);
        return false;
    }
    if ((dwType != REG_SZ) && (dwType != REG_EXPAND_SZ)) {
        SetLastError(ERROR_INVALID_DATA);
        return false;
    }
    if ((nBytes % sizeof(TCHAR)) != 0) {
        SetLastError(ERROR_INVALID_DATA);
        return false;
    }
    if (sValue[(nBytes / sizeof(TCHAR)) - 1] != _T('\0')) {
        // Forcibly null terminate the data ourselves
        sValue[(nBytes / sizeof(TCHAR))] = _T('\0');
    }

    return true;
}

static bool queryRegistryPortName(ATL::CRegKey& deviceKey, int &nPort) {
    // What will be the return value from the method (assume the worst)
    bool bAdded = false;

    // Read in the name of the port
    std::string sPortName;
    if (regQueryValueString(deviceKey, _T("PortName"), sPortName)) {
        // If it looks like "COMX" then
        // add it to the array which will be returned
        const size_t nLen = sPortName.length();
        if (nLen > 3) {
            if (_tcsnicmp(sPortName.c_str(), _T("COM"), 3) == 0) {
                //Work out the port number
                nPort = _ttoi(sPortName.c_str() + 3);
                bAdded = true;
            }
        }
    }

    return bAdded;
}

static bool queryDeviceDescription(HDEVINFO hDevInfoSet, SP_DEVINFO_DATA& devInfo, std::string &sFriendlyName) {
    DWORD dwType = 0;
    DWORD dwSize = 0;
    // Query initially to get the buffer size required
    if (!SetupDiGetDeviceRegistryProperty(hDevInfoSet, &devInfo, SPDRP_DEVICEDESC, &dwType, nullptr, 0, &dwSize)) {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        return false;
    }
    sFriendlyName.resize(dwSize / sizeof(TCHAR));
    if (!SetupDiGetDeviceRegistryProperty(hDevInfoSet, &devInfo, SPDRP_DEVICEDESC, &dwType, reinterpret_cast<PBYTE>(&(sFriendlyName[0])), dwSize, &dwSize)) {
        return false;
    }
    if (dwType != REG_SZ) {
        SetLastError(ERROR_INVALID_DATA);
        return false;
    }
    return true;
}

void serial_enumerate(obs_property_t *list_to_update) {
    std::vector<std::pair<UINT, std::string> > ports;

    const GUID guid = GUID_DEVINTERFACE_COMPORT;
    DWORD dwFlags = DIGCF_PRESENT | DIGCF_DEVICEINTERFACE;

    // Create a "device information set" for the specified GUID
    HDEVINFO hDevInfoSet = SetupDiGetClassDevs(&guid, nullptr, nullptr, dwFlags);
    if (hDevInfoSet == INVALID_HANDLE_VALUE) {
        blog(LOG_ERROR, "WinSerial: SetupDiGetClassDevs failed");
        return;
    }

    // Finally do the enumeration
    bool bMoreItems = true;
    int nIndex = 0;
    SP_DEVINFO_DATA devInfo = { 0 };
    while (bMoreItems) {
        // Enumerate the current device
        devInfo.cbSize = sizeof(SP_DEVINFO_DATA);
        bMoreItems = SetupDiEnumDeviceInfo(hDevInfoSet, nIndex, &devInfo);
        if (bMoreItems) {
            // Did we find a serial port for this device
            bool bAdded = false;

            std::pair<UINT, std::string> pair;

            // Get the registry key which stores the ports settings
            ATL::CRegKey deviceKey;
            deviceKey.Attach(SetupDiOpenDevRegKey(hDevInfoSet, &devInfo, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_QUERY_VALUE));
            if (deviceKey != INVALID_HANDLE_VALUE) {
                int nPort = 0;
                if (queryRegistryPortName(deviceKey, nPort)) {
                    blog(LOG_INFO, "WinSerial: Port with number %d found", nPort);
                    pair.first = nPort;
                    bAdded = true;
                }
            }

            // If the port was a serial port, then also try to get its friendly name
            if (bAdded) {
                if (queryDeviceDescription(hDevInfoSet, devInfo, pair.second)) {
                    blog(LOG_INFO, "WinSerial: Port %d has friendly name '%s'", pair.first, pair.second);
                    ports.push_back(pair);
                }
            }
        }
        ++nIndex;
    }

    // Free up the "device information set" now that we are finished with it
    SetupDiDestroyDeviceInfoList(hDevInfoSet);

    char com_name[7];
    char display_name[100];
    for(int i = 0; i < ports.size(); i++) {
        blog(LOG_INFO, "WinSerial: Found port %d '%s'", ports[i].first,  ports[i].second.c_str());
        snprintf(com_name, 7, "COM%d", ports[i].first);
        snprintf(display_name, 100, "%s (COM%d)", ports[i].second.c_str(), ports[i].first);
        obs_property_list_add_string(list_to_update, display_name, com_name);
    }

    return;
}
