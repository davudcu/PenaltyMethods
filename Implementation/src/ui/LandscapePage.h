#pragma once
#include <gui/View.h>
#include <gui/Label.h>
#include <gui/Slider.h>
#include <gui/HorizontalLayout.h>
#include <gui/VerticalLayout.h>
#include "LandscapeCanvas.h"
#include <algorithm>
#include <cmath>

// ============================================================
// LandscapePage: landscape canvas plus a slider that picks the
// outer iteration k whose subproblem is shown.
// ============================================================
namespace ui
{

class LandscapePage : public gui::View
{
    gui::Label            _lblK;
    gui::Slider           _slK;
    gui::Label            _lblValue;
    gui::HorizontalLayout _hl;
    LandscapeCanvas       _canvas;
    gui::VerticalLayout   _vl;
    ResultPtr             _result;
    int                   _maxK = 0;

    void updateValueLabel(int k)
    {
        if (!_result || _result->runs.empty()) { _lblValue.setTitle(td::String("")); return; }
        const auto& mc = _result->primary();
        auto muAt = [k](const pm::SolveHistory& h)
        {
            if (h.outer.empty()) return 0.0;
            return h.outer[(size_t) (std::min)(k, (int) h.outer.size() - 1)].mu;
        };
        _lblValue.setTitle(fmt("k = %d     %s: %s = %.1e     %s: %s = %.1e", k,
                               tr("penaltyShort").c_str(), glyph::mu, muAt(mc.penalty),
                               tr("augLagShort").c_str(), glyph::mu, muAt(mc.al)));
    }

public:
    LandscapePage()
    : _lblK(tr("subproblemK"))
    , _slK(tr("subproblemKTT"))
    , _hl(3)
    , _vl(2)
    {
        _slK.setRange(0, 1);
        _slK.setValue(0.0, false);
        _slK.enable(false);
        _slK.setSizeLimits(160, gui::Control::Limit::UseAsMin);

        _slK.onChangedValue([this]()
        {
            const int k = (std::min)(_maxK, (std::max)(0, (int) std::lround(_slK.getValue())));
            _canvas.setOuterIndex(k);
            updateValueLabel(k);
        });

        _hl << _lblK << _slK << _lblValue;
        _vl << _hl << _canvas;
        setMargins(6, 6, 6, 6);
        setLayout(&_vl);
    }

    void setResult(const ResultPtr& r)
    {
        _result = r;
        _canvas.setResult(r);
        _maxK = 0;
        if (r && !r->runs.empty())
        {
            const auto& mc = r->primary();
            _maxK = (int) (std::max)(mc.penalty.outer.size(), mc.al.outer.size()) - 1;
        }
        _maxK = (std::max)(0, _maxK);
        _slK.setRange(0, (std::max)(1, _maxK), _maxK + 1);
        _slK.setValue(_maxK, false);
        _slK.enable(_maxK > 0);
        _canvas.setOuterIndex(_maxK);
        updateValueLabel(_maxK);
    }

    gui::Canvas& canvas() { return _canvas; }
};

} // namespace ui
