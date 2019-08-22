#pragma once
#include "ImageRenderer.g.h"

namespace winrt::DXRenderer::implementation
{
    struct ImageRenderer : ImageRendererT<ImageRenderer>
    {
        ImageRenderer() = default;

        ImageRenderer(Windows::UI::Xaml::Controls::SwapChainPanel const& panel);
        void OnDeviceLost();
        void OnDeviceRestored();
        void SetSwapChainPanel(Windows::UI::Xaml::Controls::SwapChainPanel const& panel);
        void SetLogicalSize(Windows::Foundation::Size const& logicalSize);
        void SetCurrentOrientation(Windows::Graphics::Display::DisplayOrientations const& currentOrientation);
        void SetDpi(float dpi);
        void SetCompositionScale(float compositionScaleX, float compositionScaleY);
        void ValidateDevice();
        void HandleDeviceLost();
        void Trim();
        void Present();
        void CreateDeviceIndependentResources();
        void CreateDeviceDependentResources();
        void CreateWindowSizeDependentResources();
        void ReleaseDeviceDependentResources();
        void Draw();
        void CreateImageDependentResources();
        void ReleaseImageDependentResources();
        void UpdateManipulationState(Windows::UI::Input::ManipulationUpdatedEventArgs const& args);
        void SetRenderOptions(DXRenderer::RenderEffectKind const& effect, float brightnessAdjustment, Windows::Graphics::Display::AdvancedColorInfo const& acInfo);
        void ExportImageToSdr(Windows::Storage::Streams::IRandomAccessStream const& outputStream, winrt::guid const& wicFormat);
    };
}
namespace winrt::DXRenderer::factory_implementation
{
    struct ImageRenderer : ImageRendererT<ImageRenderer, implementation::ImageRenderer>
    {
    };
}
