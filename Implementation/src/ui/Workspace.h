#pragma once
#include <gui/View.h>
#include <gui/SplitterLayout.h>
#include <gui/HorizontalLayout.h>
#include <gui/Thread.h>
#include "NavRail.h"
#include "PageStack.h"
#include "InspectorPanel.h"
#include "AppFolders.h"
#include "../core/HistoryCsv.h"
#include <atomic>
#include <chrono>
#include <fstream>
#include <thread>
#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

// ============================================================
// Workspace: central view.
//
//   ┌──────┬────────────────────────────────────┬──────────────┐
//   │ Nav  │ Page (Summary / Landscape /        │ Inspector    │
//   │ rail │ Convergence / Conditioning / Data) │ (experiment) │
//   └──────┴────────────────────────────────────┴──────────────┘
//
// Also the run controller: solvers execute on a worker thread and
// the immutable RunResult is handed to the pages on the main thread
// through gui::thread::asyncExecInMainThread.
// ============================================================
namespace ui
{

class ContentArea : public gui::View
{
    gui::SplitterLayout _splitter;
public:
    PageStack      pages;
    InspectorPanel inspector;

    ContentArea()
    : _splitter(gui::SplitterLayout::Orientation::Horizontal, gui::SplitterLayout::AuxiliaryCell::Second)
    {
        setMargins(0, 0, 0, 0);
        _splitter.setContent(pages, inspector);
        setLayout(&_splitter);
    }
};

class Workspace : public gui::View
{
    gui::HorizontalLayout _hl;
    NavRail               _nav;
    ContentArea           _content;

    std::thread         _worker;
    std::atomic<bool>   _cancel { false };
    bool                _running = false;
    bool                _rerunPending = false;
    std::shared_ptr<int> _alive = std::make_shared<int>(1);
    ResultPtr           _result;

    std::function<void(const td::String&)> _onStatus;

    // ---- export state ----------------------------------------------
    fo::fs::path     _exportDir;          // folder of the current result, created on first export
    std::vector<int> _exportQueue;        // pages still to export in "Export all"
    int              _exportFiles = 0;
    int              _exportReturnPage = 0;
    bool             _exporting = false;

    static const char* pageStem(int page)
    {
        switch (page)
        {
            case PageStack::Summary:      return "summary";
            case PageStack::Landscape:    return "landscape";
            case PageStack::Convergence:  return "convergence";
            case PageStack::Conditioning: return "conditioning";
            default:                      return "page";
        }
    }

    // One folder per result: every export of the same run lands together.
    const fo::fs::path& exportDir()
    {
        std::error_code ec;
        if (_exportDir.empty() || !fo::fs::is_directory(_exportDir, ec))
        {
            std::string label = _result && !_result->runs.empty() ? _result->primary().problem->name() : std::string("export");
            if (_result && _result->sweep) label = "all-problems";
            _exportDir = appfs::createExportFolder(label);
        }
        return _exportDir;
    }

