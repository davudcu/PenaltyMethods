#pragma once
#include <gui/Window.h>
#include "ui/MainMenuBar.h"
#include "ui/StatusStrip.h"
#include "ui/Workspace.h"
#include "ui/WindowPlacement.h"

// ============================================================
// MainWindow: window chrome: menu, status line, central view.
// Menu actions are forwarded to the workspace.
// ============================================================
class MainWindow : public gui::Window
{
    ui::MainMenuBar _menu;
    ui::StatusStrip _statusStrip;
    ui::Workspace   _workspace;

protected:
    bool onActionItem(gui::ActionItemDescriptor& aiDesc) override
    {
        using M = ui::MainMenuBar;
        auto [menuID, firstSubMenuID, lastSubMenuID, actionID] = aiDesc.getIDs();
        (void) firstSubMenuID; (void) lastSubMenuID;

        switch (menuID)
        {
            case M::MenuExperiment:
                switch (actionID)
                {
                    case M::ActRun:   _workspace.runComparison();  return true;
                    case M::ActSweep: _workspace.runAllProblems(); return true;
                    case M::ActStop:  _workspace.stop();           return true;
                }
                break;

            case M::MenuView:
                if (actionID >= M::ActPage0 && actionID < M::ActPage0 + ui::PageStack::Count)
                {
                    _workspace.showPage(actionID - M::ActPage0);
                    return true;
                }
                break;

            case M::MenuExport:
                switch (actionID)
                {
                    case M::ActAll:        _workspace.exportAll();               return true;
                    case M::ActPdf:        _workspace.exportCurrentPage(true);   return true;
                    case M::ActSvg:        _workspace.exportCurrentPage(false);  return true;
                    case M::ActCsv:        _workspace.exportData();              return true;
                    case M::ActOpenFolder: _workspace.openExportsFolder();       return true;
                }
                break;

            case M::MenuHelp:
                if (actionID == M::ActAbout)
                {
                    showAlert(tr("aboutTitle"), tr("aboutText"));
                    return true;
                }
                break;
        }
        return false;
    }

public:
    MainWindow()
    : gui::Window(ui::fitToScreen(1440, 900))
    {
        setTitle(tr("appTitle"));
        _menu.setAsMain(this);
        setStatusBar(_statusStrip);
        _workspace.setOnStatus([this](const td::String& s) { _statusStrip.setMessage(s); });
        setCentralView(&_workspace);
    }

    bool shouldClose() override
    {
        _workspace.cancelAndWait();
        return true;
    }
};
