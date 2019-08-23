#include "pch.h"
#include "ImageRenderer.h"
#include "ImageRenderer.g.cpp"

#include "Common\DirectXHelper.h"
#include "DirectXTex.h"
#include "DirectXTex\DirectXTexEXR.h"
#include "ImageExporter.h"
#include "MagicConstants.h"
#include "RenderEffects\SimpleTonemapEffect.h"
#include "RenderEffects\SdrOverlayEffect.h"
#include "RenderEffects\LuminanceHeatmapEffect.h"
#include "RenderEffects\SphereMapEffect.h"

using namespace DXR;

using namespace winrt;

using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Input;

namespace winrt::DXRenderer::implementation
{
    ImageRenderer::ImageRenderer(Windows::UI::Xaml::Controls::SwapChainPanel const& panel) :
        m_renderEffectKind(RenderEffectKind::None),
        m_zoom(1.0f),
        m_minZoom(1.0f), // Dynamically calculated on window size.
        m_imageOffset(),
        m_pointerPos(),
        m_imageCLL{ -1.0f, -1.0f },
        m_brightnessAdjust(1.0f),
        m_imageInfo{},
        m_isComputeSupported(false)
    {
        // DeviceResources must be initialized first.
        // TODO: Current architecture does not allow multiple Renderers to share DeviceResources.
        m_deviceResources = std::make_shared<DeviceResources>();
        m_deviceResources->SetSwapChainPanel(panel);

        // Register to be notified if the GPU device is lost or recreated.
        winrt::DXRenderer::IDeviceNotify iface = winrt::make<ImageRenderer>();
        m_deviceResources->RegisterDeviceNotify(&iface);

        CreateDeviceIndependentResources();
        CreateDeviceDependentResources();
        CreateWindowSizeDependentResources();
    }

    // Notifies renderers that device resources need to be released.
    void ImageRenderer::OnDeviceLost()
    {
        ReleaseImageDependentResources();
        ReleaseDeviceDependentResources();
    }

    // Notifies renderers that device resources may now be recreated.
    void ImageRenderer::OnDeviceRestored()
    {
        CreateDeviceDependentResources();
        CreateImageDependentResources();
        CreateWindowSizeDependentResources();

        Draw();
    }

    void ImageRenderer::CreateDeviceIndependentResources()
    {
        auto fact = m_deviceResources->GetD2DFactory();

        // TODO: This instance never does anything as it gets overwritten upon image load.
        m_imageLoader = std::make_unique<ImageLoader>(m_deviceResources);

        // Register the custom render effects.
        CHK(SimpleTonemapEffect::Register(fact));
        CHK(SdrOverlayEffect::Register(fact));
        CHK(LuminanceHeatmapEffect::Register(fact));
        CHK(SphereMapEffect::Register(fact));
    }

    void ImageRenderer::CreateDeviceDependentResources()
    {
        // All this app's device-dependent resources also depend on
        // the loaded image, so they are all created in
        // CreateImageDependentResources.
    }

    void ImageRenderer::CreateWindowSizeDependentResources()
    {
        // Window size changes don't require recomputing image HDR metadata.
        FitImageToWindow(false);
    }

    void ImageRenderer::ReleaseDeviceDependentResources()
    {
        m_imageLoader->ReleaseDeviceDependentResources();
    }

    // Renders the loaded image with user-specified options.
    void ImageRenderer::Draw()
    {
        auto d2dContext = m_deviceResources->GetD2DDeviceContext();

        d2dContext->BeginDraw();

        d2dContext->Clear(D2D1::ColorF(D2D1::ColorF::Black));

        d2dContext->SetTransform(m_deviceResources->GetOrientationTransform2D());

        if (m_loadedImage)
        {
            d2dContext->DrawImage(m_finalOutput.get(), m_imageOffset);

            EmitHdrMetadata();
        }

        // We ignore D2DERR_RECREATE_TARGET here. This error indicates that the device
        // is lost. It will be handled during the next call to Present.
        HRESULT hr = d2dContext->EndDraw();
        if (hr != D2DERR_RECREATE_TARGET)
        {
            CHK(hr);
        }

        m_deviceResources->Present();
    }

