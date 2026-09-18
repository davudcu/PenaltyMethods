#pragma once
#include "ChartCanvas.h"
#include "../core/ComparisonInsights.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

// ============================================================
// SummaryCanvas: the landing page after every run.
//
//   ┌ problem: name, formulation, dimensions ─────────────────────┐
//   │ key findings (plain language, generated from the histories) │
//   ├ scorecard (metric | penalty | aug. Lagrangian) ┬ κ per k    ┤
//   │ better value highlighted and ticked            │ ‖h‖ per k  │
//   └────────────────────────────────────────────────┴────────────┘
// After "Run all problems" the lower-right chart is replaced by a
// conditioning table across all test problems.
// ============================================================
namespace ui
{

class SummaryCanvas : public ChartCanvas
{
    static constexpr double kMargin = 18;
    static constexpr double kRowH   = 27;

    static void drawTick(double x, double y, td::ColorID c)
    {
        gui::Shape::drawLine({ x, y }, { x + 4, y + 4 }, c, 2.0f);
        gui::Shape::drawLine({ x + 4, y + 4 }, { x + 11, y - 5 }, c, 2.0f);
    }

    static void sectionTitle(const char* text, double x, double y, double w)
    {
        gui::DrawableString::draw(td::String(text), gui::Rect(x, y, x + w, y + 18),
                                  gui::Font::ID::SystemSmallerBold, td::ColorID::Gray);
    }

    double paintHeader(const gui::Rect& r, const pm::MethodComparison& mc)
    {
        const pm::IProblem& p = *mc.problem;
        sectionTitle("PROBLEM", r.left, r.top, r.width());
        gui::DrawableString::draw(td::String(p.name().c_str()), gui::Rect(r.left, r.top + 18, r.right, r.top + 44),
                                  gui::Font::ID::SystemLargestBold, textColor());
        gui::DrawableString::draw(fmt("%s     %s     n = %d,  m = %d     %s     %s",
                                      p.description().c_str(), "\xc2\xb7", (int) p.dim(), (int) p.numConstraints(), "\xc2\xb7",
                                      mc.penalty.hasReference ? "reference solution known" : "no reference solution"),
                                  gui::Rect(r.left, r.top + 46, r.right, r.top + 66),
                                  gui::Font::ID::SystemNormal, textColor());
        return r.top + 76;
    }

    double paintFindings(const gui::Rect& r, const pm::MethodComparison& mc)
    {
        sectionTitle("KEY FINDINGS", r.left, r.top, r.width());
        double y = r.top + 22;
        for (const std::string& f : pm::ComparisonInsights::findings(mc))
        {
            gui::DrawableString ds(f.c_str());
            // natID's wrapped-text measurement returns unreliable heights,
            // so the height is estimated from the text length (PDF/SVG glyphs run wider).
            const double glyphW = printModeFlag() ? 7.0 : 6.0;
            const double charsPerLine = (std::max)(20.0, (r.width() - 22) / glyphW);
            const double chars = (double) std::count_if(f.begin(), f.end(), [](char ch) { return (ch & 0xC0) != 0x80; });
            const double lines = std::ceil(chars / charsPerLine);
            const double estimate = lines * 19.0 + 4;
            const double h = (std::max)(20.0, estimate);
            gui::Shape dot;
            dot.createCircle(gui::Circle(r.left + 5, y + 9, 3.0), 1);
            dot.drawFill(td::ColorID::RoyalBlue);
            ds.draw(gui::Rect(r.left + 18, y, r.right, y + h), gui::Font::ID::SystemNormal, textColor(),
                    td::TextAlignment::Left, td::VAlignment::Top, td::TextEllipsize::None);
            y += h + 4;
        }
        return y + 8;
    }

