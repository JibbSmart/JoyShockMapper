// (c) Eric Korff de Gidts
// SPDX-License-Identifier: MIT
// HidHideApi.cpp
//
// An abstraction layer for the business logic. Relies on exception handling for error handling
// Ideally, this layer has no state behavior, and limits itself to status reporting only (supporting proper decision making)
//
#include "HidHideApi.h"
#include <iostream>
#include <set>

// Logging
#define THROW_WIN32(result)     { throw std::runtime_error(""/*result*/);            }
#define THROW_WIN32_LAST_ERROR  { throw std::runtime_error(""/*::GetLastError()*/);  }

typedef std::unique_ptr<std::remove_pointer<HANDLE>::type, decltype(&::FindVolumeClose)> FindVolumeClosePtr;
typedef std::unique_ptr<std::remove_pointer<HANDLE>::type, decltype(&::CloseHandle)> CloseHandlePtr;
typedef std::unique_ptr<std::remove_pointer<PHIDP_PREPARSED_DATA>::type, decltype(&::HidD_FreePreparsedData)> HidD_FreePreparsedDataPtr;
typedef std::tuple<std::filesystem::path, GUID, CM_POWER_DATA> DeviceInstancePathAndClassGuidAndPowerState;

// The Hid Hide I/O control custom device type (range 32768 .. 65535)
constexpr auto IoControlDeviceType{ 32769u };

// The Hid Hide I/O control codes
constexpr auto IOCTL_GET_WHITELIST{ CTL_CODE(IoControlDeviceType, 2048, METHOD_BUFFERED, FILE_READ_DATA) };
constexpr auto IOCTL_SET_WHITELIST{ CTL_CODE(IoControlDeviceType, 2049, METHOD_BUFFERED, FILE_READ_DATA) };
constexpr auto IOCTL_GET_BLACKLIST{ CTL_CODE(IoControlDeviceType, 2050, METHOD_BUFFERED, FILE_READ_DATA) };
constexpr auto IOCTL_SET_BLACKLIST{ CTL_CODE(IoControlDeviceType, 2051, METHOD_BUFFERED, FILE_READ_DATA) };
constexpr auto IOCTL_GET_ACTIVE   { CTL_CODE(IoControlDeviceType, 2052, METHOD_BUFFERED, FILE_READ_DATA) };
constexpr auto IOCTL_SET_ACTIVE   { CTL_CODE(IoControlDeviceType, 2053, METHOD_BUFFERED, FILE_READ_DATA) };