    // Runs fn on the main thread after a short delay (lets a page become visible).
    void later(int ms, std::function<void()> fn)
    {
        std::weak_ptr<int> alive = _alive;
        std::thread([alive, ms, fn]()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(ms));
            gui::thread::asyncExecInMainThread([alive, fn]() { if (!alive.expired()) fn(); });
        }).detach();
    }

    static bool exportCanvas(gui::Canvas* canvas, const fo::fs::path& target, bool pdf)
    {
        if (!canvas) return false;
        fo::fs::path base = target;
        base.replace_extension("");
        const td::String baseName(appfs::toUtf8(base).c_str());
        const bool ok = pdf ? canvas->exportToPDF(baseName, true) : canvas->exportToSVG(baseName, true);

        // natID appends the extension itself; keep a fallback if it does not
        std::error_code ec;
        if (ok && !fo::fs::exists(target, ec) && fo::fs::exists(base, ec))
            fo::fs::rename(base, target, ec);
        return ok && fo::fs::exists(target, ec);
    }

    int writeData(const fo::fs::path& dir)
    {
        if (!_result) return 0;
        int files = 0;
        {
            std::ofstream f(appfs::uniqueFile(dir, "iterations", ".csv"));
            pm::HistoryCsv::writeIterations(f, *_result);
            if (f.good()) ++files;
        }
        {
            std::ofstream f(appfs::uniqueFile(dir, "summary", ".csv"));
            pm::HistoryCsv::writeSummary(f, *_result);
            if (f.good()) ++files;
        }
        return files;
    }

    void reportExport(const td::String& what, int files)
    {
        status(fmt("%s: %d %s %s", what.c_str(), files, tr("exportFilesIn").c_str(), appfs::toUtf8(_exportDir).c_str()));
    }

    // "Export all": show each chart page, wait until natID can render it, export it.
    void exportNextPage(int attempt)
    {
        if (_exportQueue.empty())
        {
            showPage(_exportReturnPage);
            _exporting = false;
            reportExport(tr("exportAllDone"), _exportFiles);
            return;
        }
        const int page = _exportQueue.front();
        if (_content.pages.getCurrentViewPos() != page) showPage(page);

        later(150, [this, page, attempt]()
        {
            if (exportCanvas(_content.pages.currentCanvas(), appfs::uniqueFile(exportDir(), pageStem(page), ".pdf"), true))
                ++_exportFiles;
            else if (attempt < 20)
            {
                exportNextPage(attempt + 1);     // page not ready yet: try again shortly
                return;
            }
            _exportQueue.erase(_exportQueue.begin());
            exportNextPage(0);
        });
    }

    void status(const td::String& s)
    {
        if (_onStatus) _onStatus(s);
    }

    void onFinished(const ResultPtr& res, const td::String& error)
    {
        if (_worker.joinable()) _worker.join();
        _running = false;
        _content.inspector.setRunning(false);

        if (error.length() > 0)
        {
            status(fmt("%s %s", tr("runFailed").c_str(), error.c_str()));
            showAlert(tr("runFailed"), error);
        }
        else if (res && !res->runs.empty())
        {
            _result = res;
            if (!_exporting) _exportDir.clear();      // new result -> new export folder
            _content.pages.setResult(res);
            const auto& mc = res->primary();
            status(fmt("%s%s   %s   %s: %s, %d subproblems, max %s %.1e   %s   %s: %s, %d subproblems, max %s %.1e",
                       res->cancelled ? tr("cancelledPrefix").c_str() : "",
                       mc.problem->name().c_str(), "\xc2\xb7",
                       tr("penaltyShort").c_str(), mc.penalty.message.c_str(), (int) mc.penalty.outer.size(),
                       glyph::kappa, mc.penalty.maxCond(), "\xc2\xb7",
                       tr("augLagShort").c_str(), mc.al.message.c_str(), (int) mc.al.outer.size(),
                       glyph::kappa, mc.al.maxCond()));
        }

        if (_rerunPending)
        {
            _rerunPending = false;
            start(false);
        }
    }

    void start(bool sweep)
    {
        if (_running)
        {
            _rerunPending = !sweep;
            return;
        }
        const pm::RunSettings settings = _content.inspector.settings();
        _cancel = false;
        _running = true;
        _content.inspector.setRunning(true);
        status(tr(sweep ? "runningSweep" : "running"));

        std::weak_ptr<int> alive = _alive;
        _worker = std::thread([this, settings, sweep, alive]()
        {
            ResultPtr res;
            td::String error;
            try
            {
                res = std::make_shared<const pm::RunResult>(pm::ComparisonRunner::run(settings, sweep, &_cancel));
            }
            catch (const std::exception& ex)
            {
                error = td::String(ex.what());
            }
            catch (...)
            {
                error = td::String("unknown error");
            }
            gui::thread::asyncExecInMainThread([this, res, error, alive]()
            {
                if (alive.expired()) return;
                onFinished(res, error);
            });
        });
    }

