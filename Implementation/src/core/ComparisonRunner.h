#pragma once
#include "ProblemRegistry.h"
#include "QuadraticPenaltySolver.h"
#include "AugmentedLagrangianSolver.h"
#include <memory>
#include <algorithm>
#include <atomic>
#include <vector>

// ============================================================
// ComparisonRunner: runs both constrained methods on identical
// problems with identical starting points and inner-solver
// settings.  GUI-free: used by the application and the tests.
// ============================================================
namespace pm
{

struct RunSettings
{
    size_t         problemIndex = 0;
    ProblemConfig  problem;
    PenaltyOptions penalty;
    ALOptions      al;
};

struct MethodComparison
{
    std::shared_ptr<const IProblem> problem;
    SolveHistory penalty;
    SolveHistory al;
};

struct RunResult
{
    RunSettings settings;
    bool sweep = false;                  // true: every registered problem was solved
    bool cancelled = false;
    std::vector<MethodComparison> runs;  // runs[0] is the selected problem

    const MethodComparison& primary() const { return runs.front(); }
};

class ComparisonRunner
{
public:
    static MethodComparison runProblem(size_t problemIndex, const RunSettings& s,
                                       const std::atomic<bool>* cancel)
    {
        const auto& registry = problemRegistry();
        MethodComparison mc;
        mc.problem = registry[problemIndex].create(s.problem);

        QuadraticPenaltySolver penalty(s.penalty);
        AugmentedLagrangianSolver al(s.al);
        const IConstrainedSolver& a = penalty;
        const IConstrainedSolver& b = al;

        const Vec x0 = mc.problem->initialPoint();
        a.solve(*mc.problem, x0, mc.penalty, cancel);
        b.solve(*mc.problem, x0, mc.al, cancel);
        return mc;
    }

    static RunResult run(const RunSettings& s, bool allProblems, const std::atomic<bool>* cancel)
    {
        RunResult res;
        res.settings = s;
        res.sweep = allProblems;

        const size_t nProblems = problemRegistry().size();
        const size_t first = (std::min)(s.problemIndex, nProblems - 1);
        res.runs.push_back(runProblem(first, s, cancel));

        if (allProblems)
            for (size_t i = 0; i < nProblems; ++i)
            {
                if (i == first) continue;
                if (cancel && cancel->load()) break;
                res.runs.push_back(runProblem(i, s, cancel));
            }

        res.cancelled = cancel && cancel->load();
        return res;
    }
};

} // namespace pm
