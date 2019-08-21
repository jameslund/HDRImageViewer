#pragma once
#include "RenderOptionsViewModel.g.h"

namespace winrt::DXRenderer::implementation
{
    struct RenderOptionsViewModel : RenderOptionsViewModelT<RenderOptionsViewModel>
    {
    public:
        RenderOptionsViewModel();

        Windows::Foundation::Collections::IVectorView<DXRenderer::EffectOption> RenderEffects();

    private:
        Windows::Foundation::Collections::IVectorView<DXRenderer::EffectOption> m_renderEffects;

    };
}
namespace winrt::DXRenderer::factory_implementation
{
    struct RenderOptionsViewModel : RenderOptionsViewModelT<RenderOptionsViewModel, implementation::RenderOptionsViewModel>
    {
    };
}
