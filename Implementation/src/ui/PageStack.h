#pragma once
#include <gui/View.h>
#include <gui/Label.h>
#include <gui/VerticalLayout.h>
#include <gui/ViewSwitcher.h>
#include "SummaryCanvas.h"
#include "LandscapePage.h"
#include "ConvergenceCanvas.h"
#include "ConditioningCanvas.h"
#include "IterationLogPage.h"

// ============================================================
// PageStack: the content area selected by the navigation rail.
// Every page has the same header (title + one-line purpose), so
// the user always knows what question the page answers.
// ============================================================
namespace ui
{

class PageFrame : public gui::View
{
    gui::Label          _title;
    gui::Label          _subtitle;
    gui::VerticalLayout _vl;

public:
    PageFrame(const td::String& title, const td::String& subtitle, gui::Control& content)
    : _title(title, gui::Font::ID::SystemLargestBold)
    , _subtitle(subtitle)
    , _vl(4)
    {
        _vl.appendSpace(4);
        _vl << _title << _subtitle << content;
        setMargins(16, 10, 10, 6);
        setLayout(&_vl);
    }
};

class PageStack : public gui::ViewSwitcher
{
    SummaryCanvas      _summary;
    LandscapePage      _landscape;
    ConvergenceCanvas  _convergence;
    ConditioningCanvas _conditioning;
    IterationLogPage   _data;

    PageFrame _pgSummary;
    PageFrame _pgLandscape;
    PageFrame _pgConvergence;
    PageFrame _pgConditioning;
    PageFrame _pgData;

public:
    enum Page { Summary = 0, Landscape, Convergence, Conditioning, Data, Count };

    PageStack()
    : gui::ViewSwitcher(Count)
    , _pgSummary(tr("pgSummary"), tr("pgSummarySub"), _summary)
    , _pgLandscape(tr("pgLandscape"), tr("pgLandscapeSub"), _landscape)
    , _pgConvergence(tr("pgConvergence"), tr("pgConvergenceSub"), _convergence)
    , _pgConditioning(tr("pgConditioning"), tr("pgConditioningSub"), _conditioning)
    , _pgData(tr("pgData"), tr("pgDataSub"), _data)
    {
        addView(&_pgSummary, true);
        addView(&_pgLandscape, false);
        addView(&_pgConvergence, false);
        addView(&_pgConditioning, false);
        addView(&_pgData, false);
    }

    void setResult(const ResultPtr& r)
    {
        _summary.setResult(r);
        _landscape.setResult(r);
        _convergence.setResult(r);
        _conditioning.setResult(r);
        _data.setResult(r);
    }

    void show(Page p) { showView((int) p); }

    // Canvas of the visible page, nullptr for the table page.
    gui::Canvas* currentCanvas()
    {
        switch (getCurrentViewPos())
        {
            case Summary:      return &_summary;
            case Landscape:    return &_landscape.canvas();
            case Convergence:  return &_convergence;
            case Conditioning: return &_conditioning;
            default:           return nullptr;
        }
    }
};

} // namespace ui
