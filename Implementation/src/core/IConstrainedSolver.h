#pragma once
#include "IProblem.h"
#include "MeritFunctions.h"
#include "NewtonSolver.h"
#include "SolveHistory.h"
#include <atomic>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <string>

// ============================================================
// IConstrainedSolver: abstract interface for equality-
// constrained methods.  Method-specific parameters are given to
// the concrete solver's constructor, so the interface stays the
// same for every method (LSP) and callers depend only on it (DIP).
// ============================================================
namespace pm
{

class IConstrainedSolver
{
public:
    virtual ~IConstrainedSolver() = default;

    virtual std::string name() const = 0;

    virtual void solve(const IProblem& problem,
                       const Vec& x0,
                       SolveHistory& history,
                       const std::atomic<bool>* cancel = nullptr) const = 0;
};

// Settings shared by all outer loops.
struct OuterOptions
{
    int      maxOuter = 30;
    double   feasTol  = 1e-8;   // ||h(x)||_inf
    double   optTol   = 1e-6;   // ||grad f + J' lambda||_inf
    CondNorm condNorm = CondNorm::Inf;
    NewtonOptions newton;
};

// ------------------------------------------------------------
// Helpers shared by concrete solvers (no method logic here).
// ------------------------------------------------------------
namespace detail
{

inline void beginHistory(const std::string& method, const IProblem& p, const Vec& x0,
                         CondNorm condNorm, SolveHistory& hist)
{
    hist.clear();
    hist.method = method;
    hist.problem = p.name();
    hist.condNorm = condNorm;
    hist.x0 = x0;
    hist.path.push_back(x0);
    hist.hasReference = p.referenceSolution(hist.xStar, hist.lambdaStar);
    if (hist.hasReference) hist.fStar = p.f(hist.xStar);
}

// Evaluates all diagnostics of subproblem k at its solution.
inline OuterRecord makeRecord(const IProblem& p,
                              const AugmentedLagrangianMerit& merit,
                              const NewtonReport& rep,
                              const Vec& lambdaEstimate,
                              const SolveHistory& hist,
                              int k)
{
    OuterRecord r;
    r.k = k;
    r.mu = merit.mu();
    r.lambdaUsed = merit.lambda();
    r.lambdaEstimate = lambdaEstimate;
    r.x = rep.x;
    r.pathEnd = hist.path.empty() ? 0 : hist.path.size() - 1;

    Vec h, g, jt;
    Mat J, H;
    p.constraints(rep.x, h);
    r.f = p.f(rep.x);
    r.merit = merit.value(rep.x);
    r.hNorm2 = norm2(h);
    r.hInf = normInf(h);

    p.gradF(rep.x, g);
    p.jacobian(rep.x, J);
    mulTransVec(J, lambdaEstimate, jt);
    for (size_t i = 0; i < g.size(); ++i) g[i] += jt[i];
    r.kktNorm = normInf(g);

    merit.hessian(rep.x, H);
    r.cond = conditionNumber(H, hist.condNorm);

    r.newtonIters = rep.iterations;
    r.cumulativeNewton = hist.totalNewton + rep.iterations;
    r.newtonStatus = newtonStatusName(rep.status);
    r.newtonGrad = rep.gradNorm;
    r.hessShift = rep.maxShift;
    r.linResidual = rep.maxLinResidual;

    if (hist.hasReference)
    {
        r.xError = p.solutionError(rep.x, hist.xStar);
        double le = 0;
        for (size_t i = 0; i < lambdaEstimate.size() && i < hist.lambdaStar.size(); ++i)
            le = (std::max)(le, std::abs(lambdaEstimate[i] - hist.lambdaStar[i]));
        r.lambdaError = le;
        r.fError = std::abs(r.f - hist.fStar);
    }
    return r;
}

class Stopwatch
{
    std::chrono::steady_clock::time_point _t0 = std::chrono::steady_clock::now();
public:
    double ms() const
    {
        return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - _t0).count();
    }
};

} // namespace detail
} // namespace pm
