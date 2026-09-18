// PenaltyMethodsTests: console verification of the numerical core.
//
//   1. linear-algebra building blocks (natID solve/invert, Cholesky, Jacobi)
//   2. analytic gradients / Hessians / Jacobians vs central finite differences
//   3. both constrained solvers vs known solutions
//   4. conditioning trend: penalty ~ O(mu), augmented Lagrangian bounded
//
// Exit code 0 when every check passes.

#include <mu/Application.h>
#include "../src/core/ProblemRegistry.h"
#include "../src/core/QuadraticPenaltySolver.h"
#include "../src/core/AugmentedLagrangianSolver.h"
#include "../src/core/ComparisonInsights.h"
#include <cstdio>
#include <random>
#include <algorithm>
#include <cmath>
#include <vector>

using namespace pm;

static int g_failures = 0;
static int g_checks = 0;

static void check(bool ok, const char* what, double value = 0, double limit = 0)
{
    ++g_checks;
    if (!ok) ++g_failures;
    std::printf("  [%s] %-58s %12.3e  (limit %.1e)\n", ok ? " OK " : "FAIL", what, value, limit);
}

// ------------------------------------------------------------
static void testLinearAlgebra()
{
    std::printf("\n== Linear algebra ==\n");

    Mat A = zeroMat(3, 3);
    {
        auto a = A.getManipulator();
        a(0, 0) = 4; a(0, 1) = 1; a(0, 2) = 2;
        a(1, 0) = 1; a(1, 1) = 5; a(1, 2) = 3;
        a(2, 0) = 2; a(2, 1) = 3; a(2, 2) = 6;
    }
    Vec xTrue = { 1.0, -2.0, 3.0 }, b, x;
    mulVec(A, xTrue, b);
    bool ok = solveLinear(A, b, x);
    check(ok && distance2(x, xTrue) < 1e-12, "natID solve() 3x3", ok ? distance2(x, xTrue) : 1.0, 1e-12);

    // A must be unchanged by solveLinear
    Vec b2;
    mulVec(A, xTrue, b2);
    check(distance2(b, b2) == 0.0, "solveLinear leaves A untouched", distance2(b, b2), 0);

    Mat D = zeroMat(3, 3);
    { auto d = D.getManipulator(); d(0, 0) = 1; d(1, 1) = 10; d(2, 2) = 100; }
    double c = conditionNumber(D, CondNorm::Inf);
    check(std::abs(c - 100.0) < 1e-9, "cond_inf(diag(1,10,100)) = 100", c, 100);

    // Hilbert 3x3: cond_inf = 748
    Mat Hb = zeroMat(3, 3);
    { auto h = Hb.getManipulator(); for (int i = 0; i < 3; ++i) for (int j = 0; j < 3; ++j) h(i, j) = 1.0 / (i + j + 1); }
    c = conditionNumber(Hb, CondNorm::Inf);
    check(std::abs(c - 748.0) < 1e-6, "cond_inf(Hilbert3) = 748", c, 748);
    c = conditionNumber(Hb, CondNorm::One);
    check(std::abs(c - 748.0) < 1e-6, "cond_1(Hilbert3) = 748", c, 748);

    Mat S = zeroMat(2, 2);
    { auto s = S.getManipulator(); s(0, 0) = 2; s(0, 1) = 1; s(1, 0) = 1; s(1, 1) = 2; }
    c = conditionNumber(S, CondNorm::Two);
    check(std::abs(c - 3.0) < 1e-12, "cond_2([[2,1],[1,2]]) = 3", c, 3);

    Vec ev = symmetricEigen(Hb);
    double lo = (std::min)({ ev[0], ev[1], ev[2] }), hi = (std::max)({ ev[0], ev[1], ev[2] });
    check(std::abs(hi / lo - 524.0567775860627) < 1e-4, "Jacobi eigen Hilbert3 kappa_2 = 524.0568", hi / lo, 524.06);

    check(isPositiveDefinite(S), "Cholesky: [[2,1],[1,2]] is PD", 0, 0);
    Mat N = zeroMat(2, 2);
    { auto s = N.getManipulator(); s(0, 0) = 1; s(0, 1) = 2; s(1, 0) = 2; s(1, 1) = 1; }
    check(!isPositiveDefinite(N), "Cholesky: [[1,2],[2,1]] is not PD", 0, 0);
}

