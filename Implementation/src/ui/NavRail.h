#pragma once
#include <gui/Canvas.h>
#include <gui/Shape.h>
#include <gui/DrawableString.h>
#include "Style.h"
#include <functional>
#include <vector>

// ============================================================
// NavRail: vertical page navigation on the left edge.
//
// Icons are drawn with vector shapes in the current text colour,
// so they match light and dark themes and look identical on
// Windows, macOS and Linux (no bitmap icon set needed).
// Drawn as a rounded panel sized to its items; selected page is filled
// with the accent colour, hovered page with a light grey.
// ============================================================
namespace ui
{

class NavRail : public gui::Canvas
{
public:
    enum class Icon { Summary, Landscape, Convergence, Conditioning, Data };

private:
    struct Item { td::String label; Icon icon; };

    std::vector<Item> _items;
    int _selected = 0;
    int _hover = -1;
    std::function<void(int)> _onSelect;

    static constexpr double kWidth  = 92;
    static constexpr double kBrandH = 10;     // top padding
    static constexpr double kItemH  = 68;
    static constexpr td::ColorID kAccent = td::ColorID::RoyalBlue;

    double contentHeight() const { return kBrandH + _items.size() * kItemH + kBrandH; }

    int hitTest(const gui::Point& p) const
    {
        if (p.y < kBrandH) return -1;
        const int i = (int) ((p.y - kBrandH) / kItemH);
        return (i >= 0 && i < (int) _items.size()) ? i : -1;
    }

    static void drawIcon(Icon icon, double cx, double cy, td::ColorID c)
    {
        const float w = 1.8f;
        switch (icon)
        {
            case Icon::Summary:            // report card with lines and a check mark
            {
                gui::Shape card;
                card.createRoundedRect(gui::Rect(cx - 12, cy - 13, cx + 12, cy + 13), 3, w);
                card.drawWire(c);
                gui::Shape::drawLine({ cx - 7, cy - 6 }, { cx + 7, cy - 6 }, c, w);
                gui::Shape::drawLine({ cx - 7, cy - 1 }, { cx + 3, cy - 1 }, c, w);
                gui::Shape::drawLine({ cx - 7, cy + 7 }, { cx - 3, cy + 10 }, c, w);
                gui::Shape::drawLine({ cx - 3, cy + 10 }, { cx + 7, cy + 3 }, c, w);
                break;
            }
            case Icon::Landscape:          // nested contour ellipses
            {
                for (int k = 0; k < 3; ++k)
                {
                    const double rx = 14 - 4.5 * k, ry = 9 - 3.0 * k;
                    gui::Shape oval;
                    oval.createOval(gui::Rect(cx - rx, cy - ry, cx + rx, cy + ry), w);
                    oval.drawWire(c);
                }
                gui::Shape::drawLine({ cx - 14, cy + 13 }, { cx + 14, cy - 13 }, c, 1.2f, td::LinePattern::Dash);
                break;
            }
            case Icon::Convergence:        // axes with a decaying curve
            {
                gui::Shape::drawLine({ cx - 13, cy - 13 }, { cx - 13, cy + 12 }, c, w);
                gui::Shape::drawLine({ cx - 13, cy + 12 }, { cx + 14, cy + 12 }, c, w);
                const gui::Point pts[] = { { cx - 10, cy - 10 }, { cx - 5, cy - 1 }, { cx, cy + 4 }, { cx + 6, cy + 7 }, { cx + 13, cy + 8 } };
                gui::Shape curve;
                curve.createPolyLine(pts, 5, w);
                curve.drawWire(c);
                break;
            }
            case Icon::Conditioning:       // axes with a steeply rising curve and a flat one
            {
                gui::Shape::drawLine({ cx - 13, cy - 13 }, { cx - 13, cy + 12 }, c, w);
                gui::Shape::drawLine({ cx - 13, cy + 12 }, { cx + 14, cy + 12 }, c, w);
                const gui::Point rise[] = { { cx - 10, cy + 8 }, { cx - 1, cy + 4 }, { cx + 6, cy - 3 }, { cx + 12, cy - 13 } };
                gui::Shape r;
                r.createPolyLine(rise, 4, w);
                r.drawWire(c);
                gui::Shape::drawLine({ cx - 10, cy + 5 }, { cx + 13, cy + 5 }, c, 1.2f, td::LinePattern::Dash);
                break;
            }
            case Icon::Data:               // table
            {
                gui::Shape frame;
                frame.createRoundedRect(gui::Rect(cx - 14, cy - 11, cx + 14, cy + 11), 2, w);
                frame.drawWire(c);
                gui::Shape::drawLine({ cx - 14, cy - 4 }, { cx + 14, cy - 4 }, c, w);
                gui::Shape::drawLine({ cx - 14, cy + 4 }, { cx + 14, cy + 4 }, c, 1.0f);
                gui::Shape::drawLine({ cx - 3, cy - 11 }, { cx - 3, cy + 11 }, c, 1.0f);
                break;
            }
        }
    }

protected:
    void onDraw(const gui::Rect&) override
    {
        gui::Size sz;
        getSize(sz);
        const double W = sz.width, H = sz.height;

        (void) H;
        // rounded panel sized to the items (the layout may not stretch the rail)
        gui::Shape panel;
        panel.createRoundedRect(gui::Rect(4, 4, W - 4, contentHeight() - 4), 10, 1);
        panel.drawFill(td::ColorID::Gainsboro);

        for (int i = 0; i < (int) _items.size(); ++i)
        {
            const double top = kBrandH + i * kItemH;
            const gui::Rect cell(6, top + 3, W - 6, top + kItemH - 3);
            const bool sel = i == _selected;
            if (sel)
            {
                gui::Shape bg;
                bg.createRoundedRect(cell, 8, 1);
                bg.drawFill(kAccent);
            }
            else if (i == _hover)
            {
                gui::Shape bg;
                bg.createRoundedRect(cell, 8, 1);
                bg.drawFill(td::ColorID::Silver);
            }

            // fixed light panel, so fixed dark/white text works in light and dark themes
            const td::ColorID fg = sel ? td::ColorID::White : td::ColorID::DarkSlateGray;
            drawIcon(_items[i].icon, W / 2, top + 26, fg);
            gui::DrawableString::draw(_items[i].label, gui::Rect(2, top + 42, W - 2, top + kItemH - 6),
                                      sel ? gui::Font::ID::SystemSmallerBold : gui::Font::ID::SystemSmaller, fg,
                                      td::TextAlignment::Center, td::VAlignment::Center);
        }
    }

