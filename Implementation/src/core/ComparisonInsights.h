#pragma once
#include "ComparisonRunner.h"
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

// ============================================================
// ComparisonInsights: turns two solve histories into
//   * a scorecard: one row per metric with the better method marked
//   * short plain-language findings for the Summary page
// GUI-free so the wording can be checked without the app.
// ============================================================
namespace pm
{

enum class Better { None = 0, Penalty, AugLag };

struct ScoreRow
{
    std::string metric;
    std::string penalty;
    std::string augLag;
    Better      better = Better::None;
};

struct ProblemConditioning
{
    std::string name;
    double penaltyMaxCond = 0;
    double alMaxCond = 0;
    double penaltySlope = 0;     // d log(cond) / d log(mu) for mu >= 1e2
};

class ComparisonInsights
{
    static std::string sci(double v)
    {
        char b[32];
        if (!std::isfinite(v)) return v > 0 ? "inf" : "n/a";
        std::snprintf(b, sizeof b, "%.2e", v);
        return b;
    }

    static std::string integer(long v)
    {
        char b[32];
        std::snprintf(b, sizeof b, "%ld", v);
        return b;
    }

    // Lower is better; a winner needs a clear margin (ratio) to avoid noise.
    static Better lower(double p, double a, double ratio = 1.0)
    {
        if (!std::isfinite(p) && !std::isfinite(a)) return Better::None;
        if (!std::isfinite(p)) return Better::AugLag;
        if (!std::isfinite(a)) return Better::Penalty;
        if (p * ratio < a) return Better::Penalty;
        if (a * ratio < p) return Better::AugLag;
        return Better::None;
    }

    static int stalls(const SolveHistory& h)
    {
        int n = 0;
        for (auto& r : h.outer) if (r.newtonStatus != "converged") ++n;
        return n;
    }

public:
    static double condSlope(const SolveHistory& h)
    {
        double sx = 0, sy = 0, sxx = 0, sxy = 0;
        int n = 0;
        for (auto& r : h.outer)
        {
            if (r.mu < 1e2 || !(r.cond > 0) || !std::isfinite(r.cond)) continue;
            const double x = std::log10(r.mu), y = std::log10(r.cond);
            sx += x; sy += y; sxx += x * x; sxy += x * y; ++n;
        }
        if (n < 2) return 0;
        const double den = n * sxx - sx * sx;
        return den != 0 ? (n * sxy - sx * sy) / den : 0;
    }

    static std::vector<ScoreRow> scorecard(const MethodComparison& mc)
    {
        const SolveHistory& p = mc.penalty;
        const SolveHistory& a = mc.al;
        std::vector<ScoreRow> rows;
        if (p.outer.empty() || a.outer.empty()) return rows;

        const bool pConv = p.status == SolveStatus::Converged, aConv = a.status == SolveStatus::Converged;
        rows.push_back({ "Outcome", p.message, a.message,
                         pConv == aConv ? Better::None : (pConv ? Better::Penalty : Better::AugLag) });
        rows.push_back({ "Subproblems solved", integer((long) p.outer.size()), integer((long) a.outer.size()),
                         lower((double) p.outer.size(), (double) a.outer.size()) });
        rows.push_back({ "Newton iterations", integer(p.totalNewton), integer(a.totalNewton),
                         lower(p.totalNewton, a.totalNewton) });
        rows.push_back({ "Constraint violation", sci(p.last().hInf), sci(a.last().hInf),
                         lower(p.last().hInf, a.last().hInf, 2.0) });
        if (p.hasReference)
        {
            rows.push_back({ "Distance to x*", sci(p.last().xError), sci(a.last().xError),
                             lower(p.last().xError, a.last().xError, 2.0) });
            rows.push_back({ "Multiplier error", sci(p.last().lambdaError), sci(a.last().lambdaError),
                             lower(p.last().lambdaError, a.last().lambdaError, 2.0) });
        }
        rows.push_back({ "Largest condition number", sci(p.maxCond()), sci(a.maxCond()),
                         lower(p.maxCond(), a.maxCond(), 2.0) });
        rows.push_back({ "Largest penalty parameter", sci(p.last().mu), sci(a.last().mu),
                         lower(p.last().mu, a.last().mu, 2.0) });
        rows.push_back({ "Newton stalls / limits hit", integer(stalls(p)), integer(stalls(a)),
                         lower(stalls(p), stalls(a)) });

        char tp[32], ta[32];
        std::snprintf(tp, sizeof tp, "%.2f ms", p.elapsedMs);
        std::snprintf(ta, sizeof ta, "%.2f ms", a.elapsedMs);
        rows.push_back({ "Wall time", tp, ta, lower(p.elapsedMs, a.elapsedMs, 1.5) });
        return rows;
    }

