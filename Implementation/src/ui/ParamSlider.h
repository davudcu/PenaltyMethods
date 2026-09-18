#pragma once
#include <gui/Label.h>
#include <gui/Slider.h>
#include <gui/GridComposer.h>
#include "Style.h"
#include <cmath>
#include <functional>
#include <algorithm>

// ============================================================
// ParamSlider: one "label | slider | value" row of the inspector.
//
//   Log10   : slider moves over the exponent, value = 10^e
//             (e snapped to `step`, e.g. 1 for tolerances)
//   Linear  : value snapped to `step`
//   Integer : whole numbers
// The value label always shows the snapped value that the solver
// will actually use.
// ============================================================
namespace ui
{

class ParamSlider
{
public:
    enum class Kind { Log10, Linear, Integer };

private:
    gui::Label  _label;
    gui::Slider _slider;
    gui::Label  _value;
    Kind   _kind;
    double _lo, _hi, _step;
    std::function<void()> _onChanged;

    double snapPosition(double pos) const
    {
        if (_step > 0) pos = _lo + std::round((pos - _lo) / _step) * _step;
        return (std::min)(_hi, (std::max)(_lo, pos));
    }

    double toValue(double pos) const
    {
        pos = snapPosition(pos);
        return _kind == Kind::Log10 ? std::pow(10.0, pos) : pos;
    }

    void updateLabel()
    {
        const double v = get();
        if (_kind == Kind::Log10)       _value.setTitle(fmt("%.0e", v));
        else if (_kind == Kind::Integer) _value.setTitle(fmt("%d", (int) std::lround(v)));
        else                             _value.setTitle(fmt("%g", v));
    }

public:
    // lo/hi/step are exponents for Log10, plain values otherwise
    ParamSlider(const td::String& label, const td::String& toolTip, Kind kind, double lo, double hi, double step)
    : _label(label)
    , _slider(toolTip)
    , _kind(kind)
    , _lo(lo), _hi(hi), _step(kind == Kind::Integer ? (std::max)(1.0, step) : step)
    {
        const int ticks = _step > 0 ? (int) std::lround((_hi - _lo) / _step) + 1 : -1;
        _slider.setRange(_lo, _hi, ticks);
        _slider.setSizeLimits(120, gui::Control::Limit::UseAsMin);
        _value.setSizeLimits(54, gui::Control::Limit::UseAsMin);
        _label.setToolTip(toolTip);
        _slider.onChangedValue([this]()
        {
            updateLabel();
            if (_onChanged) _onChanged();
        });
    }

    void appendTo(gui::GridComposer& gc)
    {
        gc.appendRow(_label) << _slider << _value;
    }

    void onChanged(const std::function<void()>& f) { _onChanged = f; }

    double get() const { return toValue(_slider.getValue()); }

    void set(double v)
    {
        double pos = v;
        if (_kind == Kind::Log10) pos = v > 0 ? std::log10(v) : _lo;
        _slider.setValue(snapPosition(pos), false);
        updateLabel();
    }

    void enable(bool on)
    {
        _label.enable(on);
        _slider.enable(on);
        _value.enable(on);
    }
};

} // namespace ui