    void paintScorecard(const gui::Rect& r, const pm::MethodComparison& mc)
    {
        sectionTitle("SCORECARD", r.left, r.top, r.width());
        const auto rows = pm::ComparisonInsights::scorecard(mc);
        const double top = r.top + 22;
        // rows get shorter (down to 20 px) before any of them is dropped on small windows
        const double rowH = (std::max)(20.0, (std::min)(kRowH, (r.bottom - top) / (rows.size() + 1)));
        const double c0 = r.left, c1 = r.left + 0.36 * r.width(), c2 = r.left + 0.64 * r.width();

        // header
        gui::Shape::drawRect(gui::Rect(c0, top, r.right, top + rowH), 0.10f, td::ColorID::Gray);
        auto header = [&](double x0, double x1, const char* t, td::ColorID c)
        {
            gui::DrawableString::draw(td::String(t), gui::Rect(x0 + 10, top, x1 - 6, top + rowH),
                                      gui::Font::ID::SystemBold, c, td::TextAlignment::Left, td::VAlignment::Center);
        };
        header(c0, c1, "Metric", textColor());
        header(c1, c2, "Quadratic penalty", kPenaltyStyle.color);
        header(c2, r.right, "Augmented Lagrangian", kALStyle.color);

        double y = top + rowH;
        for (size_t i = 0; i < rows.size() && y + rowH <= r.bottom; ++i)
        {
            const pm::ScoreRow& row = rows[i];
            if (i % 2 == 1)
                gui::Shape::drawRect(gui::Rect(c0, y, r.right, y + rowH), 0.04f, td::ColorID::Gray);

            gui::DrawableString::draw(td::String(row.metric.c_str()), gui::Rect(c0 + 10, y, c1 - 6, y + rowH),
                                      gui::Font::ID::SystemNormal, textColor(), td::TextAlignment::Left, td::VAlignment::Center);

            auto cell = [&](double x0, double x1, const std::string& text, bool best, td::ColorID accent)
            {
                gui::DrawableString::draw(td::String(text.c_str()), gui::Rect(x0 + 10, y, x1 - 26, y + rowH),
                                          best ? gui::Font::ID::SystemBold : gui::Font::ID::SystemNormal,
                                          best ? accent : textColor(), td::TextAlignment::Left, td::VAlignment::Center);
                if (best) drawTick(x1 - 22, y + rowH / 2, accent);
            };
            cell(c1, c2, row.penalty, row.better == pm::Better::Penalty, kPenaltyStyle.color);
            cell(c2, r.right, row.augLag, row.better == pm::Better::AugLag, kALStyle.color);
            y += rowH;
        }
        gui::Shape::drawRect(gui::Rect(c0, top, r.right, y), td::ColorID::Silver, 1);
        gui::Shape::drawLine({ c1, top }, { c1, y }, td::ColorID::Silver, 1);
        gui::Shape::drawLine({ c2, top }, { c2, y }, td::ColorID::Silver, 1);
    }

    void paintMiniChart(const gui::Rect& r, const pm::MethodComparison& mc, bool cond)
    {
        std::vector<PlotSeries> s(2);
        s[0].style = kPenaltyStyle; s[0].label = tr("penalty");
        s[1].style = kALStyle;      s[1].label = tr("augLag");
        for (auto& rec : mc.penalty.outer) { s[0].x.push_back(rec.k); s[0].y.push_back(cond ? rec.cond : rec.hInf); }
        for (auto& rec : mc.al.outer)      { s[1].x.push_back(rec.k); s[1].y.push_back(cond ? rec.cond : rec.hInf); }

        Chart c(r, Scale::Linear, Scale::Log);
        c.setIntegerX(true);
        c.setLabels(tr(cond ? "condVsKTitle" : "violationTitleShort"), tr("outerIteration"),
                    cond ? fmt("%s(%s%s merit)", glyph::kappa, glyph::nabla, glyph::sq)
                         : fmt("%sh(x_k)%s%s", glyph::norm, glyph::norm, glyph::inf));
        c.fit(s);
        c.drawFrame();
        for (auto& x : s) c.drawSeries(x);
        c.drawLegend(s, true, !cond);
    }

