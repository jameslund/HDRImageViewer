#pragma once
#include "RenderOptionsViewModel.g.h"

namespace winrt::DXRenderer::implementation
{
    struct RenderOptionsViewModel : RenderOptionsViewModelT<RenderOptionsViewModel>
    {
        RenderOptionsViewModel() = default;

        Windows::Foundation::Collections::IVectorView<DXRenderer::EffectOption> RenderEffects();
    };
}
namespace winrt::DXRenderer::factory_implementation
{
    struct RenderOptionsViewModel : RenderOptionsViewModelT<RenderOptionsViewModel, implementation::RenderOptionsViewModel>
    {
    };
}
