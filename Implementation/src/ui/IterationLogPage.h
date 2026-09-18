#pragma once
#include <gui/View.h>
#include <gui/Label.h>
#include <gui/TableEdit.h>
#include <gui/VerticalLayout.h>
#include <dp/IDataSet.h>
#include <dp/IDatabase.h>
#include "ChartCanvas.h"
#include <limits>

// ============================================================
// IterationLogPage: tabular report of a run.
//   top   : one summary row per method (and per problem after a sweep)
//   bottom: one row per outer iteration of both methods
// Both tables are backed by in-memory (connectionless) data sets.
// ============================================================
namespace ui
{

class IterationLogPage : public gui::View
{
    gui::Label          _lblSummary;
    gui::TableEdit      _summary;
    gui::Label          _lblDetail;
    gui::TableEdit      _detail;
    gui::VerticalLayout _vl;
    dp::IDataSetPtr     _dsSummary;
    dp::IDataSetPtr     _dsDetail;

    enum SumCol { S_Problem = 0, S_Method, S_Status, S_Outer, S_Newton, S_HInf, S_XErr, S_FErr, S_MaxCond, S_LastCond, S_Ms, S_Count };
    enum DetCol { D_Method = 0, D_K, D_Mu, D_F, D_HInf, D_Kkt, D_Cond, D_Newton, D_NStatus, D_NGrad, D_Shift, D_LinRes, D_XErr, D_LErr, D_Count };

    static td::String header(const char* a, const char* b = "", const char* c = "", const char* d = "")
    {
        return fmt("%s%s%s%s", a, b, c, d);
    }

    static double orNaN(double v) { return v < 0 ? std::numeric_limits<double>::quiet_NaN() : v; }

    void initSummary()
    {
        _dsSummary = dp::createConnectionlessDataSet(dp::IDataSet::Size::Small);
        dp::DSColumns cols(_dsSummary->allocBindColumns(S_Count));
        cols << "Problem" << td::string8 << "Method" << td::string8 << "Status" << td::string8
             << "Outer" << td::int4 << "Newton" << td::int4 << "HInf" << td::real8 << "XErr" << td::real8
             << "FErr" << td::real8 << "MaxCond" << td::real8 << "LastCond" << td::real8 << "Ms" << td::real8;
        _dsSummary->execute();

        gui::Columns vis(_summary.allocBindColumns(S_Count));
        vis << gui::ThSep::DoNotShowThSep
            << gui::Header(S_Problem, tr("colProblem"), tr("colProblem"), 210)
            << gui::Header(S_Method, tr("colMethod"), tr("colMethod"), 150)
            << gui::Header(S_Status, tr("colStatus"), tr("colStatusTT"), 130)
            << gui::Header(S_Outer, tr("colOuter"), tr("colOuterTT"), 60, td::HAlignment::Right)
            << gui::Header(S_Newton, tr("colNewton"), tr("colNewtonTT"), 70, td::HAlignment::Right)
            << gui::Header(S_HInf, header(glyph::norm, "h", glyph::norm, glyph::inf), tr("colHInfTT"), 95, td::HAlignment::Right)
            << gui::Header(S_XErr, header(glyph::norm, "x - x*", glyph::norm), tr("colXErrTT"), 95, td::HAlignment::Right)
            << gui::Header(S_FErr, td::String("|f - f*|"), tr("colFErrTT"), 95, td::HAlignment::Right)
            << gui::Header(S_MaxCond, header("max ", glyph::kappa), tr("colMaxCondTT"), 95, td::HAlignment::Right)
            << gui::Header(S_LastCond, header("final ", glyph::kappa), tr("colLastCondTT"), 95, td::HAlignment::Right)
            << gui::Header(S_Ms, tr("colMs"), tr("colMsTT"), 70, td::HAlignment::Right);
        _summary.init(_dsSummary);
        for (int c : { (int) S_HInf, (int) S_XErr, (int) S_FErr, (int) S_MaxCond, (int) S_LastCond })
            _summary.setColumnNumericFormat(c, td::FormatFloat::Scientific, 2);
        _summary.setColumnNumericFormat(S_Ms, td::FormatFloat::Decimal, 2);
    }