    void onPrimaryButtonPressed(const gui::InputDevice& inputDevice) override
    {
        const int i = hitTest(inputDevice.getFramePoint());
        if (i < 0 || i == _selected) return;
        select(i, true);
    }

    void onCursorMoved(const gui::InputDevice& inputDevice) override
    {
        const int i = hitTest(inputDevice.getFramePoint());
        if (i != _hover)
        {
            _hover = i;
            setCursor(i >= 0 ? gui::Cursor::Type::Finger : gui::Cursor::Type::Default, true);
            reDraw();
        }
    }

    void onCursorExited(const gui::InputDevice&) override
    {
        if (_hover != -1)
        {
            _hover = -1;
            reDraw();
        }
    }

    void onResize(const gui::Size&) override
    {
        reDraw();
    }

public:
    NavRail()
    : gui::Canvas({ gui::InputDevice::Event::PrimaryClicks, gui::InputDevice::Event::CursorMove,
                    gui::InputDevice::Event::CursorEnterLeave })
    {
        enableResizeEvent(true);
        setSizeLimits((td::UINT2) kWidth, gui::Control::Limit::Fixed, 200, gui::Control::Limit::UseAsMin);
    }

    void addItem(const td::String& label, Icon icon)
    {
        _items.push_back({ label, icon });
        // the rail is always tall enough to show every item
        setSizeLimits((td::UINT2) kWidth, gui::Control::Limit::Fixed, (td::UINT2) contentHeight(), gui::Control::Limit::UseAsMin);
    }

    void setOnSelect(const std::function<void(int)>& f) { _onSelect = f; }

    int selected() const { return _selected; }

    void select(int i, bool notify)
    {
        if (i < 0 || i >= (int) _items.size()) return;
        _selected = i;
        reDraw();
        if (notify && _onSelect) _onSelect(i);
    }
};

} // namespace ui
