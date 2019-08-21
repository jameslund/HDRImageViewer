#include "pch.h"
#include "RenderOptionsViewModel.h"
#include "RenderOptionsViewModel.g.cpp"

using namespace winrt;

using namespace Windows::Foundation::Collections;

namespace winrt::DXRenderer::implementation
{
    RenderOptionsViewModel::RenderOptionsViewModel()
    {
        IVector<EffectOption> vector =
        {
            winrt::single_threaded_vector<EffectOption>({
                EffectOption(L"No effect",             RenderEffectKind::None),
                EffectOption(L"HDR tonemap",           RenderEffectKind::HdrTonemap),
                EffectOption(L"Draw SDR as grayscale", RenderEffectKind::SdrOverlay),
                EffectOption(L"Luminance heatmap",     RenderEffectKind::LuminanceHeatmap),

#ifdef _DEBUG
                // TODO: Sphere map is not ready for public release
                EffectOption(L"Draw as spheremap",     RenderEffectKind::SphereMap),
#endif
                })
        };

        m_renderEffects = vector.as<IVectorView<EffectOption>>();
    }

    IVectorView<EffectOption> RenderOptionsViewModel::RenderEffects()
    {
        return m_renderEffects;
    }
}
