//*********************************************************
//
// ImageFormatInfo
//
// Defines options and strings for each supported image type
// or codec type.
//
//*********************************************************

#pragma once

#include "DirectXHelper.h"

namespace HDRImageViewer
{
    // Not necessarily a 1:1 mapping with file extension.
    // Uniquely identifies a image loading configuration.
    enum class ImageFormatId
    {
        Jxr,
        Jpg,
        Png,
        Tif,
        Hdr,
        Exr,
        Dds,
        Dng,
        Heic,
        Avif,
        Unknown
    };

    enum class DecoderLibrary
    {
        WIC,
        DirectXTex,
        LibRaw,
        Unknown
    };

    struct ImageFormatInfo
    {
        ImageFormatId formatId;
        DecoderLibrary decoderLibrary;
        std::wstring fileExtension; // All lowercase.
        std::wstring fileDescription; // Used in certain pickers like FileSavePicker.
        DX::OSVer minOSVer;
    };

    static ImageFormatInfo g_imageFormats[]
    {
        { ImageFormatId::Jxr     , DecoderLibrary::WIC       , L".jxr"   , L"JPEG-XR image"      , DX::OSVer::Win1809 },
        { ImageFormatId::Jpg     , DecoderLibrary::WIC       , L".jpg"   , L"JPEG image"         , DX::OSVer::Win1809 },
        { ImageFormatId::Png     , DecoderLibrary::WIC       , L".png"   , L"PNG image"          , DX::OSVer::Win1809 },
        { ImageFormatId::Tif     , DecoderLibrary::WIC       , L".tif"   , L"TIFF image"         , DX::OSVer::Win1809 },
        { ImageFormatId::Hdr     , DecoderLibrary::DirectXTex, L".hdr"   , L"HDR Radiance image" , DX::OSVer::Win1809 },
        { ImageFormatId::Exr     , DecoderLibrary::DirectXTex, L".exr"   , L"OpenEXR image"      , DX::OSVer::Win1809 },
        { ImageFormatId::Dds     , DecoderLibrary::DirectXTex, L".dds"   , L"DDS image"          , DX::OSVer::Win1809 },
        { ImageFormatId::Dng     , DecoderLibrary::LibRaw    , L".dng"   , L"DNG image"          , DX::OSVer::Win1809 },
        { ImageFormatId::Heic    , DecoderLibrary::WIC       , L".heic"  , L"HEIF image"         , DX::OSVer::Win1903 },
        { ImageFormatId::Avif    , DecoderLibrary::WIC       , L".avif"  , L"AVIF image"         , DX::OSVer::Win1903 },
    };

    // In the format returned by file pickers (e.g. L".abc").
    inline ImageFormatId GetFormatFromExtension(std::wstring ext)
    {
        for (int i = 0; i < ARRAYSIZE(g_imageFormats); i++)
        {
            if (_wcsicmp(ext.c_str(), g_imageFormats[i].fileExtension.c_str()) == 0)
            {
                return g_imageFormats[i].formatId;
            }
        }

        return ImageFormatId::Unknown;
    }

    inline DecoderLibrary GetDecoderLibrary(ImageFormatId id)
    {
        for (int i = 0; i < ARRAYSIZE(g_imageFormats); i++)
        {
            if (id == g_imageFormats[i].formatId)
            {
                return g_imageFormats[i].decoderLibrary;
            }
        }

        return DecoderLibrary::Unknown;
    }
}