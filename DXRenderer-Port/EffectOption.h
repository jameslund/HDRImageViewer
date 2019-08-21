#pragma once
#include "EffectOption.g.h"

namespace winrt::DXRenderer::implementation
{
    struct EffectOption : EffectOptionT<EffectOption>
    {
    public:
        EffectOption() = default;

        EffectOption(hstring const& description, DXRenderer::RenderEffectKind const& kind);

        hstring Description();

        DXRenderer::RenderEffectKind Kind();

    private:
        hstring                         m_description;
        DXRenderer::RenderEffectKind    m_kind;
    };
}
namespace winrt::DXRenderer::factory_implementation
{
    struct EffectOption : EffectOptionT<EffectOption, implementation::EffectOption>
    {
    };
}
