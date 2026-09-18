#pragma once
#include "ChartCanvas.h"
#include <cmath>
#include <vector>

// ============================================================
// ConditioningCanvas: the ill-conditioning analysis.
//
//   top-left    : cond(Hessian of merit) vs mu, both methods on the
//                 same log-log axes, with an O(mu) guide line.
//                 After "Run all problems" every problem is overlaid.
//   top-right   : cond vs outer iteration k
//   bottom-left : Newton iterations needed per subproblem
//   bottom-right: gradient norm Newton actually reached; it rises
//                 with mu for the penalty method once rounding in
//                 mu * h(x) dominates
// ============================================================
namespace ui
{

class ConditioningCanvas : public ChartCanvas
{
    template <typename F>
    static PlotSeries series(const pm::SolveHistory& h, const SeriesStyle& st, const td::String& label, F yOf, bool xIsMu)
    {
        PlotSeries s;
        s.style = st;
        s.label = label;
        for (auto& r : h.outer)
        {
            s.x.push_back(xIsMu ? r.mu : (double) r.k);
            s.y.push_back(yOf(r));
        }
        return s;
    }

    void paintCondVsMu(const gui::Rect& rect)
    {
        const auto& runs = _result->runs;
        std::vector<PlotSeries> all;
        auto cond = [](const pm::OuterRecord& r) { return r.cond; };

        if (_result->sweep && runs.size() > 1)
        {
            for (size_t i = 0; i < runs.size(); ++i)
            {
                SeriesStyle pen = kPenaltyStyle; pen.color = problemColor(i);
                SeriesStyle al = kALStyle;       al.color = problemColor(i); al.pattern = td::LinePattern::Dash;
                all.push_back(series(runs[i].penalty, pen, td::String(runs[i].problem->name().c_str()), cond, true));
                all.push_back(series(runs[i].al, al, td::String(), cond, true));
            }
        }
        else
        {
            const auto& mc = _result->primary();
            all.push_back(series(mc.penalty, kPenaltyStyle, tr("penalty"), cond, true));
            all.push_back(series(mc.al, kALStyle, tr("augLag"), cond, true));
        }

        // O(mu) guide through the last penalty point of the primary run
        PlotSeries guide;
        guide.style = { kGuideColor, td::LinePattern::Dash, 1.2f, false };
        guide.markers = false;
        guide.label = fmt("O(%s)", glyph::mu);
        const auto& pen = _result->primary().penalty;
        if (!pen.outer.empty())
        {
            const auto& a = pen.outer.front();
            const auto& b = pen.outer.back();
            if (std::isfinite(b.cond) && b.mu > a.mu)
            {
                guide.x = { a.mu, b.mu };
                guide.y = { b.cond * a.mu / b.mu, b.cond };
            }
        }

        std::vector<PlotSeries> fitSet = all;
        fitSet.push_back(guide);

        Chart c(rect, Scale::Log, Scale::Log);
        c.setLabels(fmt("%s (%s)", tr("condVsMuTitle").c_str(), pm::condNormName(_result->settings.penalty.outer.condNorm)),
                    fmt("%s  (%s)", glyph::mu, tr("penaltyParameter").c_str()),
                    fmt("%s(%s%s merit)", glyph::kappa, glyph::nabla, glyph::sq));
        c.fit(fitSet);
        c.drawFrame();
        if (!guide.x.empty()) c.drawSeries(guide);
        for (auto& s : all) c.drawSeries(s);

        if (_result->sweep && runs.size() > 1)
        {
            std::vector<PlotSeries> legend;
            for (auto& s : all) if (s.label.length() > 0) legend.push_back(s);
            PlotSeries pl; pl.style = kPenaltyStyle; pl.style.color = textColor(); pl.label = tr("penaltySolid");
            PlotSeries al; al.style = kALStyle; al.style.color = textColor(); al.style.pattern = td::LinePattern::Dash; al.label = tr("augLagDashed");
            legend.push_back(pl); legend.push_back(al);
            legend.push_back(guide);
            c.drawLegend(legend, false, false);
        }
        else
        {
            std::vector<PlotSeries> legend = all;
            if (!guide.x.empty()) legend.push_back(guide);
            c.drawLegend(legend, false, false);
        }
    }

    void paintPerIteration(const gui::Rect& rect, int which)
    {
        const auto& mc = _result->primary();
        std::vector<PlotSeries> s;
        Scale ys = Scale::Log;
        td::String title, ylabel;
        std::vector<double> extra;

        if (which == 0)
        {
            auto cond = [](const pm::OuterRecord& r) { return r.cond; };
            s.push_back(series(mc.penalty, kPenaltyStyle, tr("penalty"), cond, false));
            s.push_back(series(mc.al, kALStyle, tr("augLag"), cond, false));
            title = tr("condVsKTitle");
            ylabel = fmt("%s(%s%s merit)", glyph::kappa, glyph::nabla, glyph::sq);
        }
        else if (which == 1)
        {
            auto its = [](const pm::OuterRecord& r) { return (double) r.newtonIters; };
            s.push_back(series(mc.penalty, kPenaltyStyle, tr("penalty"), its, false));
            s.push_back(series(mc.al, kALStyle, tr("augLag"), its, false));
            ys = Scale::Linear;
            title = tr("newtonItsTitle");
            ylabel = tr("newtonIts");
            extra.push_back(0.0);
        }
        else
        {
            auto grad = [](const pm::OuterRecord& r) { return r.newtonGrad; };
            s.push_back(series(mc.penalty, kPenaltyStyle, tr("penalty"), grad, false));
            s.push_back(series(mc.al, kALStyle, tr("augLag"), grad, false));
            title = tr("newtonGradTitle");
            ylabel = fmt("%s%s%s%s at x_k", glyph::norm, glyph::nabla, glyph::norm, glyph::inf);
            extra.push_back(_result->settings.penalty.outer.newton.gradTol);
        }

        Chart c(rect, Scale::Linear, ys);
        c.setIntegerX(true);
        c.setLabels(title, tr("outerIteration"), ylabel);
        c.fit(s, extra);
        c.drawFrame();
        if (which == 2)
            c.drawHLine(_result->settings.penalty.outer.newton.gradTol, kGuideColor, td::LinePattern::Dash, tr("newtonTolShort"));
        for (auto& x : s) c.drawSeries(x);
        c.drawLegend(s, which != 1, false);
    }

protected:
    void paint(const gui::Rect& r) override
    {
        if (!_result || _result->runs.empty()) { paintPlaceholder(r); return; }

        const double gap = 10;
        const double midX = (r.left + r.right) / 2;
        const double midY = r.top + 0.58 * (r.bottom - r.top);

        paintCondVsMu(gui::Rect(r.left + 6, r.top + 6, midX - gap / 2, midY - gap / 2));
        paintPerIteration(gui::Rect(midX + gap / 2, r.top + 6, r.right - 6, midY - gap / 2), 0);
        paintPerIteration(gui::Rect(r.left + 6, midY + gap / 2, midX - gap / 2, r.bottom - 6), 1);
        paintPerIteration(gui::Rect(midX + gap / 2, midY + gap / 2, r.right - 6, r.bottom - 6), 2);
    }
};

} // namespace ui
