#pragma once
#include "ChartCanvas.h"
#include "../core/MeritFunctions.h"
#include <algorithm>
#include <cmath>
#include <vector>

// ============================================================
// LandscapeCanvas: penalty landscape with the constraint-
// violation history overlaid.
//
// Top area, for a selected outer iteration k:
//   * 2-D problems: heat maps of Q(x; mu_k) and L_A(x; lambda_k, mu_k)
//     side by side, the constraint curve h(x) = 0, the Newton path
//     of each method and x*.  As mu grows the penalty valley narrows
//     around h(x) = 0, the geometric picture of ill-conditioning.
//   * n > 2 problems: the same maps on the (x1, x2) slice through x*
//   * curve fitting: data points and the fitted conics of both
//     methods at iteration k against the reference fit
// Bottom strip: ||h(x_k)||_inf for both methods with a marker at k.
// ============================================================
namespace ui
{

class LandscapeCanvas : public ChartCanvas
{
    int _k = -1;    // selected outer iteration, -1 = last

    static constexpr int kGrid = 64;

    static const pm::OuterRecord* recordAt(const pm::SolveHistory& h, int k)
    {
        if (h.outer.empty()) return nullptr;
        if (k < 0 || k >= (int) h.outer.size()) return &h.outer.back();
        return &h.outer[(size_t) k];
    }

    // Stretch the data window so one unit has the same length on both axes.
    static void equalAspect(const gui::Rect& plot, double& x0, double& x1, double& y0, double& y1)
    {
        const double dx = x1 - x0, dy = y1 - y0;
        const double pw = (std::max)(1.0, plot.width()), ph = (std::max)(1.0, plot.height());
        if (dx / pw > dy / ph)
        {
            const double ny = dx * ph / pw, cy = (y0 + y1) / 2;
            y0 = cy - ny / 2; y1 = cy + ny / 2;
        }
        else
        {
            const double nx = dy * pw / ph, cx = (x0 + x1) / 2;
            x0 = cx - nx / 2; x1 = cx + nx / 2;
        }
    }

    // Zero contour of a scalar field sampled on a (kGrid+1)^2 corner grid.
    template <typename F>
    static void drawZeroContour(const Chart& c, double x0, double x1, double y0, double y1, int n, F field,
                                td::ColorID color, td::LinePattern pattern, float width)
    {
        std::vector<double> v((size_t) (n + 1) * (n + 1));
        const double hx = (x1 - x0) / n, hy = (y1 - y0) / n;
        for (int j = 0; j <= n; ++j)
            for (int i = 0; i <= n; ++i)
                v[(size_t) j * (n + 1) + i] = field(x0 + i * hx, y0 + j * hy);

        std::vector<gui::Point> segs;
        for (int j = 0; j < n; ++j)
            for (int i = 0; i < n; ++i)
            {
                const double xs[4] = { x0 + i * hx, x0 + (i + 1) * hx, x0 + (i + 1) * hx, x0 + i * hx };
                const double ys[4] = { y0 + j * hy, y0 + j * hy, y0 + (j + 1) * hy, y0 + (j + 1) * hy };
                const double f[4] = { v[(size_t) j * (n + 1) + i], v[(size_t) j * (n + 1) + i + 1],
                                      v[(size_t) (j + 1) * (n + 1) + i + 1], v[(size_t) (j + 1) * (n + 1) + i] };
                double px[4], py[4];
                int cnt = 0;
                for (int e = 0; e < 4; ++e)
                {
                    const int a = e, b = (e + 1) % 4;
                    if (!std::isfinite(f[a]) || !std::isfinite(f[b])) continue;
                    if ((f[a] < 0) != (f[b] < 0))
                    {
                        const double t = f[a] / (f[a] - f[b]);
                        px[cnt] = xs[a] + t * (xs[b] - xs[a]);
                        py[cnt] = ys[a] + t * (ys[b] - ys[a]);
                        ++cnt;
                    }
                }
                for (int s = 0; s + 1 < cnt; s += 2)
                {
                    gui::Point pa, pb;
                    if (c.toPixel(px[s], py[s], pa) && c.toPixel(px[s + 1], py[s + 1], pb))
                    {
                        segs.push_back(pa);
                        segs.push_back(pb);
                    }
                }
            }

        c.beginClip();
        for (size_t s = 0; s + 1 < segs.size(); s += 2)
            gui::Shape::drawLine(segs[s], segs[s + 1], color, width, pattern);
        c.endClip();
    }