// ------------------------------------------------------------
template <typename F>
static double fdGradientError(size_t n, const Vec& x, F fval, const Vec& g)
{
    double err = 0;
    for (size_t i = 0; i < n; ++i)
    {
        const double step = 1e-6 * (std::max)(1.0, std::abs(x[i]));
        Vec xp = x, xm = x;
        xp[i] += step; xm[i] -= step;
        const double fd = (fval(xp) - fval(xm)) / (2 * step);
        err = (std::max)(err, std::abs(fd - g[i]) / (std::max)(1.0, std::abs(g[i])));
    }
    return err;
}

// error of a matrix-valued derivative: column i of M vs central difference of vector fn
template <typename F>
static double fdMatrixError(const Vec& x, F vecFn, const Mat& M, bool rowsAreOutputs)
{
    const size_t n = x.size();
    auto m = M.getManipulator();
    double err = 0;
    for (size_t i = 0; i < n; ++i)
    {
        const double step = 1e-6 * (std::max)(1.0, std::abs(x[i]));
        Vec xp = x, xm = x, vp, vm;
        xp[i] += step; xm[i] -= step;
        vecFn(xp, vp); vecFn(xm, vm);
        for (size_t r = 0; r < vp.size(); ++r)
        {
            const double fd = (vp[r] - vm[r]) / (2 * step);
            const double an = rowsAreOutputs ? m((td::UINT4) r, (td::UINT4) i) : m((td::UINT4) i, (td::UINT4) r);
            err = (std::max)(err, std::abs(fd - an) / (std::max)(1.0, std::abs(an)));
        }
    }
    return err;
}

static void testDerivatives()
{
    std::printf("\n== Derivatives vs finite differences ==\n");
    const double tol = 1e-6;
    std::mt19937 gen(7);
    std::uniform_real_distribution<double> U(-1.0, 1.0);

    for (auto& entry : problemRegistry())
    {
        auto prob = entry.create(ProblemConfig());
        const IProblem& p = *prob;
        const size_t n = p.dim(), m = p.numConstraints();
        std::printf(" %s\n", p.name().c_str());

        for (int trial = 0; trial < 3; ++trial)
        {
            Vec x = p.initialPoint();
            for (double& v : x) v += 0.7 * U(gen);

            Vec g;
            p.gradF(x, g);
            double e = fdGradientError(n, x, [&](const Vec& z) { return p.f(z); }, g);
            char buf[128];
            std::snprintf(buf, sizeof buf, "grad f          (trial %d)", trial);
            check(e < tol, buf, e, tol);

            Mat H;
            p.hessF(x, H);
            e = fdMatrixError(x, [&](const Vec& z, Vec& out) { p.gradF(z, out); }, H, true);
            std::snprintf(buf, sizeof buf, "Hess f          (trial %d)", trial);
            check(e < tol, buf, e, tol);

            Mat J;
            p.jacobian(x, J);
            e = fdMatrixError(x, [&](const Vec& z, Vec& out) { p.constraints(z, out); }, J, true);
            std::snprintf(buf, sizeof buf, "Jacobian h      (trial %d)", trial);
            check(e < tol, buf, e, tol);

            for (size_t i = 0; i < m; ++i)
            {
                Mat Hi;
                p.hessConstraint(i, x, Hi);
                e = fdMatrixError(x, [&](const Vec& z, Vec& out)
                {
                    Mat Jz;
                    p.jacobian(z, Jz);
                    auto jz = Jz.getManipulator();
                    out.assign(n, 0.0);
                    for (size_t k = 0; k < n; ++k) out[k] = jz((td::UINT4) i, (td::UINT4) k);
                }, Hi, true);
                std::snprintf(buf, sizeof buf, "Hess h_%zu        (trial %d)", i, trial);
                check(e < tol, buf, e, tol);
            }

            // merit function (augmented Lagrangian with random multipliers)
            Vec lam(m);
            for (double& v : lam) v = 2.0 * U(gen);
            AugmentedLagrangianMerit merit(p, lam, 3.7);
            Vec gm;
            merit.gradient(x, gm);
            e = fdGradientError(n, x, [&](const Vec& z) { return merit.value(z); }, gm);
            std::snprintf(buf, sizeof buf, "grad L_A        (trial %d)", trial);
            check(e < tol, buf, e, tol);

            Mat Hm;
            merit.hessian(x, Hm);
            e = fdMatrixError(x, [&](const Vec& z, Vec& out) { merit.gradient(z, out); }, Hm, true);
            std::snprintf(buf, sizeof buf, "Hess L_A        (trial %d)", trial);
            check(e < tol, buf, e, tol);
        }
    }
}