//namespace
//{
//    // Convert the error code into an error text
//    std::wstring ErrorMessage(_In_ DWORD errorCode)
//    {
//        std::vector<WCHAR> buffer(UNICODE_STRING_MAX_CHARS);
//        ::FormatMessageW((FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS), nullptr, errorCode, LANG_USER_DEFAULT, buffer.data(), static_cast<DWORD>(buffer.size()), nullptr);
//        return (buffer.data());
//    }
//
//    // Convert a guid into a string
//    std::wstring GuidToString(_In_ GUID const& guid)
//    {
//        
//        std::vector<WCHAR> buffer(39);
//        if (0 == ::StringFromGUID2(guid, buffer.data(), static_cast<int>(buffer.size()))) THROW_WIN32(ERROR_INVALID_PARAMETER);
//        return (buffer.data());
//    }
//
//    // Split the string at the white-spaces
//    std::vector<std::wstring> SplitStringAtWhitespaces(_In_ std::wstring const& value)
//    {
//        
//        std::vector<std::wstring> result;
//        for (size_t begin{};;)
//        {
//            if (auto const position{ value.find(L' ', begin) }; (std::wstring::npos == position))
//            {
//                if (begin < value.size()) result.emplace_back(value.substr(begin));
//                break;
//            }
//            else
//            {
//                if (begin != position) result.emplace_back(value.substr(begin, position - begin));
//                begin = position + 1;
//            }
//        }
//        return (result);
//    }

    // Convert a list of strings into a multi-string
    std::vector<WCHAR> StringListToMultiString(_In_ std::vector<std::wstring> const& values)
    {
        
        std::vector<WCHAR> result;
        for (auto const& value : values)
        {
            auto const oldSize{ result.size() };
            auto const appendSize{ value.size() + 1 };
            result.resize(oldSize + appendSize);
            if (0 != ::wcsncpy_s(&result.at(oldSize), appendSize, value.c_str(), appendSize)) THROW_WIN32(ERROR_INVALID_PARAMETER);
        }
        result.push_back(L'\0');
        return (result);
    }

    // Convert a multi-string into a list of strings
    std::vector<std::wstring> MultiStringToStringList(_In_ std::vector<WCHAR> const& multiString)
    {
        
        std::vector<std::wstring> result;
        for (size_t index{}, start{}, size{ multiString.size() }; (index < size); index++)
        {
            // Skip empty strings (typically the list terminator)
            if (0 == multiString.at(index))
            {
                std::wstring const string(&multiString.at(start), 0, index - start);
                if (!string.empty()) result.emplace_back(string);
                start = index + 1;
            }
        }
        return (result);
    }

    // Get the DosDeviceName associated with a volume
    std::filesystem::path DosDeviceName(_In_ std::vector<WCHAR> const& volumeGuid)
    {
        
        // Cleanup the volume Guid, meaning strip the leading \\?\ prefix, and remove the trailing slash
        std::wstring cleanedUpvolumeGuid{ &volumeGuid.at(4) };
        cleanedUpvolumeGuid.pop_back();
        std::vector<WCHAR> buffer(UNICODE_STRING_MAX_CHARS);
        if (0 == ::QueryDosDeviceW(cleanedUpvolumeGuid.c_str(), buffer.data(), static_cast<DWORD>(buffer.size()))) THROW_WIN32_LAST_ERROR;
        return (buffer.data());
    }

    // Get the logical drives associated with a volume
    std::set<std::filesystem::path> LogicalDrives(_In_ std::vector<WCHAR> const& volumeGuid)
    {
        
        std::set<std::filesystem::path> result;

        DWORD needed{};
        std::vector<WCHAR> logicalDrives(UNICODE_STRING_MAX_CHARS);
        if (FALSE == ::GetVolumePathNamesForVolumeNameW(volumeGuid.data(), logicalDrives.data(), static_cast<DWORD>(logicalDrives.size()), &needed))
        {
            if (ERROR_MORE_DATA != ::GetLastError()) THROW_WIN32_LAST_ERROR;
        }
        else
        {
            // Iterate all logical disks associated with this volume
            auto const list{ MultiStringToStringList(logicalDrives) };
            for (auto it{ std::begin(list) }; (std::end(list) != it); it++) result.emplace(*it);
        }

        return (result);
    }

