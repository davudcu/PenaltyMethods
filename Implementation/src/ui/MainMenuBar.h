#pragma once
#include <gui/MenuBar.h>

// ============================================================
// MainMenuBar: application menus.  Action IDs are handled in
// MainWindow::onActionItem.
// ============================================================
namespace ui
{

class MainMenuBar : public gui::MenuBar
{
    gui::SubMenu _app;
    gui::SubMenu _experiment;
    gui::SubMenu _view;
    gui::SubMenu _export;
    gui::SubMenu _help;

public:
    enum Menu : unsigned char { MenuApp = 10, MenuExperiment = 20, MenuView = 25, MenuExport = 30, MenuHelp = 40 };
    enum RunAction : unsigned char { ActRun = 10, ActSweep = 20, ActStop = 30 };
    enum ViewAction : unsigned char { ActPage0 = 10 };   // ActPage0 + page index
    enum ExportAction : unsigned char { ActPdf = 10, ActSvg = 20, ActAll = 30, ActCsv = 40, ActOpenFolder = 50 };
    enum HelpAction : unsigned char { ActAbout = 10 };

    MainMenuBar()
    : gui::MenuBar(5)
    , _app(MenuApp, tr("menuApp"), 1)
    , _experiment(MenuExperiment, tr("menuExperiment"), 4)
    , _view(MenuView, tr("menuView"), 5)
    , _export(MenuExport, tr("menuExport"), 6)
    , _help(MenuHelp, tr("menuHelp"), 1)
    {
        _app.getItems()[0].initAsQuitAppActionItem(tr("quit"), "q");

        auto& run = _experiment.getItems();
        run[0].initAsActionItem(tr("run"), ActRun, "r");
        run[1].initAsActionItem(tr("sweep"), ActSweep, "<Ctrl><Shift>r");
        run[2].initAsSeparator();
        run[3].initAsActionItem(tr("stop"), ActStop);

        auto& view = _view.getItems();
        view[0].initAsActionItem(tr("navSummary"), ActPage0 + 0, "1");
        view[1].initAsActionItem(tr("navLandscape"), ActPage0 + 1, "2");
        view[2].initAsActionItem(tr("navConvergence"), ActPage0 + 2, "3");
        view[3].initAsActionItem(tr("navConditioning"), ActPage0 + 3, "4");
        view[4].initAsActionItem(tr("navData"), ActPage0 + 4, "5");

        auto& exp = _export.getItems();
        exp[0].initAsActionItem(tr("exportAll"), ActAll, "e");
        exp[1].initAsActionItem(tr("exportPdf"), ActPdf);
        exp[2].initAsActionItem(tr("exportSvg"), ActSvg);
        exp[3].initAsActionItem(tr("exportCsv"), ActCsv);
        exp[4].initAsSeparator();
        exp[5].initAsActionItem(tr("openExports"), ActOpenFolder);

        _help.getItems()[0].initAsActionItem(tr("about"), ActAbout);

        setMenu(0, &_app);
        setMenu(1, &_experiment);
        setMenu(2, &_view);
        setMenu(3, &_export);
        setMenu(4, &_help);
    }
};

} // namespace ui