    void paintAcrossProblems(const gui::Rect& r)
    {
        sectionTitle("ALL PROBLEMS  (largest condition number)", r.left, r.top, r.width());
        const auto rows = pm::ComparisonInsights::conditioningAcrossProblems(*_result);
        const double top = r.top + 22;
        const double c0 = r.left, c1 = r.left + 0.46 * r.width(), c2 = r.left + 0.64 * r.width(), c3 = r.left + 0.82 * r.width();

        gui::Shape::drawRect(gui::Rect(c0, top, r.right, top + kRowH), 0.10f, td::ColorID::Gray);
        auto head = [&](double x0, double x1, const td::String& t, td::ColorID c)
        {
            gui::DrawableString::draw(t, gui::Rect(x0 + 8, top, x1 - 4, top + kRowH), gui::Font::ID::SystemSmallerBold, c,
                                      td::TextAlignment::Left, td::VAlignment::Center);
        };
        head(c0, c1, td::String("Problem"), textColor());
        head(c1, c2, td::String("Penalty"), kPenaltyStyle.color);
        head(c2, c3, td::String("Aug. Lagr."), kALStyle.color);
        head(c3, r.right, fmt("slope in %s", glyph::mu), textColor());

        double y = top + kRowH;
        for (size_t i = 0; i < rows.size() && y + kRowH <= r.bottom; ++i)
        {
            if (i % 2 == 1) gui::Shape::drawRect(gui::Rect(c0, y, r.right, y + kRowH), 0.04f, td::ColorID::Gray);
            auto text = [&](double x0, double x1, const td::String& t)
            {
                gui::DrawableString::draw(t, gui::Rect(x0 + 8, y, x1 - 4, y + kRowH), gui::Font::ID::SystemSmaller,
                                          textColor(), td::TextAlignment::Left, td::VAlignment::Center);
            };
            text(c0, c1, td::String(rows[i].name.c_str()));
            text(c1, c2, fmtSci(rows[i].penaltyMaxCond, 1));
            text(c2, c3, fmtSci(rows[i].alMaxCond, 1));
            text(c3, r.right, fmt("%.2f", rows[i].penaltySlope));
            y += kRowH;
        }
        gui::Shape::drawRect(gui::Rect(c0, top, r.right, y), td::ColorID::Silver, 1);
    }

protected:
    void paint(const gui::Rect& r) override
    {
        if (!_result || _result->runs.empty()) { paintPlaceholder(r); return; }
        const pm::MethodComparison& mc = _result->primary();

        const gui::Rect inner(r.left + kMargin, r.top + kMargin, r.right - kMargin, r.bottom - kMargin);
        double y = paintHeader(inner, mc);
        gui::Shape::drawLine({ inner.left, y - 6 }, { inner.right, y - 6 }, td::ColorID::Silver, 1);
        y = paintFindings(gui::Rect(inner.left, y, inner.right, inner.bottom), mc);

        const bool sweepTable = _result->sweep && _result->runs.size() > 1;

        if (inner.width() >= 820)
        {
            // wide: scorecard left, charts stacked on the right
            const double splitX = inner.left + 0.56 * inner.width();
            paintScorecard(gui::Rect(inner.left, y, splitX - 12, inner.bottom), mc);

            const double midY = (y + inner.bottom) / 2;
            paintMiniChart(gui::Rect(splitX + 4, y - 8, inner.right, midY), mc, true);
            if (sweepTable)
                paintAcrossProblems(gui::Rect(splitX + 12, midY + 8, inner.right, inner.bottom));
            else
                paintMiniChart(gui::Rect(splitX + 4, midY, inner.right, inner.bottom), mc, false);
            return;
        }

        // narrow: scorecard across the full width, charts side by side below if there is room
        const double scoreH = 22 + kRowH * (pm::ComparisonInsights::scorecard(mc).size() + 1) + 8;
        paintScorecard(gui::Rect(inner.left, y, inner.right, (std::min)(inner.bottom, y + scoreH)), mc);
        const double below = y + scoreH + 8;
        if (inner.bottom - below < 200) return;
        const double midX = (inner.left + inner.right) / 2;
        paintMiniChart(gui::Rect(inner.left, below, midX - 6, inner.bottom), mc, true);
        if (sweepTable)
            paintAcrossProblems(gui::Rect(midX + 6, below, inner.right, inner.bottom));
        else
            paintMiniChart(gui::Rect(midX + 6, below, inner.right, inner.bottom), mc, false);
    }
};

} // namespace ui