// ------------------------------------------------------------
static double slopeLogLog(const std::vector<double>& xs, const std::vector<double>& ys)
{
    const size_t n = xs.size();
    if (n < 2) return 0;
    double sx = 0, sy = 0, sxx = 0, sxy = 0;
    for (size_t i = 0; i < n; ++i)
    {
        const double lx = std::log10(xs[i]), ly = std::log10(ys[i]);
        sx += lx; sy += ly; sxx += lx * lx; sxy += lx * ly;
    }
    return (n * sxy - sx * sy) / (n * sxx - sx * sx);
}

static void printHistory(const SolveHistory& h)
{
    std::printf("   %s: %s, %d outer, %d Newton, %.2f ms\n", h.method.c_str(), h.message.c_str(),
                (int) h.outer.size(), h.totalNewton, h.elapsedMs);
    std::printf("     k        mu        ||h||inf     cond        Newton  status          |x-x*|     ||grad||   shift      linRes\n");
    for (auto& r : h.outer)
        std::printf("    %2d  %10.2e  %10.3e  %10.3e   %4d   %-14s  %10.3e %10.2e %10.2e %10.2e\n",
                    r.k, r.mu, r.hInf, r.cond, r.newtonIters, r.newtonStatus.c_str(), r.xError,
                    r.newtonGrad, r.hessShift, r.linResidual);
}

static void testSolvers()
{
    std::printf("\n== Solvers vs reference solutions ==\n");

    for (auto& entry : problemRegistry())
    {
        auto prob = entry.create(ProblemConfig());
        const IProblem& p = *prob;
        std::printf("\n %s\n", p.name().c_str());

        PenaltyOptions po;
        ALOptions ao;
        QuadraticPenaltySolver penalty(po);
        AugmentedLagrangianSolver al(ao);

        SolveHistory hp, ha;
        penalty.solve(p, p.initialPoint(), hp);
        al.solve(p, p.initialPoint(), ha);
        printHistory(hp);
        printHistory(ha);

        check(hp.hasReference && ha.hasReference, "reference solution available", 0, 0);
        if (!hp.hasReference) continue;

        check(hp.last().xError < 1e-5, "penalty  |x - x*|", hp.last().xError, 1e-5);
        check(ha.last().xError < 1e-6, "aug. Lagrangian |x - x*|", ha.last().xError, 1e-6);
        check(ha.last().lambdaError < 1e-5, "aug. Lagrangian |lambda - lambda*|", ha.last().lambdaError, 1e-5);
        check(ha.status == SolveStatus::Converged, "aug. Lagrangian converged", 0, 0);

        // conditioning: penalty cond grows ~ mu (slope ~1 on log-log for large mu)
        std::vector<double> mus, conds;
        for (auto& r : hp.outer)
            if (r.mu >= 1e3 && std::isfinite(r.cond)) { mus.push_back(r.mu); conds.push_back(r.cond); }
        const double slope = slopeLogLog(mus, conds);
        check(slope > 0.8 && slope < 1.2, "penalty d log(cond) / d log(mu) ~ 1", slope, 1.0);

        double alMin = kInf, alMax = 0;
        for (auto& r : ha.outer) { alMin = (std::min)(alMin, r.cond); alMax = (std::max)(alMax, r.cond); }
        check(alMax / alMin < 10.0, "aug. Lagrangian cond max/min bounded", alMax / alMin, 10.0);
        check(hp.maxCond() > 1e4 * alMax, "penalty max cond >> AL max cond", hp.maxCond() / alMax, 1e4);
    }
}

// ------------------------------------------------------------
static void testInsights()
{
    std::printf("\n== Summary page: scorecard and findings ==\n");
    RunSettings settings;
    RunResult all = ComparisonRunner::run(settings, true, nullptr);
    check(all.runs.size() == problemRegistry().size(), "run all problems solves every registered problem",
          (double) all.runs.size(), (double) problemRegistry().size());

    for (auto& mc : all.runs)
    {
        const auto rows = ComparisonInsights::scorecard(mc);
        const auto notes = ComparisonInsights::findings(mc);
        std::printf(" %s\n", mc.problem->name().c_str());
        for (auto& n : notes) std::printf("    - %s\n", n.c_str());

        Better condWinner = Better::None;
        for (auto& r : rows) if (r.metric == "Largest condition number") condWinner = r.better;
        check(rows.size() >= 8, "scorecard has all rows", (double) rows.size(), 8);
        check(condWinner == Better::AugLag, "scorecard: aug. Lagrangian wins on conditioning", 0, 0);
        check(notes.size() >= 3, "findings generated", (double) notes.size(), 3);
    }
}

int main(int argc, const char* argv[])
{
    mu::Application app(argc, argv);

    testLinearAlgebra();
    testDerivatives();
    testSolvers();
    testInsights();

    std::printf("\n%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
