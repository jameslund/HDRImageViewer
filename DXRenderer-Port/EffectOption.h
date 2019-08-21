#pragma once
#include "EffectOption.g.h"

namespace winrt::DXRenderer::implementation
{
    struct EffectOption : EffectOptionT<EffectOption>
    {
        EffectOption() = default;

        EffectOption(hstring const& description, DXRenderer::RenderEffectKind const& kind);
        hstring Description();
        DXRenderer::RenderEffectKind Kind();
    };
}
namespace winrt::DXRenderer::factory_implementation
{
    struct EffectOption : EffectOptionT<EffectOption, implementation::EffectOption>
    {
    };
}
