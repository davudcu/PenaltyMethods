#pragma once
#include "ChartCanvas.h"
#include <vector>

// ============================================================
// ConvergenceCanvas: side-by-side comparison of both methods
// on the same problem:
//   left : constraint violation ||h(x_k)||_inf per outer iteration
//   right: distance to the reference solution against the total
//          number of Newton iterations spent (work-accuracy view)
// ============================================================
namespace ui
{

class ConvergenceCanvas : public ChartCanvas
{
    static void violationSeries(const pm::SolveHistory& h, PlotSeries& s)
    {
        for (auto& r : h.outer) { s.x.push_back(r.k); s.y.push_back(r.hInf); }
    }

    static void errorSeries(const pm::SolveHistory& h, const pm::IProblem& p, PlotSeries& s)
    {
        const bool ref = h.hasReference;
        s.x.push_back(0);
        if (ref) s.y.push_back(p.solutionError(h.x0, h.xStar));
        else
        {
            pm::Vec g;
            p.gradF(h.x0, g);
            s.y.push_back(pm::normInf(g));
        }
        for (auto& r : h.outer)
        {
            s.x.push_back(r.cumulativeNewton);
            s.y.push_back(ref ? r.xError : r.kktNorm);
        }
    }


protected:
    void paint(const gui::Rect& r) override
    {
        if (!_result || _result->runs.empty()) { paintPlaceholder(r); return; }
        const pm::MethodComparison& mc = _result->primary();

        const double gap = 12;
        const double mid = (r.left + r.right) / 2;
        gui::Rect left(r.left + 6, r.top + 6, mid - gap / 2, r.bottom - 6);
        gui::Rect right(mid + gap / 2, r.top + 6, r.right - 6, r.bottom - 6);

        // --- constraint violation -------------------------------
        {
            std::vector<PlotSeries> series(2);
            series[0].style = kPenaltyStyle; series[0].label = tr("penalty");
            series[1].style = kALStyle;      series[1].label = tr("augLag");
            violationSeries(mc.penalty, series[0]);
            violationSeries(mc.al, series[1]);

            const double feasTol = _result->settings.penalty.outer.feasTol;
            Chart c(left, Scale::Linear, Scale::Log);
            c.setIntegerX(true);
            c.setLabels(fmt("%s %s", tr("convViolationTitle").c_str(), mc.problem->name().c_str()),
                        tr("outerIteration"),
                        fmt("%sh(x%s)%s%s", glyph::norm, "_k", glyph::norm, glyph::inf));
            c.fit(series, { feasTol });
            c.drawFrame();
            c.drawHLine(feasTol, kGuideColor, td::LinePattern::Dash, tr("feasTolShort"));
            for (auto& s : series) c.drawSeries(s);
            c.drawLegend(series, true, false);
        }

        // --- accuracy vs work -----------------------------------
        {
            std::vector<PlotSeries> series(2);
            series[0].style = kPenaltyStyle; series[0].label = tr("penalty");
            series[1].style = kALStyle;      series[1].label = tr("augLag");
            errorSeries(mc.penalty, *mc.problem, series[0]);
            errorSeries(mc.al, *mc.problem, series[1]);

            const bool ref = mc.penalty.hasReference;
            Chart c(right, Scale::Linear, Scale::Log);
            c.setIntegerX(true);
            c.setLabels(tr(ref ? "convErrorTitle" : "convKktTitle"),
                        tr("cumulativeNewton"),
                        ref ? fmt("%sx%s - x*%s", glyph::norm, "_k", glyph::norm)
                            : fmt("%s%sL%s%s", glyph::norm, glyph::nabla, glyph::norm, glyph::inf));
            c.fit(series);
            c.drawFrame();
            for (auto& s : series) c.drawSeries(s);
            c.drawLegend(series, true, false);
        }
    }
};

} // namespace ui