    // Configures a Direct2D image pipeline, including source, color management, 
    // tonemapping, and white level, based on the loaded image.
    void ImageRenderer::CreateImageDependentResources()
    {
        auto context = m_deviceResources->GetD2DDeviceContext();

        // Next, configure the app's effect pipeline, consisting of a color management effect
        // followed by a tone mapping effect.

        CHK(context->CreateEffect(CLSID_D2D1ColorManagement, m_colorManagementEffect.put()));

        CHK(m_colorManagementEffect->SetValue(
                D2D1_COLORMANAGEMENT_PROP_QUALITY,
                D2D1_COLORMANAGEMENT_QUALITY_BEST));   // Required for floating point and DXGI color space support.

        // The color management effect takes a source color space and a destination color space,
        // and performs the appropriate math to convert images between them.
        CHK(m_colorManagementEffect->SetValue(
                D2D1_COLORMANAGEMENT_PROP_SOURCE_COLOR_CONTEXT,
                m_imageLoader->GetImageColorContext()));

        // The destination color space is the render target's (swap chain's) color space. This app uses an
        // FP16 swap chain, which requires the colorspace to be scRGB.
        com_ptr<ID2D1ColorContext1> destColorContext;
        CHK(context->CreateColorContextFromDxgiColorSpace(
                DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709, // scRGB
                destColorContext.put()));

        CHK(m_colorManagementEffect->SetValue(
                D2D1_COLORMANAGEMENT_PROP_DESTINATION_COLOR_CONTEXT,
                destColorContext.get()));

        // White level scale is used to multiply the color values in the image; this allows the user
        // to adjust the brightness of the image on an HDR display.
        CHK(context->CreateEffect(CLSID_D2D1ColorMatrix, m_whiteScaleEffect.put()));

        // Input to white level scale may be modified in SetRenderOptions.
        m_whiteScaleEffect->SetInputEffect(0, m_colorManagementEffect.get());

        // Set the actual matrix in SetRenderOptions.

        // Instantiate and cache all of the tonemapping/render effects.
        // Some effects are implemented as Direct2D custom effects; see the RenderEffects filter in the
        // Solution Explorer.

        GUID sdrWhiteScale = {};
        GUID tonemapper = {};
        if (CheckPlatformSupport(DXR::Win1809))
        {
            // HDR tonemapper and white level adjust are only available in 1809 and above.
            tonemapper = CLSID_D2D1HdrToneMap;
            sdrWhiteScale = CLSID_D2D1WhiteLevelAdjustment;
        }
        else
        {
            tonemapper = CLSID_CustomSimpleTonemapEffect;

            // For 1803, this effect should never actually be rendered. Invert is a good "sentinel".
            sdrWhiteScale = CLSID_D2D1Invert;
        }

        CHK(context->CreateEffect(tonemapper, m_hdrTonemapEffect.put()));
        CHK(context->CreateEffect(sdrWhiteScale, m_sdrWhiteScaleEffect.put()));
        CHK(context->CreateEffect(CLSID_CustomSdrOverlayEffect, m_sdrOverlayEffect.put()));
        CHK(context->CreateEffect(CLSID_CustomLuminanceHeatmapEffect, m_heatmapEffect.put()));
        CHK(context->CreateEffect(CLSID_CustomSphereMapEffect, m_sphereMapEffect.put()));

        // TEST: border effect to remove seam at the boundary of the image (subpixel sampling)
        // Unclear if we can force D2D_BORDER_MODE_HARD somewhere to avoid the seam.
        com_ptr<ID2D1Effect> border;
        CHK(context->CreateEffect(CLSID_D2D1Border, border.put()));
        border->SetValue(D2D1_BORDER_PROP_EDGE_MODE_X, D2D1_BORDER_EDGE_MODE_WRAP);
        border->SetValue(D2D1_BORDER_PROP_EDGE_MODE_Y, D2D1_BORDER_EDGE_MODE_WRAP);
        border->SetInputEffect(0, m_whiteScaleEffect.get());

        m_hdrTonemapEffect->SetInputEffect(0, m_whiteScaleEffect.get());
        m_sphereMapEffect->SetInputEffect(0, border.get());

        // SphereMap needs to know the pixel size of the image.
        CHK(m_sphereMapEffect->SetValue(
                SPHEREMAP_PROP_SCENESIZE,
                D2D1::SizeF(m_imageInfo.size.Width, m_imageInfo.size.Height)));

        // For the following effects, we want white level scale to be applied after
        // tonemapping (otherwise brightness adjustments will affect numerical values).
        m_heatmapEffect->SetInputEffect(0, m_colorManagementEffect.get());
        m_sdrOverlayEffect->SetInputEffect(0, m_colorManagementEffect.get());

        // The remainder of the Direct2D effect graph is constructed in SetRenderOptions based on the
        // selected RenderEffectKind.

        CreateHistogramResources();
    }

