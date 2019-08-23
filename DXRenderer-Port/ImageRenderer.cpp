#include "pch.h"
#include "ImageRenderer.h"
#include "ImageRenderer.g.cpp"

namespace winrt::DXRenderer::implementation
{
    ImageRenderer::ImageRenderer(Windows::UI::Xaml::Controls::SwapChainPanel const& panel)
    {
        throw hresult_not_implemented();
    }
    void ImageRenderer::OnDeviceLost()
    {
        throw hresult_not_implemented();
    }
    void ImageRenderer::OnDeviceRestored()
    {
        throw hresult_not_implemented();
    }
    void ImageRenderer::SetSwapChainPanel(Windows::UI::Xaml::Controls::SwapChainPanel const& panel)
    {
        throw hresult_not_implemented();
    }
    void ImageRenderer::SetLogicalSize(Windows::Foundation::Size const& logicalSize)
    {
        throw hresult_not_implemented();
    }
    void ImageRenderer::SetCurrentOrientation(Windows::Graphics::Display::DisplayOrientations const& currentOrientation)
    {
        throw hresult_not_implemented();
    }
    void ImageRenderer::SetDpi(float dpi)
    {
        throw hresult_not_implemented();
    }
    void ImageRenderer::SetCompositionScale(float compositionScaleX, float compositionScaleY)
    {
        throw hresult_not_implemented();
    }
    void ImageRenderer::ValidateDevice()
    {
        throw hresult_not_implemented();
    }
    void ImageRenderer::HandleDeviceLost()
    {
        throw hresult_not_implemented();
    }
    void ImageRenderer::Trim()
    {
        throw hresult_not_implemented();
    }
    void ImageRenderer::Present()
    {
        throw hresult_not_implemented();
    }
    void ImageRenderer::CreateDeviceIndependentResources()
    {
        throw hresult_not_implemented();
    }
    void ImageRenderer::CreateDeviceDependentResources()
    {
        throw hresult_not_implemented();
    }
    void ImageRenderer::CreateWindowSizeDependentResources()
    {
        throw hresult_not_implemented();
    }
    void ImageRenderer::ReleaseDeviceDependentResources()
    {
        throw hresult_not_implemented();
    }
    void ImageRenderer::Draw()
    {
        throw hresult_not_implemented();
    }
    void ImageRenderer::CreateImageDependentResources()
    {
        throw hresult_not_implemented();
    }
    void ImageRenderer::ReleaseImageDependentResources()
    {
        throw hresult_not_implemented();
    }
    void ImageRenderer::UpdateManipulationState(Windows::UI::Input::ManipulationUpdatedEventArgs const& args)
    {
        throw hresult_not_implemented();
    }
    DXRenderer::ImageCLL ImageRenderer::FitImageToWindow(bool computeMetadata)
    {
        throw hresult_not_implemented();
    }
    void ImageRenderer::SetRenderOptions(DXRenderer::RenderEffectKind const& effect, float brightnessAdjustment, Windows::Graphics::Display::AdvancedColorInfo const& acInfo)
    {
        throw hresult_not_implemented();
    }
    DXRenderer::ImageInfo ImageRenderer::LoadImageFromWic(Windows::Storage::Streams::IRandomAccessStream const& imageStream)
    {
        throw hresult_not_implemented();
    }
    DXRenderer::ImageInfo ImageRenderer::LoadImageFromDirectXTex(hstring const& filename, hstring const& extension)
    {
        throw hresult_not_implemented();
    }
    void ImageRenderer::ExportImageToSdr(Windows::Storage::Streams::IRandomAccessStream const& outputStream, winrt::guid const& wicFormat)
    {
        throw hresult_not_implemented();
    }
}