    static std::vector<std::string> findings(const MethodComparison& mc)
    {
        const SolveHistory& p = mc.penalty;
        const SolveHistory& a = mc.al;
        std::vector<std::string> out;
        if (p.outer.empty() || a.outer.empty()) return out;
        char b[400];

        // conditioning  (UTF-8: \xce\xba = kappa, \xce\xbc = mu)
        const double pFirst = p.outer.front().cond, pMax = p.maxCond(), aMax = a.maxCond();
        const double slope = condSlope(p);
        if (std::isfinite(pMax) && pFirst > 0)
        {
            char growth[64] = "";
            if (slope > 0) std::snprintf(growth, sizeof growth, ", growing like \xce\xbc^%.2f", slope);
            std::snprintf(b, sizeof b,
                "Conditioning: penalty Hessian \xce\xba rose from %.1e to %.1e (\xc3\x97%.1e) as \xce\xbc reached %.0e%s. "
                "The augmented Lagrangian stayed at \xce\xba \xe2\x89\xa4 %.1e.",
                pFirst, pMax, pMax / pFirst, p.last().mu, growth, aMax);
            out.push_back(b);
        }

        // feasibility and accuracy
        if (p.hasReference)
            std::snprintf(b, sizeof b,
                "Accuracy: distance to x* is %.1e with the penalty method and %.1e with the augmented Lagrangian; "
                "constraint violation %.1e vs %.1e.",
                p.last().xError, a.last().xError, p.last().hInf, a.last().hInf);
        else
            std::snprintf(b, sizeof b, "Feasibility: final constraint violation %.1e (penalty) vs %.1e (augmented Lagrangian).",
                          p.last().hInf, a.last().hInf);
        out.push_back(b);

        // work
        std::snprintf(b, sizeof b,
            "Work: %d subproblems / %d Newton iterations for the penalty method, %d / %d for the augmented Lagrangian.",
            (int) p.outer.size(), p.totalNewton, (int) a.outer.size(), a.totalNewton);
        out.push_back(b);

        // Newton degradation
        const int ps = stalls(p), as = stalls(a);
        if (ps > 0 || as > 0)
        {
            std::snprintf(b, sizeof b,
                "Newton: %d penalty subproblem(s) and %d augmented-Lagrangian subproblem(s) ended without reaching "
                "the gradient tolerance (rounding in \xce\xbc\xc2\xb7h(x) limits the attainable accuracy).",
                ps, as);
            out.push_back(b);
        }
        return out;
    }

    static std::vector<ProblemConditioning> conditioningAcrossProblems(const RunResult& r)
    {
        std::vector<ProblemConditioning> out;
        for (auto& mc : r.runs)
        {
            ProblemConditioning pc;
            pc.name = mc.problem->name();
            pc.penaltyMaxCond = mc.penalty.maxCond();
            pc.alMaxCond = mc.al.maxCond();
            pc.penaltySlope = condSlope(mc.penalty);
            out.push_back(pc);
        }
        return out;
    }
};

} // namespace pm
