#include "pch.h"
#include "EffectOption.h"
#include "EffectOption.g.cpp"

namespace winrt::DXRenderer::implementation
{
    EffectOption::EffectOption(hstring const& description, DXRenderer::RenderEffectKind const& kind)
    {
        this->m_description = description;
        this->m_kind = kind;
    }

    hstring EffectOption::Description()
    {
        return m_description;
    }

    DXRenderer::RenderEffectKind EffectOption::Kind()
    {
        return m_kind;
    }
}
