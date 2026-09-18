#pragma once
#include <gui/View.h>
#include <gui/Label.h>
#include <gui/NumericEdit.h>
#include <gui/ComboBox.h>
#include <gui/CheckBox.h>
#include <gui/Button.h>
#include <gui/GridLayout.h>
#include <gui/GridComposer.h>
#include <gui/HorizontalLayout.h>
#include <gui/VerticalLayout.h>
#include <gui/StandardTabView.h>
#include "../core/ComparisonRunner.h"
#include "ParamSlider.h"
#include "Style.h"
#include <functional>
#include <random>
#include <algorithm>
#include <cmath>

// ============================================================
// InspectorPanel: experiment definition (right-hand side).
//
//   EXPERIMENT   problem + description
//                [Run comparison] [Stop]   [Run all problems]
//                [x] re-run automatically
//                [Export all] [Open exports folder]
//   ┌ Methods ┬ Stopping ┬ Data ┐     parameters as sliders,
//   │ label   | slider  | value │     grouped in tabs so the
//   └───────────────────────────┘     panel fits small screens
//
// The panel reads/writes settings and reports intent through
// callbacks; it never runs solvers itself (SRP).
// ============================================================
namespace ui
{

using PK = ParamSlider::Kind;

// gui::View with a layout assigned from outside (setMargins/setLayout are protected)
class LayoutView : public gui::View
{
public:
    void place(gui::Layout& layout, int left, int top, int right, int bottom)
    {
        setMargins(left, top, right, bottom);
        setLayout(&layout);
    }
};

// ---- tab: both methods ------------------------------------------
class MethodsTab : public gui::View
{
public:
    gui::Label    hdrPenalty;
    ParamSlider   penMu0;
    ParamSlider   penGrowth;
    ParamSlider   penMuMax;
    gui::Label    hdrAL;
    ParamSlider   alMu;
    gui::CheckBox alAdaptive;
    ParamSlider   alGrowth;
    gui::Button   reset;
    gui::GridLayout grid;
    LayoutView    body;     // natural-height block; the spacer below takes the rest
    gui::VerticalLayout vl;

    MethodsTab()
    : hdrPenalty(tr("secPenalty"), gui::Font::ID::SystemSmallerBold)
    , penMu0(tr("mu0"), tr("penMu0TT"), PK::Log10, -2, 4, 1)
    , penGrowth(tr("growth"), tr("penGrowthTT"), PK::Integer, 2, 50, 1)
    , penMuMax(tr("muMax"), tr("penMuMaxTT"), PK::Log10, 4, 16, 1)
    , hdrAL(tr("secAL"), gui::Font::ID::SystemSmallerBold)
    , alMu(tr("alMu"), tr("alMuTT"), PK::Log10, -2, 6, 0.5)
    , alAdaptive(tr("alAdaptive"))
    , alGrowth(tr("growth"), tr("alGrowthTT"), PK::Integer, 2, 50, 1)
    , reset(tr("resetDefaults"), tr("resetDefaultsTT"))
    , grid(9, 3)
    , vl(2)
    {
        alAdaptive.setToolTip(tr("alAdaptiveTT"));
        gui::GridComposer gc(grid);
        gc.appendRow(hdrPenalty, 0);
        penMu0.appendTo(gc);
        penGrowth.appendTo(gc);
        penMuMax.appendTo(gc);
        gc.appendRow(hdrAL, 0);
        alMu.appendTo(gc);
        gc.appendRow(alAdaptive, 0);
        alGrowth.appendTo(gc);
        gc.appendRow(reset, 0);
        grid.setSpaceBetweenCells(6, 8);
        body.place(grid, 8, 8, 8, 8);
        vl << body;
        vl.appendSpacer();
        setLayout(&vl);
    }
};

// ---- tab: stopping criteria and analysis --------------------------
class StoppingTab : public gui::View
{
public:
    ParamSlider   feasTol;
    ParamSlider   optTol;
    ParamSlider   newtonTol;
    ParamSlider   maxOuter;
    ParamSlider   maxNewton;
    gui::Label    lblCondNorm;
    gui::ComboBox condNorm;
    gui::GridLayout grid;
    LayoutView    body;     // natural-height block; the spacer below takes the rest
    gui::VerticalLayout vl;

