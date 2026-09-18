#pragma once
#include <gui/Canvas.h>
#include "../core/ComparisonRunner.h"
#include "Chart.h"
#include <memory>

// ============================================================
// ChartCanvas: base for every result view drawn on a canvas.
// Holds the shared, immutable run result and redraws on resize.
// Subclasses implement paint() for the full canvas rectangle.
// ============================================================
namespace ui
{

using ResultPtr = std::shared_ptr<const pm::RunResult>;

class ChartCanvas : public gui::Canvas
{
protected:
    ResultPtr _result;

    virtual void paint(const gui::Rect& r) = 0;

    void onDraw(const gui::Rect&) override
    {
        gui::Size sz;
        getSize(sz);
        const bool exporting = _backend != Backend::Display;
        PrintScope print(exporting);     // black text on white for PDF/SVG
        if (exporting) gui::Shape::drawRect(gui::Rect(0, 0, sz.width, sz.height), td::ColorID::White);
        paint(gui::Rect(0, 0, sz.width, sz.height));
    }

    void onResize(const gui::Size&) override
    {
        reDraw();
    }

    // "no data yet" hint centred in the canvas
    void paintPlaceholder(const gui::Rect& r) const
    {
        gui::DrawableString::draw(tr("noResult"), r, gui::Font::ID::SystemNormal, textColor(),
                                  td::TextAlignment::Center, td::VAlignment::Center);
    }

public:
    ChartCanvas()
    {
        enableResizeEvent(true);
        setSizeLimits(300, gui::Control::Limit::UseAsMin, 220, gui::Control::Limit::UseAsMin);
    }

    virtual void setResult(const ResultPtr& r)
    {
        _result = r;
        reDraw();
    }
};

} // namespace ui
