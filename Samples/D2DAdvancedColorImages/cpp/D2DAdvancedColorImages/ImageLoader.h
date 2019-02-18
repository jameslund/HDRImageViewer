//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

//*********************************************************
//
// ImageLoader
//
// Manages loading an image from disk or other stream source
// into Direct2D. Handles all codec operations (WIC),
// detecting image info, and providing a Direct2D ImageSource.
//
// ImageLoader relies on the caller to explicitly inform it
// of device lost/restored events, i.e. it does not
// independently register for DX::IDeviceNotify.
//
// Throws WINCODEC_ERR_[foo] HRESULTs in exceptions as these
// match well with the intended error states.
//
//*********************************************************

#pragma once
#include "DeviceResources.h"

namespace D2DAdvancedColorImages
{
    enum ImageLoaderState
    {
        NotInitialized,
        LoadingSucceeded,
        LoadingFailed,
        NeedDeviceResources // Device resources must be (re)created but otherwise image data is valid.
    };

    struct ImageInfo
    {
        unsigned int                                    bitsPerPixel;
        unsigned int                                    bitsPerChannel;
        bool                                            isFloat;
        Windows::Foundation::Size                       size;
        unsigned int                                    numProfiles;
        Windows::Graphics::Display::AdvancedColorKind   imageKind;
        bool                                            isXboxHdrScreenshot;
        bool                                            isValid;
    };

    struct ImageCLL
    {
        float   maxNits;
        float   medNits;
    };

    class ImageLoader
    {
    public:
        ImageLoader(const std::shared_ptr<DX::DeviceResources>& deviceResources);
        ~ImageLoader();

        ImageLoaderState GetState() const { return m_state; };

        ImageInfo LoadImageFromWic(_In_ IStream* imageStream);
        ImageInfo LoadImageFromDirectXTex(_In_ Platform::String^ filename, _In_ Platform::String^ extension);

        ID2D1TransformedImageSource* GetLoadedImage(float zoom);
        ID2D1ColorContext* GetImageColorContext();

        void CreateDeviceDependentResources();
        void ReleaseDeviceDependentResources();

    private:
        inline void EnforceState(ImageLoaderState state)
        {
            if (m_state != state) throw ref new Platform::COMException(WINCODEC_ERR_WRONGSTATE);
        }

        void LoadImageCommon(_In_ IWICBitmapSource* source);
        void CreateDeviceDependentResourcesInternal();
        void PopulateImageInfoACKind(_Inout_ ImageInfo* info, _In_ IWICBitmapSource* source);
        bool IsImageXboxHdrScreenshot(_In_ IWICBitmapSource* source);
        GUID TranslateDxgiFormatToWic(DXGI_FORMAT fmt);

        std::shared_ptr<DX::DeviceResources>                    m_deviceResources;

        // Device-independent
        Microsoft::WRL::ComPtr<IWICFormatConverter>             m_formatConvert;
        Microsoft::WRL::ComPtr<IWICColorContext>                m_wicColorContext;

        ImageLoaderState                                        m_state;
        ImageInfo                                               m_imageInfo;

        // Device-dependent
        Microsoft::WRL::ComPtr<ID2D1ImageSourceFromWic>         m_imageSource;
        Microsoft::WRL::ComPtr<ID2D1ColorContext>               m_colorContext;
    };
}