    StoppingTab()
    : feasTol(tr("feasTol"), tr("feasTolTT"), PK::Log10, -14, -2, 1)
    , optTol(tr("optTol"), tr("optTolTT"), PK::Log10, -12, -2, 1)
    , newtonTol(tr("newtonTol"), tr("newtonTolTT"), PK::Log10, -14, -4, 1)
    , maxOuter(tr("maxOuter"), tr("maxOuterTT"), PK::Integer, 1, 100, 1)
    , maxNewton(tr("maxNewton"), tr("maxNewtonTT"), PK::Integer, 5, 500, 5)
    , lblCondNorm(tr("condNorm"))
    , grid(6, 3)
    , vl(2)
    {
        condNorm.addItem(tr("condInf"));
        condNorm.addItem(tr("cond1"));
        condNorm.addItem(tr("cond2"));
        condNorm.setToolTip(tr("condNormTT"));

        gui::GridComposer gc(grid);
        feasTol.appendTo(gc);
        optTol.appendTo(gc);
        newtonTol.appendTo(gc);
        maxOuter.appendTo(gc);
        maxNewton.appendTo(gc);
        gc.appendRow(lblCondNorm); gc.appendCol(condNorm, 0);
        grid.setSpaceBetweenCells(6, 8);
        body.place(grid, 8, 8, 8, 8);
        vl << body;
        vl.appendSpacer();
        setLayout(&vl);
    }
};

// ---- tab: curve-fit data ------------------------------------------
class DataTab : public gui::View
{
public:
    gui::Label       hint;
    ParamSlider      points;
    ParamSlider      noise;
    gui::Label       lblSeed;
    gui::NumericEdit seed;
    gui::Button      newSeed;
    gui::GridLayout  grid;
    LayoutView       body;
    gui::VerticalLayout vl;

    DataTab()
    : hint(tr("dataHint"), gui::Font::ID::SystemSmaller)
    , points(tr("dataPoints"), tr("dataPointsTT"), PK::Integer, 6, 200, 1)
    , noise(tr("noise"), tr("noiseTT"), PK::Linear, 0.0, 0.5, 0.01)
    , lblSeed(tr("seed"))
    , seed(td::int4, gui::LineEdit::Messages::Send, false, tr("seedTT"))
    , newSeed(tr("newSeed"), tr("newSeedTT"))
    , grid(4, 3)
    , vl(2)
    {
        seed.showThSep(false);
        seed.setSizeLimits(90, gui::Control::Limit::UseAsMin);
        gui::GridComposer gc(grid);
        gc.appendRow(hint, 0);
        points.appendTo(gc);
        noise.appendTo(gc);
        gc.appendRow(lblSeed) << seed << newSeed;
        grid.setSpaceBetweenCells(6, 8);
        body.place(grid, 8, 8, 8, 8);
        vl << body;
        vl.appendSpacer();
        setLayout(&vl);
    }
};

// ---- the panel ------------------------------------------------------
class InspectorPanel : public gui::View
{
    gui::Label       _hdrExperiment;
    gui::ComboBox    _cmbProblem;
    gui::Label       _lblDescription;
    gui::HorizontalLayout _hlActions;
    gui::Button      _btnRun;
    gui::Button      _btnStop;
    gui::Button      _btnSweep;
    gui::CheckBox    _cbLive;
    gui::HorizontalLayout _hlExport;
    gui::Button      _btnExportAll;
    gui::Button      _btnOpenExports;
    gui::GridLayout  _topGrid;
    LayoutView       _top;

    MethodsTab       _methods;
    StoppingTab      _stopping;
    DataTab          _data;
    gui::StandardTabView _tabs;

    gui::VerticalLayout _vl;

    std::function<void()> _onRun, _onStop, _onSweep, _onSettingsChanged, _onExportAll, _onOpenExports;

    void changed()
    {
        if (_onSettingsChanged) _onSettingsChanged();
    }

