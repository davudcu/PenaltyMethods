#pragma once
#include "IConstrainedSolver.h"
#include <atomic>
#include <string>

// ============================================================
// Quadratic penalty method (Nocedal & Wright, Framework 17.1)
//
//   for k = 0, 1, 2, ...
//       x_k = argmin Q(x; mu_k),  Q = f + mu_k/2 ||h||^2
//             (Newton, warm-started from x_{k-1})
//       lambda_k = mu_k h(x_k)          multiplier estimate
//       stop when ||h(x_k)|| <= feasTol and ||grad L|| <= optTol
//       mu_{k+1} = growth * mu_k
//
// Feasibility is reached only as mu -> infinity: h(x_k) ~ -lambda*/mu_k.
// Hess Q = Hess L + mu J'J has m eigenvalues of order mu, so its
// condition number grows like O(mu).
// ============================================================
namespace pm
{

struct PenaltyOptions
{
    OuterOptions outer;
    double mu0    = 1.0;
    double growth = 10.0;
    double muMax  = 1e12;
};

class QuadraticPenaltySolver : public IConstrainedSolver
{
    PenaltyOptions _opt;

public:
    explicit QuadraticPenaltySolver(const PenaltyOptions& opt)
    : _opt(opt)
    {}

    std::string name() const override { return "Quadratic penalty"; }

    void solve(const IProblem& p, const Vec& x0, SolveHistory& hist,
               const std::atomic<bool>* cancel = nullptr) const override
    {
        detail::Stopwatch clock;
        detail::beginHistory(name(), p, x0, _opt.outer.condNorm, hist);

        NewtonSolver newton(_opt.outer.newton);
        Vec x = x0;
        double mu = _opt.mu0;
        hist.status = SolveStatus::MaxOuterIterations;

        for (int k = 0; k < _opt.outer.maxOuter; ++k)
        {
            QuadraticPenaltyMerit merit(p, mu);
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
            Vec lambda(h.size());
            for (size_t i = 0; i < h.size(); ++i) lambda[i] = mu * h[i];

            OuterRecord rec = detail::makeRecord(p, merit, rep, lambda, hist, k);
            hist.totalNewton = rec.cumulativeNewton;
            hist.outer.push_back(rec);

            if (rec.hInf <= _opt.outer.feasTol && rec.kktNorm <= _opt.outer.optTol)
            {
                hist.status = SolveStatus::Converged;
                break;
            }
            if (mu * _opt.growth > _opt.muMax)
            {
                hist.status = SolveStatus::PenaltyLimit;
                break;
            }
            mu *= _opt.growth;
        }

        if (hist.message.empty()) hist.message = solveStatusName(hist.status);
        hist.elapsedMs = clock.ms();
    }
};

} // namespace pm
