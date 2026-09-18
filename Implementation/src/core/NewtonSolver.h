#pragma once
#include "MeritFunctions.h"
#include <atomic>
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

// ============================================================
// NewtonSolver: inner unconstrained solver shared by both
// constrained methods.
//
// Each iteration:
//   1. g = grad phi(x), stop when ||g||_inf <= gradTol
//   2. H = Hess phi(x)
//   3. Hessian modification: if Cholesky of H fails, use H + tau I
//      with tau increased tenfold until positive definite
//      (Nocedal & Wright, Algorithm 3.3)
//   4. Newton step: solve (H + tau I) p = -g   (natID dense solve)
//   5. Armijo backtracking line search on phi
//
// Reported diagnostics show how Newton degrades on ill-conditioned
// Hessians: relative residual of the linear solve, the Hessian
// shift that was needed, and stalls of the line search when the
// decrease required is below rounding level.
// ============================================================
namespace pm
{

struct NewtonOptions
{
    double gradTol       = 1e-9;
    int    maxIter       = 100;
    double armijoC       = 1e-4;
    double backtrack     = 0.5;
    int    maxBacktracks = 60;
};

enum class NewtonStatus { Converged, MaxIterations, Stalled, NonFinite, Cancelled };

inline const char* newtonStatusName(NewtonStatus s)
{
    switch (s)
    {
        case NewtonStatus::Converged:     return "converged";
        case NewtonStatus::MaxIterations: return "max iterations";
        case NewtonStatus::Stalled:       return "stalled";
        case NewtonStatus::NonFinite:     return "non-finite";
        case NewtonStatus::Cancelled:     return "cancelled";
    }
    return "?";
}

struct NewtonReport
{
    Vec          x;
    NewtonStatus status         = NewtonStatus::MaxIterations;
    int          iterations     = 0;
    int          backtracks     = 0;
    double       value          = 0;
    double       gradNorm       = 0;   // ||grad phi||_inf at the returned x
    double       maxShift       = 0;   // largest tau used in H + tau I
    double       maxLinResidual = 0;   // max ||(H+tau I)p + g|| / ||g||
};

class NewtonSolver
{
    NewtonOptions _opt;

public:
    explicit NewtonSolver(const NewtonOptions& opt = NewtonOptions())
    : _opt(opt)
    {}

    const NewtonOptions& options() const { return _opt; }

    NewtonReport minimize(const ITwiceDifferentiable& phi,
                          const Vec& x0,
                          std::vector<Vec>* path = nullptr,
                          const std::atomic<bool>* cancel = nullptr) const
    {
        const size_t n = phi.dim();
        NewtonReport rep;
        rep.x = x0;

        Vec g, p, r, trial(n);
        Mat H;
        double val = phi.value(rep.x);
        phi.gradient(rep.x, g);

        for (;;)
        {
            rep.value = val;
            rep.gradNorm = normInf(g);

            if (!std::isfinite(val) || !allFinite(g)) { rep.status = NewtonStatus::NonFinite; break; }
            if (rep.gradNorm <= _opt.gradTol)          { rep.status = NewtonStatus::Converged; break; }
            if (rep.iterations >= _opt.maxIter)        { rep.status = NewtonStatus::MaxIterations; break; }
            if (cancel && cancel->load())              { rep.status = NewtonStatus::Cancelled; break; }

            // --- Hessian and modification ---------------------------
            phi.hessian(rep.x, H);
            if (!matAllFinite(H)) { rep.status = NewtonStatus::NonFinite; break; }

            Mat Hm = H.makeCopy();
            double tau = 0.0;
            if (!isPositiveDefinite(Hm))
            {
                tau = (std::max)(1e-3, 1e-3 * matNormInf(H));
                for (int k = 0; k < 80; ++k)
                {
                    Hm = H.makeCopy();
                    addDiagonal(Hm, tau);
                    if (isPositiveDefinite(Hm)) break;
                    tau *= 10.0;
                }
            }
            rep.maxShift = (std::max)(rep.maxShift, tau);

            // --- Newton direction -----------------------------------
            Vec rhs(n);
            for (size_t i = 0; i < n; ++i) rhs[i] = -g[i];
            bool haveStep = solveLinear(Hm, rhs, p);
            if (haveStep)
            {
                mulVec(Hm, p, r);
                for (size_t i = 0; i < n; ++i) r[i] += g[i];
                rep.maxLinResidual = (std::max)(rep.maxLinResidual, normInf(r) / (std::max)(rep.gradNorm, 1e-300));
            }

            double slope = haveStep ? dot(g, p) : 0.0;
            if (!haveStep || !(slope < 0.0))
            {
                p = rhs;                // steepest descent fallback
                slope = -dot(g, g);
            }

            // --- Rounding regime -------------------------------------
            // When the predicted decrease is below the rounding level of
            // phi, Armijo cannot distinguish progress from noise.  Take the
            // full Newton step if it reduces the gradient, else stop.
            const double roundLevel = 64.0 * std::numeric_limits<double>::epsilon() * (std::max)(1.0, std::abs(val));
            if (-slope <= roundLevel)
            {
                Vec gTrial;
                for (size_t i = 0; i < n; ++i) trial[i] = rep.x[i] + p[i];
                phi.gradient(trial, gTrial);
                if (!(normInf(gTrial) < rep.gradNorm))
                {
                    rep.status = NewtonStatus::Stalled;
                    break;
                }
                rep.x = trial;
                val = phi.value(rep.x);
                g = gTrial;
                ++rep.iterations;
                if (path) path->push_back(rep.x);
                continue;
            }

            // --- Armijo backtracking --------------------------------
            double alpha = 1.0, trialVal = val;
            bool accepted = false;
            for (int bt = 0; bt <= _opt.maxBacktracks; ++bt)
            {
                for (size_t i = 0; i < n; ++i) trial[i] = rep.x[i] + alpha * p[i];
                trialVal = phi.value(trial);
                if (std::isfinite(trialVal) && trialVal <= val + _opt.armijoC * alpha * slope)
                {
                    accepted = true;
                    break;
                }
                alpha *= _opt.backtrack;
                ++rep.backtracks;
            }

            if (!accepted)
            {
                // required decrease is below the rounding level of phi
                rep.status = NewtonStatus::Stalled;
                break;
            }

            rep.x = trial;
            val = trialVal;
            phi.gradient(rep.x, g);
            ++rep.iterations;
            if (path) path->push_back(rep.x);
        }
        return rep;
    }
};

} // namespace pm
