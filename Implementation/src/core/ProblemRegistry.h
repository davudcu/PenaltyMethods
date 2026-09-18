#pragma once
#include "AnalyticProblems.h"
#include "ConicFitProblem.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

// ============================================================
// ProblemRegistry: the single place where test problems are
// listed.  To add a problem: implement IProblem and append one
// entry here.  Solvers and GUI pick it up automatically.
// ============================================================
namespace pm
{

struct ProblemEntry
{
    std::string name;
    bool usesDataConfig;   // enables the data-point / noise / seed inputs
    std::function<std::unique_ptr<IProblem>(const ProblemConfig&)> create;
};

inline const std::vector<ProblemEntry>& problemRegistry()
{
    static const std::vector<ProblemEntry> entries = {
        { "Conic curve fit (6-D)", true,
          [](const ProblemConfig& c) { return std::make_unique<ConicFitProblem>(c); } },
        { "Circle projection (2-D)", false,
          [](const ProblemConfig&) { return std::make_unique<CircleProjectionProblem>(); } },
        { "Linear objective on circle (2-D)", false,
          [](const ProblemConfig&) { return std::make_unique<LinearOnCircleProblem>(); } },
        { "Hock-Schittkowski 7 (2-D)", false,
          [](const ProblemConfig&) { return std::make_unique<HockSchittkowski7Problem>(); } },
        { "Equality-constrained QP (4-D)", false,
          [](const ProblemConfig&) { return std::make_unique<EqualityQPProblem>(); } },
    };
    return entries;
}

} // namespace pm