    void initDetail()
    {
        _dsDetail = dp::createConnectionlessDataSet(dp::IDataSet::Size::Medium);
        dp::DSColumns cols(_dsDetail->allocBindColumns(D_Count));
        cols << "Method" << td::string8 << "K" << td::int4 << "Mu" << td::real8 << "F" << td::real8
             << "HInf" << td::real8 << "Kkt" << td::real8 << "Cond" << td::real8 << "Newton" << td::int4
             << "NStatus" << td::string8 << "NGrad" << td::real8 << "Shift" << td::real8 << "LinRes" << td::real8
             << "XErr" << td::real8 << "LErr" << td::real8;
        _dsDetail->execute();

        gui::Columns vis(_detail.allocBindColumns(D_Count));
        vis << gui::ThSep::DoNotShowThSep
            << gui::Header(D_Method, tr("colMethod"), tr("colMethod"), 150)
            << gui::Header(D_K, td::String("k"), tr("colKTT"), 40, td::HAlignment::Right)
            << gui::Header(D_Mu, td::String(glyph::mu), tr("colMuTT"), 85, td::HAlignment::Right)
            << gui::Header(D_F, td::String("f(x_k)"), tr("colFTT"), 95, td::HAlignment::Right)
            << gui::Header(D_HInf, header(glyph::norm, "h", glyph::norm, glyph::inf), tr("colHInfTT"), 90, td::HAlignment::Right)
            << gui::Header(D_Kkt, header(glyph::norm, glyph::nabla, "L", glyph::norm), tr("colKktTT"), 90, td::HAlignment::Right)
            << gui::Header(D_Cond, td::String(glyph::kappa), tr("colCondTT"), 90, td::HAlignment::Right)
            << gui::Header(D_Newton, tr("colNewton"), tr("colNewtonItTT"), 65, td::HAlignment::Right)
            << gui::Header(D_NStatus, tr("colNewtonStatus"), tr("colNewtonStatusTT"), 100)
            << gui::Header(D_NGrad, header(glyph::norm, glyph::nabla, "merit", glyph::norm), tr("colNGradTT"), 90, td::HAlignment::Right)
            << gui::Header(D_Shift, tr("colShift"), tr("colShiftTT"), 80, td::HAlignment::Right)
            << gui::Header(D_LinRes, tr("colLinRes"), tr("colLinResTT"), 90, td::HAlignment::Right)
            << gui::Header(D_XErr, header(glyph::norm, "x - x*", glyph::norm), tr("colXErrTT"), 90, td::HAlignment::Right)
            << gui::Header(D_LErr, header(glyph::norm, glyph::lambda, " - ", glyph::lambda), tr("colLErrTT"), 90, td::HAlignment::Right);
        _detail.init(_dsDetail);
        for (int c : { (int) D_Mu, (int) D_F, (int) D_HInf, (int) D_Kkt, (int) D_Cond, (int) D_NGrad,
                       (int) D_Shift, (int) D_LinRes, (int) D_XErr, (int) D_LErr })
            _detail.setColumnNumericFormat(c, td::FormatFloat::Scientific, 2);
    }

    void addSummaryRow(const pm::MethodComparison& mc, const pm::SolveHistory& h)
    {
        auto& row = _summary.getEmptyRow();
        row[S_Problem] = mc.problem->name().c_str();
        row[S_Method] = h.method.c_str();
        row[S_Status] = h.message.c_str();
        row[S_Outer] = (td::INT4) h.outer.size();
        row[S_Newton] = (td::INT4) h.totalNewton;
        const bool any = !h.outer.empty();
        row[S_HInf] = any ? h.last().hInf : 0.0;
        row[S_XErr] = any ? orNaN(h.last().xError) : 0.0;
        row[S_FErr] = any ? orNaN(h.last().fError) : 0.0;
        row[S_MaxCond] = h.maxCond();
        row[S_LastCond] = any ? h.last().cond : 0.0;
        row[S_Ms] = h.elapsedMs;
        _summary.push_back();
    }

    void addDetailRows(const pm::SolveHistory& h)
    {
        for (auto& r : h.outer)
        {
            auto& row = _detail.getEmptyRow();
            row[D_Method] = h.method.c_str();
            row[D_K] = (td::INT4) r.k;
            row[D_Mu] = r.mu;
            row[D_F] = r.f;
            row[D_HInf] = r.hInf;
            row[D_Kkt] = r.kktNorm;
            row[D_Cond] = r.cond;
            row[D_Newton] = (td::INT4) r.newtonIters;
            row[D_NStatus] = r.newtonStatus.c_str();
            row[D_NGrad] = r.newtonGrad;
            row[D_Shift] = r.hessShift;
            row[D_LinRes] = r.linResidual;
            row[D_XErr] = orNaN(r.xError);
            row[D_LErr] = orNaN(r.lambdaError);
            _detail.push_back();
        }
    }

public:
    IterationLogPage()
    : _lblSummary(tr("summaryTitle"), gui::Font::ID::SystemBold)
    , _summary(td::Ownership::Extern, gui::TableEdit::RowNumberVisibility::NotVisible)
    , _lblDetail(tr("detailTitle"), gui::Font::ID::SystemBold)
    , _detail(td::Ownership::Extern, gui::TableEdit::RowNumberVisibility::NotVisible)
    , _vl(4)
    {
        initSummary();
        initDetail();
        _summary.setSizeLimits(0, gui::Control::Limit::None, 170, gui::Control::Limit::Fixed);
        _vl << _lblSummary << _summary << _lblDetail << _detail;
        setMargins(6, 6, 6, 6);
        setLayout(&_vl);
    }

    void setResult(const ResultPtr& r)
    {
        _summary.beginUpdate();
        _summary.clean();
        _detail.beginUpdate();
        _detail.clean();
        if (r)
        {
            for (auto& mc : r->runs)
            {
                addSummaryRow(mc, mc.penalty);
                addSummaryRow(mc, mc.al);
            }
            const auto& mc = r->primary();
            addDetailRows(mc.penalty);
            addDetailRows(mc.al);
        }
        _summary.endUpdate();
        _detail.endUpdate();
    }
};

} // namespace ui