    void ImageRenderer::ReleaseImageDependentResources()
    {
        // TODO: This method is only called during device lost. In that situation,
        // m_imageLoader should not be reset. Confirm this is the only case we want to call this.

        m_loadedImage = { nullptr };
        m_colorManagementEffect = { nullptr };
        m_whiteScaleEffect = { nullptr };
        m_sdrWhiteScaleEffect = { nullptr };
        m_hdrTonemapEffect = { nullptr };
        m_sdrOverlayEffect = { nullptr };
        m_heatmapEffect = { nullptr };
        m_histogramPrescale = { nullptr };
        m_histogramEffect = { nullptr };
        m_finalOutput = { nullptr };
    }

    void ImageRenderer::UpdateManipulationState(ManipulationUpdatedEventArgs const& args)
    {
        Point position = args.Position();
        Point positionDelta = args.Delta().Translation;
        float zoomDelta = args.Delta().Scale;

        if (m_renderEffectKind == RenderEffectKind::SphereMap)
        {
            // For sphere map, panning and zooming is implemented in the effect.
            m_pointerPos.x += positionDelta.X;
            m_pointerPos.y += positionDelta.Y;

            D2D1_SIZE_F targetSize = m_deviceResources->GetD2DDeviceContext()->GetSize();

            // Normalize panning position to pixel dimensions of render target.
            auto x = m_pointerPos.x / targetSize.width;
            auto y = m_pointerPos.y / targetSize.height;

            CHK(m_sphereMapEffect->SetValue(SPHEREMAP_PROP_CENTER, D2D1::Point2F(x, y)));

            m_zoom *= zoomDelta;
            m_zoom = Clamp(m_zoom, sc_MinZoomSphereMap, sc_MaxZoom);

            CHK(m_sphereMapEffect->SetValue(SPHEREMAP_PROP_ZOOM, m_zoom));
        }
        else
        {
            // Normal image pan/zoom for all other render effects.
            m_imageOffset.x += positionDelta.X;
            m_imageOffset.y += positionDelta.Y;

            // We want to have any zoom operation be "centered" around the pointer position, which
            // requires recalculating the view position based on the new zoom and pointer position.
            // Step 1: Calculate the absolute pointer position (image position).
            D2D1_POINT_2F pointerAbsolutePosition = D2D1::Point2F(
                (m_imageOffset.x - position.X) / m_zoom,
                (m_imageOffset.y - position.Y) / m_zoom);

            // Step 2: Apply the zoom; do not allow user to go beyond max zoom level.
            m_zoom *= zoomDelta;
            m_zoom = min(m_zoom, sc_MaxZoom);

            // Step 3: Adjust the view position based on the new m_zoom value.
            m_imageOffset.x = pointerAbsolutePosition.x * m_zoom + position.X;
            m_imageOffset.y = pointerAbsolutePosition.y * m_zoom + position.Y;

            // Step 4: Clamp the translation to the window bounds.
            Size panelSize = m_deviceResources->GetLogicalSize();
            m_imageOffset.x = Clamp(m_imageOffset.x, panelSize.Width - m_imageInfo.size.Width * m_zoom, 0);
            m_imageOffset.y = Clamp(m_imageOffset.y, panelSize.Height - m_imageInfo.size.Height * m_zoom, 0);

            UpdateImageTransformState();
        }

        Draw();
    }

    // Overrides any pan/zoom state set by the user to fit image to the window size.
    // Returns the computed content light level (CLL) of the image in nits.
    // Recomputing the HDR metadata is only needed when loading a new image.
    DXRenderer::ImageCLL ImageRenderer::FitImageToWindow(bool computeMetadata)
    {
        // TODO: Suspect this sometimes crashes due to AV. Need to root cause.
        if (m_imageLoader != nullptr &&
            m_imageLoader->GetState() == ImageLoaderState::LoadingSucceeded)
        {
            Size panelSize = m_deviceResources->GetLogicalSize();

            // Set image to be letterboxed in the window, up to the max allowed scale factor.
            float letterboxZoom = min(
                panelSize.Width / m_imageInfo.size.Width,
                panelSize.Height / m_imageInfo.size.Height);

            m_zoom = min(sc_MaxZoom, letterboxZoom);

            // SphereMap needs to know the pixel size of the image.
            CHK(m_sphereMapEffect->SetValue(
                    SPHEREMAP_PROP_SCENESIZE,
                    D2D1::SizeF(m_imageInfo.size.Width * m_zoom, m_imageInfo.size.Height * m_zoom)));

            // Center the image.
            m_imageOffset = D2D1::Point2F(
                (panelSize.Width - (m_imageInfo.size.Width * m_zoom)) / 2.0f,
                (panelSize.Height - (m_imageInfo.size.Height * m_zoom)) / 2.0f
            );

            UpdateImageTransformState();

            if (computeMetadata)
            {
                // HDR metadata is supposed to be independent of any rendering options, but
                // we can't compute it until the full effect graph is hooked up, which is here.
                ComputeHdrMetadata();
            }
        }

        return m_imageCLL;
    }

