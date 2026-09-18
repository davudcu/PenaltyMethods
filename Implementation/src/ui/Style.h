#pragma once
#include <td/ColorID.h>
#include <td/LinePattern.h>
#include <td/String.h>
#include <cmath>
#include <cstdarg>
#include <cstdio>

// ============================================================
// Style: shared colours, glyphs and number formatting for all
// views, so both methods look the same in every plot.
// ============================================================
namespace ui
{

// UTF-8 glyphs (kept as byte escapes so the source encoding does
// not matter on any compiler).
namespace glyph
{
    inline constexpr const char* mu     = "\xce\xbc";
    inline constexpr const char* kappa  = "\xce\xba";
    inline constexpr const char* lambda = "\xce\xbb";
    inline constexpr const char* theta  = "\xce\xb8";
    inline constexpr const char* nabla  = "\xe2\x88\x87";
    inline constexpr const char* sq     = "\xc2\xb2";
    inline constexpr const char* inf    = "\xe2\x88\x9e";
    inline constexpr const char* norm   = "\xe2\x80\x96";
    inline constexpr const char* star   = "*";
    inline constexpr const char* dash   = "\xe2\x80\x94";
}

// ---- print mode ----------------------------------------------
// Exports (PDF/SVG) must not use theme colours: in dark mode the
// system text colour is white and would vanish on a white page.
// ChartCanvas switches this on while natID renders to an export backend.
inline bool& printModeFlag()
{
    static thread_local bool on = false;
    return on;
}

struct PrintScope
{
    bool previous;
    explicit PrintScope(bool on) : previous(printModeFlag()) { printModeFlag() = on; }
    ~PrintScope() { printModeFlag() = previous; }
};

inline td::ColorID textColor()  { return printModeFlag() ? td::ColorID::Black : td::ColorID::SysText; }
inline td::ColorID panelColor() { return printModeFlag() ? td::ColorID::White : td::ColorID::SysCtrlBack; }

// Series identity, reused by every chart and by the legend.
struct SeriesStyle
{
    td::ColorID     color;
    td::LinePattern pattern;
    float           width;
    bool            squareMarker;   // circle for penalty, square for AL
};

inline constexpr SeriesStyle kPenaltyStyle   { td::ColorID::OrangeRed,  td::LinePattern::Solid, 2.0f, false };
inline constexpr SeriesStyle kALStyle        { td::ColorID::RoyalBlue,  td::LinePattern::Solid, 2.0f, true  };
inline constexpr SeriesStyle kReferenceStyle { td::ColorID::ForestGreen, td::LinePattern::Dash, 1.5f, false };

inline constexpr td::ColorID kGuideColor     = td::ColorID::Gray;
inline constexpr td::ColorID kConstraintColor = td::ColorID::LimeGreen;

// Distinct colours for multi-problem overlays.
inline td::ColorID problemColor(size_t i)
{
    static const td::ColorID colors[] = {
        td::ColorID::OrangeRed, td::ColorID::RoyalBlue, td::ColorID::ForestGreen,
        td::ColorID::DarkViolet, td::ColorID::DarkGoldenRod, td::ColorID::Teal
    };
    return colors[i % (sizeof(colors) / sizeof(colors[0]))];
}

// Landscape heat-map levels, light (low merit) to dark (high merit).
inline constexpr td::ColorID kHeatLevels[] = {
    td::ColorID::White, td::ColorID::WhiteSmoke, td::ColorID::Gainsboro, td::ColorID::LightGray,
    td::ColorID::Silver, td::ColorID::DarkGray, td::ColorID::Gray, td::ColorID::DimGray
};
inline constexpr int kHeatLevelCount = sizeof(kHeatLevels) / sizeof(kHeatLevels[0]);

// ---- number formatting -------------------------------------
inline td::String fmtSci(double v, int digits = 3)
{
    char buf[48];
    if (!std::isfinite(v)) std::snprintf(buf, sizeof buf, "%s", v > 0 ? glyph::inf : "n/a");
    else std::snprintf(buf, sizeof buf, "%.*e", digits, v);
    return td::String(buf);
}

inline td::String fmt(const char* format, ...)
{
    char buf[512];
    va_list args;
    va_start(args, format);
    std::vsnprintf(buf, sizeof buf, format, args);
    va_end(args);
    return td::String(buf);
}

} // namespace ui
