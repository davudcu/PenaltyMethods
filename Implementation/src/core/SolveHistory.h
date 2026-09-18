#pragma once
#include "LinAlg.h"
#include <string>
#include <algorithm>
#include <vector>

// ============================================================
// SolveHistory: everything recorded during one constrained
// solve.  Every plot and table renders from this structure, and
// both methods fill it identically, which is what makes them
// directly comparable.
// ============================================================
namespace pm
{

enum class SolveStatus { NotRun, Converged, MaxOuterIterations, PenaltyLimit, Failed, Cancelled };

inline const char* solveStatusName(SolveStatus s)
{
    switch (s)
    {
        case SolveStatus::NotRun:             return "not run";
        case SolveStatus::Converged:          return "converged";
        case SolveStatus::MaxOuterIterations: return "max outer iterations";
        case SolveStatus::PenaltyLimit:       return "mu limit reached";
        case SolveStatus::Failed:             return "failed";
        case SolveStatus::Cancelled:          return "cancelled";
    }
    return "?";
}

// One outer iteration = one unconstrained subproblem solved by Newton.
struct OuterRecord
{
    int    k = 0;
    double mu = 0;             // penalty parameter used in the subproblem
    Vec    lambdaUsed;         // multipliers in the subproblem (zero for penalty)
    Vec    lambdaEstimate;     // multiplier estimate after the subproblem
    Vec    x;                  // subproblem solution x_k
    size_t pathEnd = 0;        // index of x_k in SolveHistory::path

    double f = 0;              // f(x_k)
    double merit = 0;          // Q(x_k; mu) or L_A(x_k; lambda, mu)
    double hNorm2 = 0;         // ||h(x_k)||_2
    double hInf = 0;           // ||h(x_k)||_inf
    double kktNorm = 0;        // ||grad f + J' lambdaEstimate||_inf
    double cond = 0;           // cond(Hessian of the merit function at x_k)

    int    newtonIters = 0;
    int    cumulativeNewton = 0;
    std::string newtonStatus;
    double newtonGrad = 0;     // ||grad merit||_inf returned by Newton
    double hessShift = 0;      // largest Hessian modification tau
    double linResidual = 0;    // largest relative residual of the Newton linear solve

    double xError = -1;        // ||x_k - x*||    (-1 when no reference)
    double lambdaError = -1;   // ||lambda_k - lambda*||_inf
    double fError = -1;        // |f(x_k) - f*|
};

struct SolveHistory
{
    std::string method;
    std::string problem;
    CondNorm    condNorm = CondNorm::Inf;

    Vec x0;
    std::vector<OuterRecord> outer;
    std::vector<Vec> path;          // x0 followed by every accepted Newton iterate

    bool hasReference = false;
    Vec  xStar, lambdaStar;
    double fStar = 0;

    SolveStatus status = SolveStatus::NotRun;
    std::string message;
    int    totalNewton = 0;
    double elapsedMs = 0;

    void clear()
    {
        *this = SolveHistory();
    }

    bool empty() const { return outer.empty(); }
    const OuterRecord& last() const { return outer.back(); }

    double maxCond() const
    {
        double c = 0;
        for (auto& r : outer) c = (std::max)(c, r.cond);
        return c;
    }
};

} // namespace pm