    // White level scale is used to multiply the color values in the image; allows the user to
    // adjust the brightness of the image on an HDR display.
    void ImageRenderer::SetRenderOptions(
        DXRenderer::RenderEffectKind const& effect,
        float brightnessAdjustment,
        AdvancedColorInfo const& acInfo)
    {
        m_dispInfo = acInfo;
        m_renderEffectKind = effect;
        m_brightnessAdjust = brightnessAdjustment;

        auto sdrWhite = m_dispInfo ? m_dispInfo.SdrWhiteLevelInNits() : sc_nominalRefWhite;

        UpdateWhiteLevelScale(m_brightnessAdjust, sdrWhite);

        // Adjust the Direct2D effect graph based on RenderEffectKind.
        // Some RenderEffectKind values require us to apply brightness adjustment
        // after the effect as their numerical output is affected by any luminance boost.
        switch (m_renderEffectKind)
        {
            // Effect graph: ImageSource > ColorManagement > WhiteScale > HDRTonemap > WhiteScale2*
        case RenderEffectKind::HdrTonemap:
            if (m_dispInfo.CurrentAdvancedColorKind() != AdvancedColorKind::HighDynamicRange)
            {
                // *Second white scale is needed as an integral part of using the Direct2D HDR
                // tonemapper on SDR/WCG displays to stay within [0, 1] numeric range.
                m_finalOutput.copy_from(m_sdrWhiteScaleEffect.get());
            }
            else
            {
                m_finalOutput.copy_from(m_hdrTonemapEffect.get());
            }

            m_sdrWhiteScaleEffect->SetInputEffect(0, m_hdrTonemapEffect.get());
            m_whiteScaleEffect->SetInputEffect(0, m_colorManagementEffect.get());
            break;

            // Effect graph: ImageSource > ColorManagement > WhiteScale
        case RenderEffectKind::None:
            m_finalOutput.copy_from(m_whiteScaleEffect.get());
            m_whiteScaleEffect->SetInputEffect(0, m_colorManagementEffect.get());
            break;

            // Effect graph: ImageSource > ColorManagement > Heatmap > WhiteScale
        case RenderEffectKind::LuminanceHeatmap:
            m_finalOutput.copy_from(m_whiteScaleEffect.get());
            m_whiteScaleEffect->SetInputEffect(0, m_heatmapEffect.get());
            break;

            // Effect graph: ImageSource > ColorManagement > SdrOverlay > WhiteScale
        case RenderEffectKind::SdrOverlay:
            m_finalOutput.copy_from(m_whiteScaleEffect.get());
            m_whiteScaleEffect->SetInputEffect(0, m_sdrOverlayEffect.get());
            break;

            // Effect graph: ImageSource > ColorManagement > WhiteScale > SphereMap
        case RenderEffectKind::SphereMap:
            m_finalOutput.copy_from(m_sphereMapEffect.get());
            m_whiteScaleEffect->SetInputEffect(0, m_colorManagementEffect.get());
            break;

        default:
            winrt::throw_hresult(E_NOTIMPL);
            break;
        }

        float targetMaxNits = GetBestDispMaxLuminance();

        // Update HDR tonemappers with display information.
        // The 1803 custom tonemapper uses mostly the same property definitions as the 1809 Direct2D tonemapper, for simplicity.
        CHK(m_hdrTonemapEffect->SetValue(D2D1_HDRTONEMAP_PROP_OUTPUT_MAX_LUMINANCE, targetMaxNits));

        float maxCLL = m_imageCLL.maxNits != -1.0f ? m_imageCLL.maxNits : sc_DefaultImageMaxCLL;
        maxCLL *= m_brightnessAdjust;

        // Very low input max luminance can produce unexpected rendering behavior. Restrict to
        // a reasonable level - the Direct2D tonemapper performs nearly a no-op if input < output max nits.
        maxCLL = max(maxCLL, 0.5f * targetMaxNits);

        CHK(m_hdrTonemapEffect->SetValue(D2D1_HDRTONEMAP_PROP_INPUT_MAX_LUMINANCE, maxCLL));

        // The 1809 Direct2D tonemapper optimizes for HDR or SDR displays; the 1803 custom tonemapper ignores this hint.
        D2D1_HDRTONEMAP_DISPLAY_MODE mode =
            m_dispInfo.CurrentAdvancedColorKind() == AdvancedColorKind::HighDynamicRange ?
            D2D1_HDRTONEMAP_DISPLAY_MODE_HDR : D2D1_HDRTONEMAP_DISPLAY_MODE_SDR;

        CHK(m_hdrTonemapEffect->SetValue(D2D1_HDRTONEMAP_PROP_DISPLAY_MODE, mode));

        // If an HDR tonemapper is used on an SDR or WCG display, perform additional white level correction.
        if (m_dispInfo.CurrentAdvancedColorKind() != AdvancedColorKind::HighDynamicRange)
        {
            // Both the D2D and custom HDR tonemappers output values in scRGB using scene-referred luminance -  a typical SDR display will
            // be around numeric range [0.0, 3.0] corresponding to [0, 240 nits]. To encode correctly for an SDR/WCG display
            // output, we must reinterpret the scene-referred input content (80 nits) as display-referred (targetMaxNits).
            CHK(m_sdrWhiteScaleEffect->SetValue(D2D1_WHITELEVELADJUSTMENT_PROP_INPUT_WHITE_LEVEL, D2D1_SCENE_REFERRED_SDR_WHITE_LEVEL));
            CHK(m_sdrWhiteScaleEffect->SetValue(D2D1_WHITELEVELADJUSTMENT_PROP_OUTPUT_WHITE_LEVEL, targetMaxNits));
        }

        Draw();
    }