    static void drawCross(const Chart& c, double x, double y, td::ColorID color)
    {
        gui::Point p;
        if (!c.toPixel(x, y, p)) return;
        const double d = 6;
        c.beginClip();
        gui::Shape::drawLine({ p.x - d, p.y - d }, { p.x + d, p.y + d }, color, 2.5f);
        gui::Shape::drawLine({ p.x - d, p.y + d }, { p.x + d, p.y - d }, color, 2.5f);
        c.endClip();
    }

    // ---- merit landscape of one method -----------------------------
    void paintMerit(const gui::Rect& rect, const pm::MethodComparison& mc, const pm::SolveHistory& h,
                    bool isPenalty, const SeriesStyle& st)
    {
        const pm::IProblem& p = *mc.problem;
        const pm::OuterRecord* rec = recordAt(h, _k);
        Chart chart(rect, Scale::Linear, Scale::Linear);
        if (!rec)
        {
            chart.drawMessage(tr("noIterations"));
            return;
        }

        // base point and window
        pm::Vec base = h.hasReference ? h.xStar : rec->x;
        double x0, x1, y0, y1;
        const bool planar = p.asPlanar() != nullptr;
        if (planar) p.asPlanar()->plotWindow(x0, x1, y0, y1);
        else
        {
            x0 = base[0] - 1.5; x1 = base[0] + 1.5;
            y0 = base[1] - 1.5; y1 = base[1] + 1.5;
        }
        equalAspect(chart.plotRect(), x0, x1, y0, y1);
        chart.setRawRanges(x0, x1, y0, y1);

        pm::Vec lambda = isPenalty ? pm::Vec(p.numConstraints(), 0.0) : rec->lambdaUsed;
        pm::AugmentedLagrangianMerit merit(p, lambda, rec->mu);

        const int kShown = (int) (rec - h.outer.data());
        td::String title = isPenalty
            ? fmt("%s   Q(x; %s = %.1e),  k = %d", tr("penalty").c_str(), glyph::mu, rec->mu, kShown)
            : fmt("%s   L_A(x; %s_k, %s = %.1e),  k = %d", tr("augLag").c_str(), glyph::lambda, glyph::mu, rec->mu, kShown);
        chart.setLabels(title, planar ? td::String("x1") : tr("sliceX1"), planar ? td::String("x2") : tr("sliceX2"));

        // --- heat map (log-spaced levels above the minimum on the window)
        std::vector<double> vals((size_t) kGrid * kGrid);
        const double hx = (x1 - x0) / kGrid, hy = (y1 - y0) / kGrid;
        double vmin = pm::kInf, vmax = -pm::kInf;
        pm::Vec pt = base;
        for (int j = 0; j < kGrid; ++j)
            for (int i = 0; i < kGrid; ++i)
            {
                pt[0] = x0 + (i + 0.5) * hx;
                pt[1] = y0 + (j + 0.5) * hy;
                const double v = merit.value(pt);
                vals[(size_t) j * kGrid + i] = v;
                if (std::isfinite(v)) { vmin = (std::min)(vmin, v); vmax = (std::max)(vmax, v); }
            }

        if (vmax > vmin)
        {
            const double range = vmax - vmin;
            const double eps = range * 1e-6;
            const double l0 = std::log10(eps), l1 = std::log10(range + eps);
            chart.beginClip();
            for (int j = 0; j < kGrid; ++j)
            {
                int runStart = 0, runLevel = -1;
                auto level = [&](int i)
                {
                    const double v = vals[(size_t) j * kGrid + i];
                    if (!std::isfinite(v)) return kHeatLevelCount - 1;
                    const double t = (std::log10(v - vmin + eps) - l0) / (l1 - l0);
                    return (std::min)(kHeatLevelCount - 1, (std::max)(0, (int) (t * kHeatLevelCount)));
                };
                for (int i = 0; i <= kGrid; ++i)
                {
                    const int lv = i < kGrid ? level(i) : -2;
                    if (lv != runLevel)
                    {
                        if (runLevel >= 0)
                        {
                            gui::Point a, b;
                            chart.toPixel(x0 + runStart * hx, y0 + (j + 1) * hy, a);
                            chart.toPixel(x0 + i * hx, y0 + j * hy, b);
                            gui::Shape::drawRect(gui::Rect(a.x, a.y, b.x + 0.5, b.y + 0.5), kHeatLevels[runLevel]);
                        }
                        runStart = i;
                        runLevel = lv;
                    }
                }
            }
            chart.endClip();
        }
        chart.drawFrame(false);

        // --- constraint curves h_i(x) = 0
        for (size_t ci = 0; ci < p.numConstraints(); ++ci)
        {
            pm::Vec q = base, hv;
            drawZeroContour(chart, x0, x1, y0, y1, kGrid, [&](double u, double w)
            {
                q[0] = u; q[1] = w;
                p.constraints(q, hv);
                return hv[ci];
            }, kConstraintColor, td::LinePattern::Solid, 2.0f);
        }

        // --- Newton path up to x_k and the outer iterates
        PlotSeries path;
        path.style = st;
        path.style.width = 1.5f;
        path.markers = false;
        for (size_t i = 0; i <= rec->pathEnd && i < h.path.size(); ++i)
        {
            path.x.push_back(h.path[i][0]);
            path.y.push_back(h.path[i][1]);
        }
        chart.drawSeries(path);

        chart.beginClip();
        for (int i = 0; i <= kShown; ++i)
        {
            gui::Point px;
            if (chart.toPixel(h.outer[(size_t) i].x[0], h.outer[(size_t) i].x[1], px))
                chart.drawMarker(px, st, i == kShown ? 5.5 : 3.0);
        }
        chart.endClip();

        if (h.hasReference) drawCross(chart, h.xStar[0], h.xStar[1], kReferenceStyle.color);

        // legend
        std::vector<PlotSeries> legend(3);
        legend[0].style = st; legend[0].label = tr("newtonPath");
        legend[1].style = { kConstraintColor, td::LinePattern::Solid, 2.0f, false };
        legend[1].markers = false; legend[1].label = tr("constraintCurve");
        legend[2].style = { kReferenceStyle.color, td::LinePattern::Solid, 2.5f, false };
        legend[2].line = false; legend[2].label = tr("solutionMarker");
        if (!h.hasReference) legend.pop_back();
        chart.drawLegend(legend, true, true);
    }

