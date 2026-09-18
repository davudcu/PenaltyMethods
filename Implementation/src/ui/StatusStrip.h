#pragma once
#include <gui/StatusBar.h>
#include <gui/Label.h>

// ============================================================
// StatusStrip: single message line at the bottom of the window.
// ============================================================
namespace ui
{

class StatusStrip : public gui::StatusBar
{
    gui::Label _message;

public:
    StatusStrip()
    : gui::StatusBar(3)
    , _message(tr("ready"))
    {
        _message.setResizable();
        _layout.appendSpace(12);   // keeps the text off the window edge
        _layout << _message;
        _layout.appendSpace(12);
        setLayout(&_layout);
    }

    void setMessage(const td::String& text) { _message.setTitle(text); }
};

} // namespace ui