    DXRenderer::ImageInfo ImageRenderer::LoadImageFromWic(IRandomAccessStream const& imageStream)
    {
        com_ptr<IStream> iStream;
        // TODO: [c++/winrt type].as<::IUnknown>().get(), really??
        CHK(CreateStreamOverRandomAccessStream(imageStream.as<::IUnknown>().get(), IID_PPV_ARGS(&iStream)));

        m_imageLoader = std::make_unique<ImageLoader>(m_deviceResources);
        m_imageInfo = m_imageLoader->LoadImageFromWic(iStream.get());
        return m_imageInfo;
    }

    DXRenderer::ImageInfo ImageRenderer::LoadImageFromDirectXTex(hstring const& filename, hstring const& extension)
    {
        m_imageLoader = std::make_unique<ImageLoader>(m_deviceResources);
        m_imageInfo = m_imageLoader->LoadImageFromDirectXTex(filename.c_str(), extension.c_str());
        return m_imageInfo;
    }

    void ImageRenderer::ExportImageToSdr(IRandomAccessStream const& outputStream, guid const& wicFormat)
    {
        com_ptr<IStream> iStream;
        CHK(CreateStreamOverRandomAccessStream(outputStream.as<::IUnknown>().get(), IID_PPV_ARGS(&iStream)));

        ImageExporter::ExportToSdr(m_imageLoader.get(), m_deviceResources.get(), iStream.get(), wicFormat);
    }