    // ---- fitted curves (curve-fitting problems) ---------------------
    void paintCurveFit(const gui::Rect& rect, const pm::MethodComparison& mc)
    {
        const pm::ICurveFitProblem* cf = mc.problem->asCurveFit();
        const pm::OuterRecord* rp = recordAt(mc.penalty, _k);
        const pm::OuterRecord* ra = recordAt(mc.al, _k);

        double x0, x1, y0, y1;
        cf->plotWindow(x0, x1, y0, y1);
        Chart chart(rect, Scale::Linear, Scale::Linear);
        equalAspect(chart.plotRect(), x0, x1, y0, y1);
        chart.setRawRanges(x0, x1, y0, y1);

        const int kp = rp ? (int) (rp - mc.penalty.outer.data()) : -1;
        const int ka = ra ? (int) (ra - mc.al.outer.data()) : -1;
        chart.setLabels(fmt("%s  (%s k = %d,  %s k = %d)", tr("fitTitle").c_str(),
                            tr("penaltyShort").c_str(), kp, tr("augLagShort").c_str(), ka),
                        td::String("x"), td::String("y"));
        chart.drawFrame(true);

        const int n = 140;
        if (mc.penalty.hasReference)
        {
            const pm::Vec& ts = mc.penalty.xStar;
            drawZeroContour(chart, x0, x1, y0, y1, n, [&](double u, double w) { return cf->curveValue(ts, u, w); },
                            kReferenceStyle.color, td::LinePattern::Dash, 3.0f);
        }
        if (rp)
            drawZeroContour(chart, x0, x1, y0, y1, n, [&](double u, double w) { return cf->curveValue(rp->x, u, w); },
                            kPenaltyStyle.color, td::LinePattern::Solid, 2.0f);
        if (ra)
            drawZeroContour(chart, x0, x1, y0, y1, n, [&](double u, double w) { return cf->curveValue(ra->x, u, w); },
                            kALStyle.color, td::LinePattern::Solid, 2.0f);

        SeriesStyle dataStyle{ textColor(), td::LinePattern::Solid, 1.0f, false };
        chart.beginClip();
        for (auto& d : cf->dataPoints())
        {
            gui::Point px;
            if (chart.toPixel(d.x, d.y, px)) chart.drawMarker(px, dataStyle, 2.5);
        }
        chart.endClip();

        std::vector<PlotSeries> legend(4);
        legend[0].style = dataStyle; legend[0].line = false; legend[0].label = tr("dataPointsLegend");
        legend[1].style = kReferenceStyle; legend[1].markers = false; legend[1].label = tr("referenceFit");
        legend[2].style = kPenaltyStyle; legend[2].markers = false; legend[2].label = tr("penalty");
        legend[3].style = kALStyle; legend[3].markers = false; legend[3].label = tr("augLag");
        chart.drawLegend(legend, true, false);
    }

