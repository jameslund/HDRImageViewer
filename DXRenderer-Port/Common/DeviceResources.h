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

#pragma once

#include "pch.h"

using namespace winrt;

namespace DXR
{
    // Controls all the DirectX device resources.
    class DeviceResources
    {
    public:
        DeviceResources();
        void SetSwapChainPanel(Windows::UI::Xaml::Controls::SwapChainPanel const& panel);
        void SetLogicalSize(Windows::Foundation::Size logicalSize);
        void SetCurrentOrientation(Windows::Graphics::Display::DisplayOrientations currentOrientation);
        void SetDpi(float dpi);
        void SetCompositionScale(float compositionScaleX, float compositionScaleY);
        void ValidateDevice();
        void HandleDeviceLost();
        void RegisterDeviceNotify(winrt::DXRenderer::IDeviceNotify* deviceNotify);
        void Trim();
        void Present();

        // Device Accessors.
        Windows::Foundation::Size   GetOutputSize() const                   { return m_outputSize; }
        Windows::Foundation::Size   GetLogicalSize() const                  { return m_logicalSize; }
        float                       GetDpi() const                          { return m_dpi; }

        // D3D Accessors.
        ID3D11Device2*              GetD3DDevice() const                    { return m_d3dDevice.get(); }
        ID3D11DeviceContext3*       GetD3DDeviceContext() const             { return m_d3dContext.get(); }
        IDXGISwapChain3*            GetSwapChain() const                    { return m_swapChain.get(); }
        D3D_FEATURE_LEVEL           GetDeviceFeatureLevel() const           { return m_d3dFeatureLevel; }
        ID3D11RenderTargetView*     GetBackBufferRenderTargetView() const   { return m_d3dRenderTargetView.get(); }
        ID3D11DepthStencilView*     GetDepthStencilView() const             { return m_d3dDepthStencilView.get(); }
        D3D11_VIEWPORT              GetScreenViewport() const               { return m_screenViewport; }
        DirectX::XMFLOAT4X4         GetOrientationTransform3D() const       { return m_orientationTransform3D; }

        // D2D Accessors.
        ID2D1Factory6*              GetD2DFactory() const                   { return m_d2dFactory.get(); }
        ID2D1Device5*               GetD2DDevice() const                    { return m_d2dDevice.get(); }
        ID2D1DeviceContext5*        GetD2DDeviceContext() const             { return m_d2dContext.get(); }
        ID2D1Bitmap1*               GetD2DTargetBitmap() const              { return m_d2dTargetBitmap.get(); }
        IDWriteFactory2*            GetDWriteFactory() const                { return m_dwriteFactory.get(); }
        IWICImagingFactory2*        GetWicImagingFactory() const            { return m_wicFactory.get(); }
        D2D1::Matrix3x2F            GetOrientationTransform2D() const       { return m_orientationTransform2D; }

    private:
        void CreateDeviceIndependentResources();
        void CreateDeviceResources();
        void CreateWindowSizeDependentResources();
        DXGI_MODE_ROTATION ComputeDisplayRotation();

        // Direct3D objects.
        winrt::com_ptr<ID3D11Device2>           m_d3dDevice;
        winrt::com_ptr<ID3D11DeviceContext3>    m_d3dContext;
        winrt::com_ptr<IDXGIFactory2>           m_dxgiFactory;
        winrt::com_ptr<IDXGISwapChain3>         m_swapChain;

        // Direct3D rendering objects. Required for 3D.
        winrt::com_ptr<ID3D11RenderTargetView>  m_d3dRenderTargetView;
        winrt::com_ptr<ID3D11DepthStencilView>  m_d3dDepthStencilView;
        D3D11_VIEWPORT                          m_screenViewport;

        // Direct2D drawing components.
        winrt::com_ptr<ID2D1Factory6>           m_d2dFactory;
        winrt::com_ptr<ID2D1Device5>            m_d2dDevice;
        winrt::com_ptr<ID2D1DeviceContext5>     m_d2dContext;
        winrt::com_ptr<ID2D1Bitmap1>            m_d2dTargetBitmap;

        // DirectWrite drawing components.
        winrt::com_ptr<IDWriteFactory2>         m_dwriteFactory;
        winrt::com_ptr<IWICImagingFactory2>     m_wicFactory;

        // Cached reference to the XAML panel.
        Windows::UI::Xaml::Controls::SwapChainPanel     m_swapChainPanel;

        // Cached device properties.
        D3D_FEATURE_LEVEL                               m_d3dFeatureLevel;
        Windows::Foundation::Size                       m_d3dRenderTargetSize;
        Windows::Foundation::Size                       m_outputSize;
        Windows::Foundation::Size                       m_logicalSize;
        Windows::Graphics::Display::DisplayOrientations m_nativeOrientation;
        Windows::Graphics::Display::DisplayOrientations m_currentOrientation;
        float                                           m_dpi;
        float                                           m_compositionScaleX;
        float                                           m_compositionScaleY;

        // Transforms used for display orientation.
        D2D1::Matrix3x2F                                m_orientationTransform2D;
        DirectX::XMFLOAT4X4                             m_orientationTransform3D;

        // The winrt::DXRenderer:: can be held directly as it owns the DeviceResources.
        winrt::DXRenderer::IDeviceNotify*               m_deviceNotify;

        // Helper function to run on UI thread - only intended to be called from CreateDeviceResources().
        Windows::Foundation::IAsyncAction SetSwapChainAsync();
    };
}