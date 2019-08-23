#pragma once
#include "ImageRenderer.g.h"

#include "Common\DeviceResources.h"
#include "ImageLoader.h"

namespace winrt::DXRenderer::implementation
{
    struct ImageRenderer : ImageRendererT<ImageRenderer>
    {
    public:
        ImageRenderer() = default;

        static void final_release(std::unique_ptr<winrt::DXRenderer::implementation::ImageRenderer> ptr) noexcept
        {
            // Deregister device notification.
            ptr->m_deviceResources->RegisterDeviceNotify(nullptr);
        }

        ImageRenderer(Windows::UI::Xaml::Controls::SwapChainPanel const& panel);

        // IDeviceNotify
        void OnDeviceLost();
        void OnDeviceRestored();

        // DeviceResources wrapper methods for Windows Runtime Component
        void SetSwapChainPanel(Windows::UI::Xaml::Controls::SwapChainPanel panel)   { m_deviceResources->SetSwapChainPanel(panel); }
        void SetLogicalSize(Windows::Foundation::Size logicalSize)                  { m_deviceResources->SetLogicalSize(logicalSize); }
        void SetCurrentOrientation(Windows::Graphics::Display::DisplayOrientations currentOrientation)
        {
            m_deviceResources->SetCurrentOrientation(currentOrientation);
        }
        void SetDpi(float dpi)                                                      { m_deviceResources->SetDpi(dpi); }
        void SetCompositionScale(float compositionScaleX, float compositionScaleY)  { m_deviceResources->SetCompositionScale(compositionScaleX, compositionScaleY); }
        void ValidateDevice()                                                       { m_deviceResources->ValidateDevice(); }
        void HandleDeviceLost()                                                     { m_deviceResources->HandleDeviceLost(); }
        void Trim()                                                                 { m_deviceResources->Trim(); }
        void Present()                                                              { m_deviceResources->Present(); }

        void CreateDeviceIndependentResources();
        void CreateDeviceDependentResources();
        void CreateWindowSizeDependentResources();
        void ReleaseDeviceDependentResources();
        void CreateImageDependentResources();
        void ReleaseImageDependentResources();

        void Draw();

        void UpdateManipulationState(Windows::UI::Input::ManipulationUpdatedEventArgs const& args);

        // Returns the computed MaxCLL and AvgCLL of the image in nits. While HDR metadata is a
        // property of the image (and is independent of rendering), our implementation
        // can't compute it until this point.
        winrt::DXRenderer::ImageCLL FitImageToWindow(bool computeMetadata);

        void SetRenderOptions(DXRenderer::RenderEffectKind const& effect, float brightnessAdjustment, Windows::Graphics::Display::AdvancedColorInfo const& acInfo);

        winrt::DXRenderer::ImageInfo LoadImageFromWic(Windows::Storage::Streams::IRandomAccessStream const& imageStream);
        winrt::DXRenderer::ImageInfo LoadImageFromDirectXTex(hstring const& filename, hstring const& extension);
        void ExportImageToSdr(Windows::Storage::Streams::IRandomAccessStream const& outputStream, winrt::guid const& wicFormat);

    private:
        inline static float Clamp(float v, float bound1, float bound2)
        {
            float low = min(bound1, bound2);
            float high = max(bound1, bound2);
            return (v < low) ? low : (v > high) ? high : v;
        }

        void CreateHistogramResources();
        void UpdateWhiteLevelScale(float brightnessAdjustment, float sdrWhiteLevel);
        void UpdateImageTransformState();
        void ComputeHdrMetadata();
        void EmitHdrMetadata();

        float GetBestDispMaxLuminance();

        // Cached pointer to device resources.
        std::shared_ptr<DXR::DeviceResources>           m_deviceResources;
        std::unique_ptr<DXR::ImageLoader>               m_imageLoader;

        // WIC and Direct2D resources.
        winrt::com_ptr<ID2D1TransformedImageSource>     m_loadedImage;
        winrt::com_ptr<ID2D1Effect>                     m_colorManagementEffect;
        winrt::com_ptr<ID2D1Effect>                     m_whiteScaleEffect;
        winrt::com_ptr<ID2D1Effect>                     m_sdrWhiteScaleEffect;
        winrt::com_ptr<ID2D1Effect>                     m_hdrTonemapEffect;
        winrt::com_ptr<ID2D1Effect>                     m_sdrOverlayEffect;
        winrt::com_ptr<ID2D1Effect>                     m_heatmapEffect;
        winrt::com_ptr<ID2D1Effect>                     m_sphereMapEffect;
        winrt::com_ptr<ID2D1Effect>                     m_histogramPrescale;
        winrt::com_ptr<ID2D1Effect>                     m_histogramEffect;
        winrt::com_ptr<ID2D1Effect>                     m_finalOutput;

        // Other renderer members.
        winrt::DXRenderer::RenderEffectKind                     m_renderEffectKind;
        float                                                   m_zoom;
        float                                                   m_minZoom;
        D2D1_POINT_2F                                           m_imageOffset;
        D2D1_POINT_2F                                           m_pointerPos;
        winrt::DXRenderer::ImageCLL                             m_imageCLL;
        float                                                   m_brightnessAdjust;
        winrt::Windows::Graphics::Display::AdvancedColorInfo    m_dispInfo{ nullptr };
        winrt::DXRenderer::ImageInfo                            m_imageInfo;
        bool                                                    m_isComputeSupported;
    };
}
namespace winrt::DXRenderer::factory_implementation
{
    struct ImageRenderer : ImageRendererT<ImageRenderer, implementation::ImageRenderer>
    {
    };
}