    void applyDefaults()
    {
        const pm::RunSettings d;
        _methods.penMu0.set(d.penalty.mu0);
        _methods.penGrowth.set(d.penalty.growth);
        _methods.penMuMax.set(d.penalty.muMax);
        _methods.alMu.set(d.al.mu0);
        _methods.alGrowth.set(d.al.growth);
        _methods.alAdaptive.setChecked(d.al.adaptive, false);
        _stopping.feasTol.set(d.penalty.outer.feasTol);
        _stopping.optTol.set(d.penalty.outer.optTol);
        _stopping.newtonTol.set(d.penalty.outer.newton.gradTol);
        _stopping.maxOuter.set(d.penalty.outer.maxOuter);
        _stopping.maxNewton.set(d.penalty.outer.newton.maxIter);
        _stopping.condNorm.selectIndex(0, false);
        _data.points.set(d.problem.nPoints);
        _data.noise.set(d.problem.noise);
        _data.seed.setValue(td::Variant((td::INT4) d.problem.seed), false);
        updateEnabledState();
    }

    void updateEnabledState()
    {
        const auto& reg = pm::problemRegistry();
        const int idx = _cmbProblem.getSelectedIndex();
        const bool data = idx >= 0 && idx < (int) reg.size() && reg[idx].usesDataConfig;
        _data.points.enable(data);
        _data.noise.enable(data);
        _data.lblSeed.enable(data);
        _data.seed.enable(data);
        _data.newSeed.enable(data);
        _methods.alGrowth.enable(_methods.alAdaptive.isChecked());
    }

    void updateDescription()
    {
        const auto& reg = pm::problemRegistry();
        const int idx = _cmbProblem.getSelectedIndex();
        if (idx < 0 || idx >= (int) reg.size()) return;
        auto p = reg[idx].create(pm::ProblemConfig());
        _lblDescription.setTitle(fmt("%s  (n = %d, m = %d)", p->description().c_str(), (int) p->dim(), (int) p->numConstraints()));
    }

public:
    InspectorPanel()
    : _hdrExperiment(tr("secExperiment"), gui::Font::ID::SystemSmallerBold)
    , _lblDescription("", gui::Font::ID::SystemSmaller)
    , _hlActions(3)
    , _btnRun(tr("run"), tr("runTT"))
    , _btnStop(tr("stop"), tr("stopTT"))
    , _btnSweep(tr("sweep"), tr("sweepTT"))
    , _cbLive(tr("live"))
    , _hlExport(2)
    , _btnExportAll(tr("exportAll"), tr("exportAllTT"))
    , _btnOpenExports(tr("openExports"), tr("openExportsTT"))
    , _topGrid(7, 1)
    , _vl(2)
    {
        for (auto& e : pm::problemRegistry()) _cmbProblem.addItem(td::String(e.name.c_str()));
        _cmbProblem.selectIndex(0, false);
        _cmbProblem.setToolTip(tr("problemTT"));
        _cbLive.setToolTip(tr("liveTT"));
        _lblDescription.setResizable(10);          // long formulas shrink with an ellipsis
        _cmbProblem.setSizeLimits(180, gui::Control::Limit::UseAsMin);

        _btnRun.setAsDefault();
        _btnStop.enable(false);
        _btnRun.onClick([this]()          { if (_onRun) _onRun(); });
        _btnStop.onClick([this]()         { if (_onStop) _onStop(); });
        _btnSweep.onClick([this]()        { if (_onSweep) _onSweep(); });
        _btnExportAll.onClick([this]()    { if (_onExportAll) _onExportAll(); });
        _btnOpenExports.onClick([this]()  { if (_onOpenExports) _onOpenExports(); });
        _cbLive.onClick([this]()          { if (_cbLive.isChecked()) changed(); });

        _cmbProblem.onChangedSelection([this]() { updateEnabledState(); updateDescription(); changed(); });

        // every slider re-runs (when auto-run is on) as soon as it moves
        for (ParamSlider* s : { &_methods.penMu0, &_methods.penGrowth, &_methods.penMuMax, &_methods.alMu, &_methods.alGrowth,
                                &_stopping.feasTol, &_stopping.optTol, &_stopping.newtonTol, &_stopping.maxOuter,
                                &_stopping.maxNewton, &_data.points, &_data.noise })
            s->onChanged([this]() { changed(); });
        _methods.alAdaptive.onClick([this]() { updateEnabledState(); changed(); });
        _methods.reset.onClick([this]()      { applyDefaults(); changed(); });
        _stopping.condNorm.onChangedSelection([this]() { changed(); });
        _data.seed.onFinishEdit([this]()     { changed(); });
        _data.newSeed.onClick([this]()
        {
            std::random_device rd;
            _data.seed.setValue(td::Variant((td::INT4) (rd() % 1000000)), false);
            changed();
        });

        _hlActions << _btnRun << _btnStop;
        _hlActions.appendSpacer();
        _hlExport << _btnExportAll << _btnOpenExports;

        gui::GridComposer gc(_topGrid);
        gc.appendRow(_hdrExperiment);
        gc.appendRow(_cmbProblem);
        gc.appendRow(_lblDescription);
        gc.appendRow(_hlActions);
        gc.appendRow(_btnSweep);
        gc.appendRow(_cbLive);
        gc.appendRow(_hlExport);
        _topGrid.setSpaceBetweenCells(6, 6);
        _top.place(_topGrid, 12, 10, 12, 4);

        _tabs.addView(&_methods, tr("tabMethods"));
        _tabs.addView(&_stopping, tr("tabStopping"));
        _tabs.addView(&_data, tr("tabData"));

        _vl << _top << _tabs;
        setMargins(0, 0, 0, 0);
        setLayout(&_vl);

        applyDefaults();
        updateDescription();
    }