    // Perform histogram pipeline setup; this should occur as part of image resource creation.
    // Histogram results in no visual output but is used to calculate HDR metadata for the image.
    void ImageRenderer::CreateHistogramResources()
    {
        auto context = m_deviceResources->GetD2DDeviceContext();

        // We need to preprocess the image data before running the histogram.
        // 1. Spatial downscale to reduce the amount of processing needed.
        CHK(context->CreateEffect(CLSID_D2D1Scale, m_histogramPrescale.put()));

        CHK(m_histogramPrescale->SetValue(D2D1_SCALE_PROP_SCALE, D2D1::Vector2F(0.5f, 0.5f)));

        // The right place to compute HDR metadata is after color management to the
        // image's native colorspace but before any tonemapping or adjustments for the display.
        m_histogramPrescale->SetInputEffect(0, m_colorManagementEffect.get());

        // 2. Convert scRGB data into luminance (nits).
        // 3. Normalize color values. Histogram operates on [0-1] numeric range,
        //    while FP16 can go up to 65504 (5+ million nits).
        // Both steps are performed in the same color matrix.
        com_ptr<ID2D1Effect> histogramMatrix;
        CHK(context->CreateEffect(CLSID_D2D1ColorMatrix, histogramMatrix.put()));

        histogramMatrix->SetInputEffect(0, m_histogramPrescale.get());

        float scale = sc_histMaxNits / sc_nominalRefWhite;

        // 3a. In Windows 10 version 1803, the color management effect does not perform
        // reference white scaling when converting from HDR10 to scRGB (as used by Xbox HDR screenshots).
        // HDR10 image data is encoded as [0, 1] UNORM values, which represents [0, 10000] nits.
        // This should be converted to scRGB [0, 125] FP16 values (10000 / 80 nits reference), but
        // instead remains as scRGB [0, 1] FP16 values, or [0, 80] nits.
        // This is fixed in Windows 10 version 1809.
        if (m_imageInfo.isXboxHdrScreenshot && !CheckPlatformSupport(DXR::Win1809))
        {
            scale /= 125.0f;
        }

        D2D1_MATRIX_5X4_F rgbtoYnorm = D2D1::Matrix5x4F(
            0.2126f / scale, 0, 0, 0,
            0.7152f / scale, 0, 0, 0,
            0.0722f / scale, 0, 0, 0,
            0, 0, 0, 1,
            0, 0, 0, 0);
        // 1st column: [R] output, contains normalized Y (CIEXYZ).
        // 2nd column: [G] output, unused.
        // 3rd column: [B] output, unused.
        // 4th column: [A] output, alpha passthrough.
        // We explicitly calculate Y; this deviates from the CEA 861.3 definition of MaxCLL
        // which approximates luminance with max(R, G, B).

        CHK(histogramMatrix->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, rgbtoYnorm));

        // 4. Apply a gamma to allocate more histogram bins to lower luminance levels.
        com_ptr<ID2D1Effect> histogramGamma;
        CHK(context->CreateEffect(CLSID_D2D1GammaTransfer, histogramGamma.put()));

        histogramGamma->SetInputEffect(0, histogramMatrix.get());

        // Gamma function offers an acceptable tradeoff between simplicity and efficient bin allocation.
        // A more sophisticated pipeline would use a more perceptually linear function than gamma.
        CHK(histogramGamma->SetValue(D2D1_GAMMATRANSFER_PROP_RED_EXPONENT, sc_histGamma));
        // All other channels are passthrough.
        CHK(histogramGamma->SetValue(D2D1_GAMMATRANSFER_PROP_GREEN_DISABLE, TRUE));
        CHK(histogramGamma->SetValue(D2D1_GAMMATRANSFER_PROP_BLUE_DISABLE, TRUE));
        CHK(histogramGamma->SetValue(D2D1_GAMMATRANSFER_PROP_ALPHA_DISABLE, TRUE));

        // 5. Finally, the histogram itself.
        HRESULT hr = context->CreateEffect(CLSID_D2D1Histogram, m_histogramEffect.put());

