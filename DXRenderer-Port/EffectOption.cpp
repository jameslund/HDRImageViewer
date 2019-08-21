#include "pch.h"
#include "EffectOption.h"
#include "EffectOption.g.cpp"

namespace winrt::DXRenderer::implementation
{
    EffectOption::EffectOption(hstring const& description, DXRenderer::RenderEffectKind const& kind)
    {
        throw hresult_not_implemented();
    }
    hstring EffectOption::Description()
    {
        throw hresult_not_implemented();
    }
    DXRenderer::RenderEffectKind EffectOption::Kind()
    {
        throw hresult_not_implemented();
    }
}
