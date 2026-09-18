#pragma once
#include <gui/Application.h>
#include "MainWindow.h"

// ============================================================
// Application: natID application entry point
// SRP: creates the first window and hands off control.
// ============================================================
class Application : public gui::Application
{
protected:
    gui::Window* createInitialWindow() override
    {
        // normal window sized to fit the screen (see ui/WindowPlacement.h)
        setInitialFrameSize(gui::Window::FrameSize::UseSpecified);
        return new MainWindow();
    }

public:
    Application(int argc, const char** argv)
    : gui::Application(argc, argv)
    {}
};
