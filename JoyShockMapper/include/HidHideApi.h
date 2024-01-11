// (c) Eric Korff de Gidts
// SPDX-License-Identifier: MIT
// Config.h
#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <Windows.h>
#include <initguid.h>
#include <devpkey.h>
#include <devguid.h>
#include <winioctl.h>
#include <Hidsdi.h>
#include <hidclass.h>
#pragma comment(lib, "Hid.lib")

namespace HidHide
{
    struct HidDeviceInstancePathWithModelInfo
    {
        HIDD_ATTRIBUTES attributes{};
        HIDP_CAPS       capabilities{};
        bool            present{};
        bool            gamingDevice{};
        std::wstring    vendor{};
        std::wstring    product{};
        std::wstring    serialNumber{};
        std::wstring    usage{};
        std::wstring    description{};
        std::wstring    deviceInstancePath{};
        std::wstring    deviceInstancePathBaseContainer{};
        GUID            deviceInstancePathBaseContainerClassGuid{};
        size_t          deviceInstancePathBaseContainerDeviceCount{};
    };

    typedef std::vector<HidDeviceInstancePathWithModelInfo> HidDeviceInstancePathsWithModelInfo;
    typedef std::vector<std::wstring> HidDeviceInstancePaths;
    typedef std::filesystem::path FullImageName;
    typedef std::vector<FullImageName> FullImageNames;
    typedef std::vector<std::pair<std::wstring, HidHide::HidDeviceInstancePathsWithModelInfo>> DescriptionToHidDeviceInstancePathsWithModelInfo;

    // Lookup a value from the string table resource based on the resource id provided
    std::wstring StringTable(_In_ uint32_t stringTableResourceId);

    // Create a hierarchical tree of human interface devices starting at the base container id level
    DescriptionToHidDeviceInstancePathsWithModelInfo GetDescriptionToHidDeviceInstancePathsWithModelInfo();

    // Method converting a logical file name into a full image name
    FullImageName FileNameToFullImageName(_In_ std::filesystem::path const& logicalFileName);

    // Get the control device state; returns true when the control device is present (installed and enabled)
    bool Present();

    // Get the device Instance Paths of the Human Interface Devices that are on the black-list (may reference not present devices)
    HidDeviceInstancePaths GetBlacklist();

    // Set the device Instance Paths of the Human Interface Devices that are on the black-list
    void SetBlacklist(_In_ HidDeviceInstancePaths const& hidDeviceInstancePaths);

    // Get the applications on the white-list
    FullImageNames GetWhitelist();

    // Set the applications on the white-list
    void SetWhitelist(_In_ FullImageNames const& fullImageNames);

    // Get the current enabled state; returns true when the device is active in hiding devices on the black-list
    bool GetActive();

    // Set the current enabled state
    void SetActive(_In_ bool active);
}