    // ---- violation history strip -----------------------------------
    void paintViolationStrip(const gui::Rect& rect, const pm::MethodComparison& mc)
    {
        std::vector<PlotSeries> s(2);
        s[0].style = kPenaltyStyle; s[0].label = tr("penalty");
        s[1].style = kALStyle;      s[1].label = tr("augLag");
        for (auto& r : mc.penalty.outer) { s[0].x.push_back(r.k); s[0].y.push_back(r.hInf); }
        for (auto& r : mc.al.outer)      { s[1].x.push_back(r.k); s[1].y.push_back(r.hInf); }

        Chart c(rect, Scale::Linear, Scale::Log);
        c.setIntegerX(true);
        c.setLabels(tr("violationStripTitle"), tr("outerIteration"),
                    fmt("%sh(x_k)%s%s", glyph::norm, glyph::norm, glyph::inf));
        c.fit(s);
        c.drawFrame();
        const int nMax = (int) (std::max)(mc.penalty.outer.size(), mc.al.outer.size());
        const int k = _k < 0 ? nMax - 1 : _k;
        c.drawVLine(k, textColor(), td::LinePattern::Dot, 1.5f);
        for (auto& x : s) c.drawSeries(x);
        c.drawLegend(s, true, false);
    }

protected:
    void paint(const gui::Rect& r) override
    {
        if (!_result || _result->runs.empty()) { paintPlaceholder(r); return; }
        const pm::MethodComparison& mc = _result->primary();

        const double splitY = r.top + 0.70 * (r.bottom - r.top);
        const gui::Rect top(r.left + 6, r.top + 6, r.right - 6, splitY - 4);
        const gui::Rect bottom(r.left + 6, splitY + 4, r.right - 6, r.bottom - 6);

        if (mc.problem->asCurveFit())
            paintCurveFit(top, mc);
        else
        {
            const double mid = (top.left + top.right) / 2;
            paintMerit(gui::Rect(top.left, top.top, mid - 6, top.bottom), mc, mc.penalty, true, kPenaltyStyle);
            paintMerit(gui::Rect(mid + 6, top.top, top.right, top.bottom), mc, mc.al, false, kALStyle);
        }
        paintViolationStrip(bottom, mc);
    }

public:
    void setOuterIndex(int k)
    {
        _k = k;
        reDraw();
    }

    void setResult(const ResultPtr& r) override
    {
        _k = -1;
        ChartCanvas::setResult(r);
    }
};

} // namespace ui