        if (hr == D2DERR_INSUFFICIENT_DEVICE_CAPABILITIES)
        {
            // The GPU doesn't support compute shaders and we can't run histogram on it.
            m_isComputeSupported = false;
        }
        else
        {
            CHK(hr);
            m_isComputeSupported = true;

            CHK(m_histogramEffect->SetValue(D2D1_HISTOGRAM_PROP_NUM_BINS, sc_histNumBins));

            m_histogramEffect->SetInputEffect(0, histogramGamma.get());
        }
    }

    // When connected to an HDR display, the OS renders SDR content (e.g. 8888 UNORM) at
    // a user configurable white level; this typically is around 200-300 nits. It is the responsibility
    // of an advanced color app (e.g. FP16 scRGB) to emulate the OS-implemented SDR white level adjustment,
    // BUT only for non-HDR content (SDR or WCG).
    void ImageRenderer::UpdateWhiteLevelScale(float brightnessAdjustment, float sdrWhiteLevel)
    {
        float scale = 1.0f;

        switch (m_imageInfo.imageKind)
        {
        case AdvancedColorKind::HighDynamicRange:
            // HDR content should not be compensated by the SdrWhiteLevel parameter.
            scale = 1.0f;
            break;

        case AdvancedColorKind::StandardDynamicRange:
        case AdvancedColorKind::WideColorGamut:
        default:
            scale = sdrWhiteLevel / sc_nominalRefWhite;
            break;
        }

        // In Windows 10 version 1803, the color management effect does not perform
        // reference white scaling when converting from HDR10 to scRGB (as used by Xbox HDR screenshots).
        // HDR10 image data is encoded as [0, 1] UNORM values, which represents [0, 10000] nits.
        // This should be converted to scRGB [0, 125] FP16 values (10000 / 80 nits reference), but
        // instead remains as scRGB [0, 1] FP16 values, or [0, 80] nits.
        // This is fixed in Windows 10 version 1809.
        if (m_imageInfo.isXboxHdrScreenshot && !CheckPlatformSupport(DXR::Win1809))
        {
            scale *= 125.0f;
        }

        // The user may want to manually adjust brightness specifically for this image, on top of any
        // white level adjustment for SDR/WCG content. Brightness adjustment using a linear gamma scale
        // is mainly useful for HDR displays, but can be useful for HDR content tonemapped to an SDR/WCG display.
        scale *= brightnessAdjustment;

        // SDR white level scaling is performing by multiplying RGB color values in linear gamma.
        // We implement this with a Direct2D matrix effect.
        D2D1_MATRIX_5X4_F matrix = D2D1::Matrix5x4F(
            scale, 0, 0, 0,  // [R] Multiply each color channel
            0, scale, 0, 0,  // [G] by the scale factor in 
            0, 0, scale, 0,  // [B] linear gamma space.
            0, 0, 0,     1,  // [A] Preserve alpha values.
            0, 0, 0,     0); //     No offset.

        CHK(m_whiteScaleEffect->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, matrix));
    }

    // Call this after updating any spatial transform state to regenerate the effect graph.
    void ImageRenderer::UpdateImageTransformState()
    {
        if (m_imageLoader->GetState() == ImageLoaderState::LoadingSucceeded)
        {
            // Set the new image as the new source to the effect pipeline.
            m_loadedImage.copy_from(m_imageLoader->GetLoadedImage(m_zoom));
            m_colorManagementEffect->SetInput(0, m_loadedImage.get());
        }
    }

    // Uses a histogram to compute a modified version of maximum content light level/ST.2086 MaxCLL
    // and average content light level.
    // Performs Begin/EndDraw on the D2D context.
    void ImageRenderer::ComputeHdrMetadata()
    {
        // Initialize with a sentinel value.
        m_imageCLL = { -1.0f, -1.0f };

        // HDR metadata is not meaningful for SDR or WCG images.
        if ((!m_isComputeSupported) ||
            (m_imageInfo.imageKind != AdvancedColorKind::HighDynamicRange))
        {
            return;
        }

        // MaxCLL is nominally calculated for the single brightest pixel in a frame.
        // But we take a slightly more conservative definition that takes the 99.99th percentile
        // to account for extreme outliers in the image.
        float maxCLLPercent = 0.9999f;

        auto ctx = m_deviceResources->GetD2DDeviceContext();

        ctx->BeginDraw();

        ctx->DrawImage(m_histogramEffect.get());

        // We ignore D2DERR_RECREATE_TARGET here. This error indicates that the device
        // is lost. It will be handled during the next call to Present.
        HRESULT hr = ctx->EndDraw();
        if (hr != D2DERR_RECREATE_TARGET)
        {
            CHK(hr);
        }

        float* histogramData = new float[sc_histNumBins];
        CHK(m_histogramEffect->GetValue(D2D1_HISTOGRAM_PROP_HISTOGRAM_OUTPUT,
                reinterpret_cast<BYTE*>(histogramData),
                sc_histNumBins * sizeof(float)));

        unsigned int maxCLLbin = 0;
        unsigned int avgCLLbin = 0; // Average is defined as 50th percentile.
        float runningSum = 0.0f; // Cumulative sum of values in histogram is 1.0.
        for (int i = sc_histNumBins - 1; i >= 0; i--)
        {
            runningSum += histogramData[i];

            // Note the inequality (<) is the opposite of the next if block.
            if (runningSum < 1.0f - maxCLLPercent)
            {
                maxCLLbin = i;
            }

            if (runningSum > 0.5f)
            {
                // Note if the entire histogram is 0, avgCLLbin remains at -1.
                avgCLLbin = i;
                break;
            }
        }

        float binNormMax = static_cast<float>(maxCLLbin) / static_cast<float>(sc_histNumBins);
        m_imageCLL.maxNits = powf(binNormMax, 1 / sc_histGamma) * sc_histMaxNits;

        float binNormAvg = static_cast<float>(avgCLLbin) / static_cast<float>(sc_histNumBins);
        m_imageCLL.medNits = powf(binNormAvg, 1 / sc_histGamma) * sc_histMaxNits;

        // Some drivers have a bug where histogram will always return 0. Or some images are pure black.
        // Treat these cases as unknown.
        if (m_imageCLL.maxNits == 0.0f)
        {
            m_imageCLL = { -1.0f, -1.0f };
        }

        // HDR metadata computation is completed before the app rendering options are known, so don't
        // attempt to draw yet.
    }

    // Set HDR10 metadata to allow HDR displays to optimize behavior based on our content.
    // Note that as of Windows 10 1903, HDR metadata is only forwarded to the display with DirectFlip or IndependentFlip:
    // https://docs.microsoft.com/en-us/windows/win32/direct3ddxgi/for-best-performance--use-dxgi-flip-model#directflip
    void ImageRenderer::EmitHdrMetadata()
    {
        auto acKind = m_dispInfo ? m_dispInfo.CurrentAdvancedColorKind() : AdvancedColorKind::StandardDynamicRange;

        if (acKind == AdvancedColorKind::HighDynamicRange)
        {
            DXGI_HDR_METADATA_HDR10 metadata = {};

            // This sample doesn't do any chrominance (e.g. xy) gamut mapping, so just use default
            // color primaries values; a more sophisticated app will explicitly set these.
            // DXGI_HDR_METADATA_HDR10 defines primaries as 1/50000 of a unit in xy space.
            metadata.RedPrimary[0]   = static_cast<UINT16>(m_dispInfo.RedPrimary().X   * 50000.0f);
            metadata.RedPrimary[1]   = static_cast<UINT16>(m_dispInfo.RedPrimary().Y   * 50000.0f);
            metadata.GreenPrimary[0] = static_cast<UINT16>(m_dispInfo.GreenPrimary().X * 50000.0f);
            metadata.GreenPrimary[1] = static_cast<UINT16>(m_dispInfo.GreenPrimary().Y * 50000.0f);
            metadata.BluePrimary[0]  = static_cast<UINT16>(m_dispInfo.BluePrimary().X  * 50000.0f);
            metadata.BluePrimary[1]  = static_cast<UINT16>(m_dispInfo.BluePrimary().Y  * 50000.0f);
            metadata.WhitePoint[0]   = static_cast<UINT16>(m_dispInfo.WhitePoint().X   * 50000.0f);
            metadata.WhitePoint[1]   = static_cast<UINT16>(m_dispInfo.WhitePoint().Y   * 50000.0f);

            float effectiveMaxCLL = 0;

            switch (m_renderEffectKind)
            {
            case RenderEffectKind::None:
                effectiveMaxCLL = max(m_imageCLL.maxNits, 0.0f) * m_brightnessAdjust;
                break;

            case RenderEffectKind::HdrTonemap:
                effectiveMaxCLL = GetBestDispMaxLuminance() * m_brightnessAdjust;
                break;

            default:
                effectiveMaxCLL = m_dispInfo.SdrWhiteLevelInNits() * m_brightnessAdjust;
                break;
            }

            // DXGI_HDR_METADATA_HDR10 defines MaxCLL in integer nits.
            metadata.MaxContentLightLevel = static_cast<UINT16>(effectiveMaxCLL);

            // The luminance analysis doesn't calculate MaxFrameAverageLightLevel. We also don't have mastering
            // information (i.e. reference display in a studio), so Min/MaxMasteringLuminance is not relevant.
            // Leave these values as 0.

            auto sc = m_deviceResources->GetSwapChain();

            com_ptr<IDXGISwapChain4> sc4;
            CHK(sc->QueryInterface(IID_PPV_ARGS(&sc4)));
            CHK(sc4->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_HDR10, sizeof(metadata), &metadata));
        }
    }

    float ImageRenderer::GetBestDispMaxLuminance()
    {
        float val = m_dispInfo.MaxLuminanceInNits();

        if (val == 0.0f)
        {
            if (m_dispInfo.CurrentAdvancedColorKind() == AdvancedColorKind::HighDynamicRange)
            {
                // HDR TVs generally don't report metadata, but monitors do.
                val = sc_DefaultHdrDispMaxNits;
            }
            else
            {
                // Almost no SDR displays report HDR metadata. WCG displays generally should report HDR metadata.
                // We assume both SDR and WCG displays have similar peak luminances and use the same constants.
                val = sc_DefaultSdrDispMaxNits;
            }
        }

        return val;
    }
}