public:
    Workspace()
    : _hl(2)
    {
        _nav.addItem(tr("navSummary"), NavRail::Icon::Summary);
        _nav.addItem(tr("navLandscape"), NavRail::Icon::Landscape);
        _nav.addItem(tr("navConvergence"), NavRail::Icon::Convergence);
        _nav.addItem(tr("navConditioning"), NavRail::Icon::Conditioning);
        _nav.addItem(tr("navData"), NavRail::Icon::Data);
        _nav.setOnSelect([this](int page)
        {
            _content.pages.show((PageStack::Page) page);
        });

        InspectorPanel& ins = _content.inspector;
        ins.setOnRun([this]()   { runComparison(); });
        ins.setOnSweep([this]() { runAllProblems(); });
        ins.setOnStop([this]()  { stop(); });
        ins.setOnExportAll([this]()   { exportAll(); });
        ins.setOnOpenExports([this]() { openExportsFolder(); });
        ins.setOnSettingsChanged([this]()
        {
            if (_content.inspector.liveUpdate()) runComparison();
        });

        setMargins(0, 0, 0, 0);
        _hl.setSpaceBetweenCells(0);
        _hl.append(_nav, td::HAlignment::Left, td::VAlignment::Top);   // rail pinned to the top
        _hl << _content;
        setLayout(&_hl);
    }

    ~Workspace()
    {
        _alive.reset();
        _cancel = true;
        if (_worker.joinable()) _worker.join();
    }

    void onInitialAppearance() override
    {
        runComparison();
    }

    void setOnStatus(const std::function<void(const td::String&)>& f) { _onStatus = f; }

    bool isRunning() const { return _running; }

    void runComparison() { start(false); }

    void runAllProblems()
    {
        start(true);
    }

    void showPage(int page)
    {
        _nav.select(page, true);
    }

    void stop()
    {
        if (!_running) return;
        _rerunPending = false;
        _cancel = true;
        status(tr("stopping"));
    }

    // Blocks until a running solve has stopped (used before closing).
    void cancelAndWait()
    {
        _rerunPending = false;
        _cancel = true;
        if (_worker.joinable()) _worker.join();
        _running = false;
    }

    // ---- exports (no dialogs, see AppFolders.h) -------------------
    void exportAll()
    {
        if (_exporting) return;
        if (!_result)
        {
            showAlert(tr("exportTitle"), tr("exportNoResult"));
            return;
        }
        _exportDir.clear();                       // every "Export all" gets its own dated folder
        exportDir();
        _exporting = true;
        _exportReturnPage = _content.pages.getCurrentViewPos();
        _exportFiles = writeData(_exportDir);
        _exportQueue = { PageStack::Summary, PageStack::Landscape, PageStack::Convergence, PageStack::Conditioning };
        status(tr("exportRunning"));
        exportNextPage(0);
    }

    void exportCurrentPage(bool pdf)
    {
        if (!_result)
        {
            showAlert(tr("exportTitle"), tr("exportNoResult"));
            return;
        }
        const int page = _content.pages.getCurrentViewPos();
        gui::Canvas* canvas = _content.pages.currentCanvas();
        if (!canvas)
        {
            showAlert(tr("exportTitle"), tr("exportNotAPlot"));
            return;
        }
        if (exportCanvas(canvas, appfs::uniqueFile(exportDir(), pageStem(page), pdf ? ".pdf" : ".svg"), pdf))
            reportExport(tr("exportDone"), 1);
        else
            showAlert(tr("exportTitle"), tr("exportFailed"));
    }

    void exportData()
    {
        if (!_result)
        {
            showAlert(tr("exportTitle"), tr("exportNoResult"));
            return;
        }
        reportExport(tr("exportDataDone"), writeData(exportDir()));
    }

    void openExportsFolder()
    {
        std::error_code ec;
        const fo::fs::path dir = (!_exportDir.empty() && fo::fs::is_directory(_exportDir, ec)) ? _exportDir : appfs::exportsFolder();
        if (!appfs::openFolder(dir))
            showAlert(tr("exportTitle"), td::String(appfs::toUtf8(dir).c_str()));
    }
};

} // namespace ui