    // --- callbacks -------------------------------------------------
    void setOnRun(const std::function<void()>& f)             { _onRun = f; }
    void setOnStop(const std::function<void()>& f)            { _onStop = f; }
    void setOnSweep(const std::function<void()>& f)           { _onSweep = f; }
    void setOnSettingsChanged(const std::function<void()>& f) { _onSettingsChanged = f; }
    void setOnExportAll(const std::function<void()>& f)       { _onExportAll = f; }
    void setOnOpenExports(const std::function<void()>& f)     { _onOpenExports = f; }

    bool liveUpdate() const { return _cbLive.isChecked(); }

    void setRunning(bool running)
    {
        _btnRun.enable(!running);
        _btnSweep.enable(!running);
        _btnStop.enable(running);
    }

    pm::RunSettings settings() const
    {
        pm::RunSettings s;
        const int idx = _cmbProblem.getSelectedIndex();
        s.problemIndex = idx < 0 ? 0 : (size_t) idx;

        s.problem.nPoints = (int) std::lround(_data.points.get());
        s.problem.noise   = _data.noise.get();
        s.problem.seed    = (unsigned) (std::max)(0, (int) _data.seed.getValue().i4Val());

        pm::OuterOptions outer;
        outer.feasTol        = _stopping.feasTol.get();
        outer.optTol         = _stopping.optTol.get();
        outer.newton.gradTol = _stopping.newtonTol.get();
        outer.maxOuter       = (int) std::lround(_stopping.maxOuter.get());
        outer.newton.maxIter = (int) std::lround(_stopping.maxNewton.get());
        const int cn = _stopping.condNorm.getSelectedIndex();
        outer.condNorm = cn == 1 ? pm::CondNorm::One : cn == 2 ? pm::CondNorm::Two : pm::CondNorm::Inf;

        s.penalty.outer  = outer;
        s.penalty.mu0    = _methods.penMu0.get();
        s.penalty.growth = _methods.penGrowth.get();
        s.penalty.muMax  = (std::max)(s.penalty.mu0, _methods.penMuMax.get());

        s.al.outer    = outer;
        s.al.mu0      = _methods.alMu.get();
        s.al.adaptive = _methods.alAdaptive.isChecked();
        s.al.growth   = _methods.alGrowth.get();
        return s;
    }
};

} // namespace ui