//    // Get the Device Instance Paths of all devices of a given class (present or not present)
//    // Note that performing a query for the Hid-Hide service controlled devices turned out not to work for VMWare environments hence query the whole list here and work from there
//    std::vector<std::wstring> DeviceInstancePaths(_In_ GUID const& classGuid)
//    {
//        
//        ULONG needed{};
//        if (auto const result{ ::CM_Get_Device_ID_List_SizeW(&needed, GuidToString(classGuid).c_str(), CM_GETIDLIST_FILTER_CLASS) }; (CR_SUCCESS != result)) THROW_CONFIGRET(result);
//        std::vector<WCHAR> buffer(needed);
//        if (auto const result{ ::CM_Get_Device_ID_ListW(GuidToString(classGuid).c_str(), buffer.data(), needed, CM_GETIDLIST_FILTER_CLASS) }; (CR_SUCCESS != result)) THROW_CONFIGRET(result);
//        return (MultiStringToStringList(buffer));
//    }
//
//    // Get the device description
//    std::wstring DeviceDescription(_In_ std::wstring const& deviceInstancePath)
//    {
//        
//        DEVINST     devInst{};
//        DEVPROPTYPE devPropType{};
//        ULONG       needed{};
//        if (auto const result{ ::CM_Locate_DevNodeW(&devInst, const_cast<DEVINSTID_W>(deviceInstancePath.c_str()), CM_LOCATE_DEVNODE_PHANTOM) }; (CR_SUCCESS != result)) THROW_CONFIGRET(result);
//        if (auto const result{ ::CM_Get_DevNode_PropertyW(devInst, &DEVPKEY_Device_DeviceDesc, &devPropType, nullptr, &needed, 0) }; (CR_BUFFER_SMALL != result)) THROW_CONFIGRET(result);
//        if (DEVPROP_TYPE_STRING != devPropType) THROW_WIN32(ERROR_INVALID_PARAMETER);
//        std::vector<WCHAR> buffer(needed);
//        if (auto const result{ ::CM_Get_DevNode_PropertyW(devInst, &DEVPKEY_Device_DeviceDesc, &devPropType, reinterpret_cast<PBYTE>(buffer.data()), &needed, 0) }; (CR_SUCCESS != result)) THROW_CONFIGRET(result);
//        return (buffer.data());
//    }
//
//    // Determine the number of child devices for a given base container device (the base control device itself is excluded from the count)
//    // Returns zero when the device instance path is empty (typical for devices that aren't part of a base container)
//    size_t BaseContainerDeviceCount(_In_ std::wstring const& deviceInstancePath)
//    {
//        
//
//        // Bail out when the device instance path is empty
//        if (deviceInstancePath.empty()) return (0);
//
//        // Prepare the iterator
//        DEVINST devInst{};
//        DEVINST devInstChild{};
//        if (auto const result{ ::CM_Locate_DevNodeW(&devInst, const_cast<DEVINSTID_W>(deviceInstancePath.c_str()), CM_LOCATE_DEVNODE_PHANTOM) }; (CR_SUCCESS != result)) THROW_CONFIGRET(result);
//        if (auto const result{ ::CM_Get_Child(&devInstChild, devInst, 0) }; (CR_NO_SUCH_DEVNODE == result)) return (0); else if (CR_SUCCESS != result) THROW_CONFIGRET(result);
//
//        // Iterate all child devices
//        for(size_t count{ 1 };; count++)
//        {
//            if (auto const result{ ::CM_Get_Sibling(&devInstChild, devInstChild, 0) }; (CR_NO_SUCH_DEVNODE == result)) return (count); else if (CR_SUCCESS != result) THROW_CONFIGRET(result);
//        }
//    }
//
//    // Get the base container id of a particular device instance
//    GUID DeviceBaseContainerId(_In_ std::wstring const& deviceInstancePath)
//    {
//        
//
//        // Bail out when the device instance path is empty
//        if (deviceInstancePath.empty()) return (GUID_NULL);
//
//        DEVINST     devInst{};
//        DEVPROPTYPE devPropType{};
//        GUID        buffer{};
//        ULONG       needed{ sizeof(buffer) };
//        if (auto const result{ ::CM_Locate_DevNodeW(&devInst, const_cast<DEVINSTID_W>(deviceInstancePath.c_str()), CM_LOCATE_DEVNODE_PHANTOM) }; (CR_SUCCESS != result)) THROW_CONFIGRET(result);
//        if (auto const result{ ::CM_Get_DevNode_PropertyW(devInst, &DEVPKEY_Device_ContainerId, &devPropType, reinterpret_cast<PBYTE>(&buffer), &needed, 0) }; (CR_SUCCESS != result)) THROW_CONFIGRET(result);
//        if (DEVPROP_TYPE_GUID != devPropType) THROW_WIN32(ERROR_INVALID_PARAMETER);
//        return (buffer);
//    }
//
//    // Get the device class guid
//    // For a base container device this is typically GUID_DEVCLASS_USB or GUID_DEVCLASS_HIDCLASS or GUID_DEVCLASS_XUSBCLASS 
//    GUID DeviceClassGuid(_In_ std::wstring const& deviceInstancePath)
//    {
//        
//
//        // Bail out when the device instance path is empty
//        if (deviceInstancePath.empty()) return (GUID_NULL);
//
//        DEVINST     devInst{};
//        DEVPROPTYPE devPropType{};
//        GUID        buffer{};
//        ULONG       needed{ sizeof(buffer) };
//        if (auto const result{ ::CM_Locate_DevNodeW(&devInst, const_cast<DEVINSTID_W>(deviceInstancePath.c_str()), CM_LOCATE_DEVNODE_PHANTOM) }; (CR_SUCCESS != result)) THROW_CONFIGRET(result);
//        if (auto const result{ ::CM_Get_DevNode_PropertyW(devInst, &DEVPKEY_Device_ClassGuid, &devPropType, reinterpret_cast<PBYTE>(&buffer), &needed, 0) }; (CR_SUCCESS != result)) THROW_CONFIGRET(result);
//        if (DEVPROP_TYPE_GUID != devPropType) THROW_WIN32(ERROR_INVALID_PARAMETER);
//        return (buffer);
//    }
//
//    // Is the device present or absent
//    bool DevicePresent(_In_ std::wstring const& deviceInstancePath)
//    {
//        
//        DEVINST devInst{};
//        if (auto const result{ ::CM_Locate_DevNodeW(&devInst, const_cast<DEVINSTID_W>(deviceInstancePath.c_str()), CM_LOCATE_DEVNODE_NORMAL) }; (CR_NO_SUCH_DEVNODE == result) || (CR_SUCCESS == result)) return (CR_SUCCESS == result); else THROW_CONFIGRET(result);
//    }
//     
//    // Get the parent of a particular device instance
//    std::wstring DeviceInstancePathParent(_In_ std::wstring const& deviceInstancePath)
//    {
//        
//
//        DEVINST            devInst{};
//        DEVPROPTYPE        devPropType{};
//        DEVINST            devInstParent{};
//        std::vector<WCHAR> buffer(UNICODE_STRING_MAX_CHARS);
//        ULONG              needed{ static_cast<ULONG>(buffer.size()) };
//        if (auto const result{ ::CM_Locate_DevNodeW(&devInst, const_cast<DEVINSTID_W>(deviceInstancePath.c_str()), CM_LOCATE_DEVNODE_PHANTOM) }; (CR_SUCCESS != result)) THROW_CONFIGRET(result);
//        if (auto const result{ ::CM_Get_Parent(&devInstParent, devInst, 0) }; (CR_SUCCESS != result)) THROW_CONFIGRET(result);
//        if (auto const result{ ::CM_Get_DevNode_PropertyW(devInstParent, &DEVPKEY_Device_InstanceId, &devPropType, reinterpret_cast<PBYTE>(buffer.data()), &needed, 0) }; (CR_SUCCESS != result)) THROW_CONFIGRET(result);
//        if (DEVPROP_TYPE_STRING != devPropType) THROW_WIN32(ERROR_INVALID_PARAMETER);
//        return (buffer.data());
//    }
//
//    // Determine the Symbolic Link towards a particular device
//    // Returns the Symbolic Link suited for CreateFile or an empty string when the device interface isn't supported
//    std::filesystem::path SymbolicLink(_In_ GUID const& deviceInterfaceGuid, _In_ std::wstring const& deviceInstancePath)
//    {
//        
//
//        // Ask the device for the device interface
//        // Note that this call will succeed, whether or not the interface is present, but the iterator will have no entries, when the device interface isn't supported
//        auto const handle{ SetupDiDestroyDeviceInfoListPtr(::SetupDiGetClassDevsW(&deviceInterfaceGuid, deviceInstancePath.c_str(), nullptr, DIGCF_DEVICEINTERFACE), &::SetupDiDestroyDeviceInfoList) };
//        if (INVALID_HANDLE_VALUE == handle.get()) THROW_WIN32(ERROR_INVALID_PARAMETER);
//
//        // Is the interface supported ?
//        SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
//        deviceInterfaceData.cbSize = sizeof(deviceInterfaceData);
//        if (FALSE == ::SetupDiEnumDeviceInterfaces(handle.get(), nullptr, &deviceInterfaceGuid, 0, &deviceInterfaceData))
//        {
//            if (ERROR_NO_MORE_ITEMS != ::GetLastError()) THROW_WIN32_LAST_ERROR;
//
//            // Sorry but not a device interface isn't supported
//            return (L"");
//        }
//
//        // Determine the buffer length needed
//        DWORD needed{};
//        if ((FALSE == ::SetupDiGetDeviceInterfaceDetailW(handle.get(), &deviceInterfaceData, nullptr, 0, &needed, nullptr)) && (ERROR_INSUFFICIENT_BUFFER != ::GetLastError())) THROW_WIN32_LAST_ERROR;
//        std::vector<BYTE> buffer(needed);
//
//        // Acquire the detailed data containing the symbolic link (aka. device path)
//        auto& deviceInterfaceDetailData{ *reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA_W>(buffer.data()) };
//        deviceInterfaceDetailData.cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
//        if (FALSE == ::SetupDiGetDeviceInterfaceDetailW(handle.get(), &deviceInterfaceData, reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA_W>(buffer.data()), static_cast<DWORD>(buffer.size()), nullptr, nullptr)) THROW_WIN32_LAST_ERROR;
//        return (deviceInterfaceDetailData.DevicePath);
//    }
//
//    // When a device has a base container id then get the device instance path of the base container id
//    // Returns an empty string when the device doesn't have a base container id
//    std::wstring DeviceInstancePathBaseContainerId(_In_ std::wstring const& deviceInstancePath)
//    {
//        
//
//        auto const baseContainerId{ DeviceBaseContainerId(deviceInstancePath) };
//        if ((GUID_NULL == baseContainerId) || (GUID_CONTAINER_ID_SYSTEM == baseContainerId)) return (std::wstring{});
//        for (auto it{ deviceInstancePath };;)
//        {
//            if (auto const deviceInstancePathParent{ DeviceInstancePathParent(it) }; (baseContainerId == DeviceBaseContainerId(deviceInstancePathParent))) it = deviceInstancePathParent; else return (it);
//        }
//    }
//
//    // Determine from the model information and usage if this is a gaming device or not
//    bool GamingDevice(_In_ HIDD_ATTRIBUTES const& attributes, _In_ HIDP_CAPS const& capabilities)
//    {
//        
//
//        // 0x28DE 0x1142 = Valve Corporation Steam Controller
//        return (((attributes.VendorID == 0x28DE) && (attributes.ProductID == 0x1142)) || (0x05 == capabilities.UsagePage) || (0x01 == capabilities.UsagePage) && ((0x04 == capabilities.Usage) || (0x05 == capabilities.Usage)));
//    }
//
//    // Get the usage name from the usage page and usage id
//    // Based on Revision 1.2 of standard (see also https://usb.org/sites/default/files/hut1_2.pdf)
//    std::wstring UsageName(_In_ HIDP_CAPS const& capabilities)
//    {
//        
//        std::vector<WCHAR> buffer(UNICODE_STRING_MAX_CHARS);
//
//        // For usage id 0x00 through 0xFF of page 1 we have a dedicated lookup table for the usage name at resource id offset 0x1100
//        if ((0x01 == capabilities.UsagePage) && (0xFF >= capabilities.Usage) && (0 != ::LoadStringW(AfxGetApp()->m_hInstance, (0x1100 + capabilities.Usage), buffer.data(), static_cast<int>(buffer.size())))) return (buffer.data());
//
//        // For usage id 0x00 through 0xFF of page 12 we have a dedicated lookup table for the usage name at resource id offset 0x1200
//        if ((0x0C == capabilities.UsagePage) && (0xFF >= capabilities.Usage) && (0 != ::LoadStringW(AfxGetApp()->m_hInstance, (0x1200 + capabilities.Usage), buffer.data(), static_cast<int>(buffer.size())))) return (buffer.data());
//
//        // For usage page 0x00 through 0xFF we have a dedicated lookup table for the page name at resource id offset 0x1000
//        std::wostringstream os;
//        os.exceptions(std::ostringstream::failbit | std::ostringstream::badbit);
//        os << std::hex << std::uppercase << std::setfill(L'0') << std::setw(4);
//        if ((0xFF >= capabilities.UsagePage) && (0 != ::LoadStringW(AfxGetApp()->m_hInstance, (0x1000 + capabilities.UsagePage), buffer.data(), static_cast<int>(buffer.size()))))
//        {
//            // Found page now only show the usage id
//            os << buffer.data() << L" " << capabilities.Usage;
//        }
//        else
//        {
//            // Generic catch-all showing the page and usage id
//            os << capabilities.UsagePage << L" " << capabilities.Usage;
//        }
//
//        return (os.str());
//    }
//
//    // Interact with the HID device and query its product id, serial number, and manufacturer
//    HidHide::HidDeviceInstancePathWithModelInfo HidModelInfo(_In_ std::wstring const& deviceInstancePath, _In_ std::filesystem::path const& symbolicLink)
//    {
//        
//        HidHide::HidDeviceInstancePathWithModelInfo result;
//        result.gamingDevice                               = false;
//        result.present                                    = DevicePresent(deviceInstancePath);
//        result.description                                = DeviceDescription(deviceInstancePath);
//        result.deviceInstancePath                         = deviceInstancePath;
//        result.deviceInstancePathBaseContainer            = DeviceInstancePathBaseContainerId(deviceInstancePath);
//        result.deviceInstancePathBaseContainerClassGuid   = DeviceClassGuid(result.deviceInstancePathBaseContainer);
//        result.deviceInstancePathBaseContainerDeviceCount = BaseContainerDeviceCount(result.deviceInstancePathBaseContainer);
//
//        // Open a handle to communicate with the HID device
//        auto const deviceObject{ CloseHandlePtr(::CreateFileW(symbolicLink.c_str(), GENERIC_READ, (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr), &::CloseHandle) };
//        if (INVALID_HANDLE_VALUE == deviceObject.get())
//        {
//            if (ERROR_ACCESS_DENIED == ::GetLastError())
//            {
//                // The device is (most-likely) cloaked by Hid Hide itself while its client application isn't on the white-list
//                result.usage = HidHide::StringTable(IDS_HID_ATTRIBUTE_DENIED);
//                return (result);
//            }
//            if (ERROR_FILE_NOT_FOUND == ::GetLastError())
//            {
//                // The device is currently not present hence we can't query its details
//                result.usage = HidHide::StringTable(IDS_HID_ATTRIBUTE_ABSENT);
//                return (result);
//            }
//            THROW_WIN32_LAST_ERROR;
//        }
//
//        // Prepare for device interactions
//        PHIDP_PREPARSED_DATA preParsedData;
//        if (FALSE == ::HidD_GetPreparsedData(deviceObject.get(), &preParsedData)) THROW_WIN32_LAST_ERROR;
//        auto const freePreparsedDataPtr{ HidD_FreePreparsedDataPtr(preParsedData, &::HidD_FreePreparsedData) };
//
//        // Get the usage page
//        if (HIDP_STATUS_SUCCESS != ::HidP_GetCaps(preParsedData, &result.capabilities)) THROW_WIN32(ERROR_INVALID_PARAMETER);
//
//        // Get the model information
//        if (FALSE == ::HidD_GetAttributes(deviceObject.get(), &result.attributes)) THROW_WIN32(ERROR_INVALID_PARAMETER);
//        
//        // Get the model information
//        std::vector<WCHAR> buffer(HidApiStringSizeInCharacters);
//        result.product      = (FALSE == ::HidD_GetProductString     (deviceObject.get(), buffer.data(), static_cast<ULONG>(sizeof(WCHAR) * buffer.size()))) ? L"" : std::wstring(buffer.data());
//        result.vendor       = (FALSE == ::HidD_GetManufacturerString(deviceObject.get(), buffer.data(), static_cast<ULONG>(sizeof(WCHAR) * buffer.size()))) ? L"" : std::wstring(buffer.data());
//        result.serialNumber = (FALSE == ::HidD_GetSerialNumberString(deviceObject.get(), buffer.data(), static_cast<ULONG>(sizeof(WCHAR) * buffer.size()))) ? L"" : std::wstring(buffer.data());
//        result.usage        = UsageName(result.capabilities);
//        result.gamingDevice = GamingDevice(result.attributes, result.capabilities);
//
//        return (result);
//    }

    // Convert path list to string list
    std::vector<std::wstring> PathListToStringList(_In_ std::vector<std::filesystem::path> const& paths)
    {
        
        std::vector<std::wstring> result;
        for (auto& path : paths) result.emplace_back(path);
        return (result);
    }

    // Convert string list to path list
    std::vector<std::filesystem::path> StringListToPathList(_In_ std::vector<std::wstring> const& strings)
    {
        
        std::vector<std::filesystem::path> result;
        for (auto& string : strings) result.emplace_back(string);
        return (result);
    }

    // Get a file handle to the device driver
    // The flag allowFileNotFound is applied when the device couldn't be found and controls whether or not an exception is thrown on failure
    CloseHandlePtr DeviceHandle(_In_ bool allowFileNotFound = false)
    {
        
        auto handle{ CloseHandlePtr(::CreateFileW(L"\\\\.\\HidHide", GENERIC_READ, (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr), &::CloseHandle) };
        if ((INVALID_HANDLE_VALUE == handle.get()) && ((ERROR_FILE_NOT_FOUND != ::GetLastError()) || (!allowFileNotFound))) THROW_WIN32_LAST_ERROR;
        return (handle);
    }

/*    CloseHandlePtr DeviceHandle()
    {
        auto handle{ CloseHandlePtr(::CreateFileW(HidHide::StringTable(IDS_CONTROL_SERVICE_PATH).c_str(), GENERIC_READ, (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE), nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr), &::CloseHandle) };
        if (INVALID_HANDLE_VALUE == handle.get()) THROW_WIN32_LAST_ERROR;
        return (handle);
    }
*/
//    // Merge model information into a single string
//    void MergeModelInformation(_Inout_ std::vector<std::wstring>& model, _In_ std::wstring const& append)
//    {
//        
//
//        // Iterate all parts of the string offered
//        for (auto& part : SplitStringAtWhitespaces(append))
//        {
//            // Add the part when its unique
//            if (std::end(model) == std::find_if(std::begin(model), std::end(model), [part](std::wstring const& value) { return (0 == ::StrCmpLogicalW(value.c_str(), part.c_str())); }))
//            {
//                model.emplace_back(part);
//            }
//        }
//    }
//
//    // Create a flat list of human interface devices found and collect typical model information
//    // PresentOnly allows for filtering-out any device that isn't yet present
//    HidHide::HidDeviceInstancePathsWithModelInfo GetHidDeviceInstancePathsWithModelInfo()
//    {
//        
//        HidHide::HidDeviceInstancePathsWithModelInfo result;
//
//        // Get the HID interface, as used by DirectInput
//        GUID hidDeviceInterfaceGuid;
//        ::HidD_GetHidGuid(&hidDeviceInterfaceGuid);
//
//        // Get the Device Instance Paths of all HID devices
//        // Note that the list includes base container devices that don't offer a HID interface
//        for (auto const& deviceInstancePath : DeviceInstancePaths(GUID_DEVCLASS_HIDCLASS))
//        {
//            // Filter the list for devices with a HID interface
//            if (auto const symbolicLink{ SymbolicLink(hidDeviceInterfaceGuid, deviceInstancePath) }; !symbolicLink.empty())
//            {
//                // Get the model information using the HID interface
//                result.emplace_back(HidModelInfo(deviceInstancePath, symbolicLink));
//            }
//        }
//
//        return (result);
//    }
//}

namespace HidHide
{
    //_Use_decl_annotations_
    //std::wstring StringTable(UINT stringTableResourceId)
    //{
    //    std::vector<WCHAR> buffer(UNICODE_STRING_MAX_CHARS);
    //    if (0 == ::LoadStringW(GetModuleHandle(nullptr), stringTableResourceId, buffer.data(), static_cast<int>(buffer.size()))) THROW_WIN32_LAST_ERROR;
    //    return (buffer.data());
    //}

//    DescriptionToHidDeviceInstancePathsWithModelInfo GetDescriptionToHidDeviceInstancePathsWithModelInfo()
//    {
//        DescriptionToHidDeviceInstancePathsWithModelInfo result;
//
//        // Get all human interface devices and process each entry
//        auto const hidDeviceInstancePathsWithModelInfo{ GetHidDeviceInstancePathsWithModelInfo()};
//        for (auto it{ std::begin(hidDeviceInstancePathsWithModelInfo) }; (std::end(hidDeviceInstancePathsWithModelInfo) != it); it++)
//        {
//            auto const hasBaseContainerId{ (!it->deviceInstancePathBaseContainer.empty()) };
//
//            // Skip already processed entries (checked by looking for the Device Instance Path of the Base Container in earlier entries)
//            if (hasBaseContainerId)
//            {
//                if (it != std::find_if(std::begin(hidDeviceInstancePathsWithModelInfo), it, [it](HidDeviceInstancePathWithModelInfo const& value) { return (value.deviceInstancePathBaseContainer == it->deviceInstancePathBaseContainer); }))
//                {
//                    continue;
//                }
//            }
//
//            // Merge manufacturer and product info
//            std::vector<std::wstring> model;
//            MergeModelInformation(model, it->vendor);
//            MergeModelInformation(model, it->product);
//
//            // When this device has a base container id then look for additional information in related entries
//            HidDeviceInstancePathsWithModelInfo children;
//            children.emplace_back(*it);
//            if (hasBaseContainerId)
//            {
//                for (auto ahead{ it + 1 }; (std::end(hidDeviceInstancePathsWithModelInfo) != ahead); ahead++)
//                {
//                    if (ahead->deviceInstancePathBaseContainer == it->deviceInstancePathBaseContainer)
//                    {
//                        children.emplace_back(*ahead);
//                        MergeModelInformation(model, ahead->vendor);
//                        MergeModelInformation(model, ahead->product);
//                    }
//                }
//            }
//
//            // Combine the information into a top-level description
//            std::wstring description;
//            for (auto& part : model)
//            {
//                description = (description.empty() ? part : description + L" " + part);
//            }
//
//            // As a last resort, when everyting else fails, take the device description (this typically doesn't say anything but at least the box isn't empty)
//            if (description.empty()) description = DeviceDescription(it->deviceInstancePath);
//
//            // Preserve the base container id and its children
//            result.emplace_back(std::make_pair(description, children));
//        }
//
//        return (result);
//    }

    _Use_decl_annotations_
    FullImageName FileNameToFullImageName(std::filesystem::path const& logicalFileName)
    {
        

        // Prepare the volume Guid iterator
        std::vector<WCHAR> volumeGuid(UNICODE_STRING_MAX_CHARS);
        auto const findVolumeClosePtr{ FindVolumeClosePtr(::FindFirstVolumeW(volumeGuid.data(), static_cast<DWORD>(volumeGuid.size())), &::FindVolumeClose) };
        if (INVALID_HANDLE_VALUE == findVolumeClosePtr.get()) THROW_WIN32_LAST_ERROR;

        // Keep iterating the volume Guids
        while (true)
        {
            // Get the logical drives associated with the volume
            auto const logicalDrives{ LogicalDrives(volumeGuid) };

            // Iterate all logical drives and see if we have a match
            for (auto it{ std::begin(logicalDrives) }; (std::end(logicalDrives) != it); it++)
            {
                // When we have a match return the full image name
                if (it->root_name() == logicalFileName.root_name())
                {
                    return (DosDeviceName(volumeGuid) / logicalFileName.relative_path());
                }
            }

            // Not found then move to the next volume
            if (FALSE == ::FindNextVolumeW(findVolumeClosePtr.get(), volumeGuid.data(), static_cast<DWORD>(volumeGuid.size())))
            {
                if (ERROR_NO_MORE_FILES != ::GetLastError()) THROW_WIN32_LAST_ERROR;

                // No more volumes to iterate
                break;
            }
        }

        return ("");
    }

    bool Present()
    {
        bool isPresent = false;
        try
        {
            isPresent = (INVALID_HANDLE_VALUE != DeviceHandle(true).get());
        }
        catch (...)
        {
            std::cerr << "HidHide Present() error " << GetLastError() << std::endl;
        }
        return isPresent;
    }

    HidDeviceInstancePaths GetBlacklist()
    {
        
        DWORD needed;
        if (FALSE == ::DeviceIoControl(DeviceHandle().get(), IOCTL_GET_BLACKLIST, nullptr, 0, nullptr, 0, &needed, nullptr)) THROW_WIN32_LAST_ERROR;
        auto buffer{ std::vector<WCHAR>(needed) };
        if (FALSE == ::DeviceIoControl(DeviceHandle().get(), IOCTL_GET_BLACKLIST, nullptr, 0, buffer.data(), static_cast<DWORD>(buffer.size() * sizeof(WCHAR)), &needed, nullptr)) THROW_WIN32_LAST_ERROR;
        return (MultiStringToStringList(buffer));
    }

    _Use_decl_annotations_
    void SetBlacklist(HidDeviceInstancePaths const& hidDeviceInstancePaths)
    {
        
        DWORD needed;
        auto buffer{ StringListToMultiString(hidDeviceInstancePaths) };
        if (FALSE == ::DeviceIoControl(DeviceHandle().get(), IOCTL_SET_BLACKLIST, buffer.data(), static_cast<DWORD>(buffer.size() * sizeof(WCHAR)), nullptr, 0, &needed, nullptr)) THROW_WIN32_LAST_ERROR;
    }

    FullImageNames GetWhitelist()
    {
        
        DWORD needed;
        if (FALSE == ::DeviceIoControl(DeviceHandle().get(), IOCTL_GET_WHITELIST, nullptr, 0, nullptr, 0, &needed, nullptr)) THROW_WIN32_LAST_ERROR;
        auto buffer{ std::vector<WCHAR>(needed) };
        if (FALSE == ::DeviceIoControl(DeviceHandle().get(), IOCTL_GET_WHITELIST, nullptr, 0, buffer.data(), static_cast<DWORD>(buffer.size() * sizeof(WCHAR)), &needed, nullptr)) THROW_WIN32_LAST_ERROR;
        return (StringListToPathList(MultiStringToStringList(buffer)));
    }

    _Use_decl_annotations_
    void SetWhitelist(FullImageNames const& applicationsOnWhitelist)
    {
        
        DWORD needed;
        auto buffer{ StringListToMultiString(PathListToStringList(applicationsOnWhitelist)) };
        if (FALSE == ::DeviceIoControl(DeviceHandle().get(), IOCTL_SET_WHITELIST, buffer.data(), static_cast<DWORD>(buffer.size() * sizeof(WCHAR)), nullptr, 0, &needed, nullptr)) THROW_WIN32_LAST_ERROR;
    }

    bool GetActive()
    {
        
        DWORD needed;
        auto buffer{ std::vector<BOOLEAN>(1) };
        if (FALSE == ::DeviceIoControl(DeviceHandle().get(), IOCTL_GET_ACTIVE, nullptr, 0, buffer.data(), static_cast<DWORD>(buffer.size() * sizeof(BOOLEAN)), &needed, nullptr)) THROW_WIN32_LAST_ERROR;
        if (sizeof(BOOLEAN) != needed) THROW_WIN32(ERROR_INVALID_PARAMETER);
        return (FALSE != buffer.at(0));
    }

    _Use_decl_annotations_
    void SetActive(bool active)
    {
        
        DWORD needed;
        auto buffer{ std::vector<BOOLEAN>(1) };
        buffer.at(0) = (active ? TRUE : FALSE);
        if (FALSE == ::DeviceIoControl(DeviceHandle().get(), IOCTL_SET_ACTIVE, buffer.data(), static_cast<DWORD>(buffer.size() * sizeof(BOOLEAN)), nullptr, 0, &needed, nullptr)) THROW_WIN32_LAST_ERROR;
    }
}
