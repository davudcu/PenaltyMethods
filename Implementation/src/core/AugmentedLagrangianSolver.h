#pragma once
#include "IConstrainedSolver.h"
#include <algorithm>
#include <atomic>
#include <string>

// ============================================================
// Augmented Lagrangian method (Nocedal & Wright, Framework 17.3)
//
//   for k = 0, 1, 2, ...
//       x_k = argmin L_A(x; lambda_k, mu_k)     (Newton, warm start)
//       lambda_{k+1} = lambda_k + mu_k h(x_k)
//       stop when ||h(x_k)|| <= feasTol and ||grad L|| <= optTol
//       mu stays fixed, or (adaptive) grows only when the constraint
//       violation did not shrink by the factor 'sufficientDecrease'
//
// Because the multiplier update removes the need for mu -> infinity,
// the Hessian of L_A stays bounded and so does its condition number.
// ============================================================
namespace pm
{

struct ALOptions
{
    OuterOptions outer;
    double mu0                = 10.0;
    bool   adaptive           = false;
    double growth             = 5.0;
    double sufficientDecrease = 0.25;
    double muMax              = 1e8;
};

class AugmentedLagrangianSolver : public IConstrainedSolver
{
    ALOptions _opt;

public:
    explicit AugmentedLagrangianSolver(const ALOptions& opt)
    : _opt(opt)
    {}

    std::string name() const override { return "Augmented Lagrangian"; }

    void solve(const IProblem& p, const Vec& x0, SolveHistory& hist,
               const std::atomic<bool>* cancel = nullptr) const override
    {
        detail::Stopwatch clock;
        detail::beginHistory(name(), p, x0, _opt.outer.condNorm, hist);

        NewtonSolver newton(_opt.outer.newton);
        Vec x = x0;
        Vec lambda(p.numConstraints(), 0.0);
        double mu = _opt.mu0;
        double prevViolation = kInf;
        hist.status = SolveStatus::MaxOuterIterations;

        for (int k = 0; k < _opt.outer.maxOuter; ++k)
        {
            AugmentedLagrangianMerit merit(p, lambda, mu);
            NewtonReport rep = newton.minimize(merit, x, &hist.path, cancel);

            if (rep.status == NewtonStatus::Cancelled) { hist.status = SolveStatus::Cancelled; break; }
            if (rep.status == NewtonStatus::NonFinite)
            {
                hist.status = SolveStatus::Failed;
                hist.message = "Newton produced non-finite values";
                break;
            }

            x = rep.x;
            Vec h;
            p.constraints(x, h);
            Vec lambdaNext(lambda.size());
            for (size_t i = 0; i < h.size(); ++i) lambdaNext[i] = lambda[i] + mu * h[i];

            OuterRecord rec = detail::makeRecord(p, merit, rep, lambdaNext, hist, k);
            hist.totalNewton = rec.cumulativeNewton;
            hist.outer.push_back(rec);

            lambda = lambdaNext;

            if (rec.hInf <= _opt.outer.feasTol && rec.kktNorm <= _opt.outer.optTol)
            {
                hist.status = SolveStatus::Converged;
                break;
            }

            if (_opt.adaptive && rec.hInf > _opt.sufficientDecrease * prevViolation)
                mu = (std::min)(mu * _opt.growth, _opt.muMax);
            prevViolation = rec.hInf;
        }

        if (hist.message.empty()) hist.message = solveStatusName(hist.status);
        hist.elapsedMs = clock.ms();
    }
};

} // namespace pm
