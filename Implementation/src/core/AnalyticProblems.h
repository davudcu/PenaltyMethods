#pragma once
#include "IProblem.h"
#include <cmath>
#include <string>

// ============================================================
// Analytic test problems with known closed-form solutions.
// Used to validate both constrained solvers before the curve-
// fitting application, and to show that the conditioning trend
// is not specific to one problem.
// ============================================================
namespace pm
{

// ------------------------------------------------------------
// Projection of a point onto the unit circle
//   min (x1 - 2)^2 + (x2 - 1)^2   s.t.  x1^2 + x2^2 - 1 = 0
//   x* = (2,1)/sqrt(5),  lambda* = sqrt(5) - 1
// ------------------------------------------------------------
class CircleProjectionProblem : public IProblem, public IPlanarProblem
{
    static constexpr double cx = 2.0, cy = 1.0;
public:
    std::string name() const override { return "Circle projection (2-D)"; }
    std::string description() const override
    {
        return "min (x1-2)^2 + (x2-1)^2  s.t.  x1^2 + x2^2 = 1";
    }
    size_t dim() const override { return 2; }
    size_t numConstraints() const override { return 1; }

    double f(const Vec& x) const override
    {
        return (x[0] - cx) * (x[0] - cx) + (x[1] - cy) * (x[1] - cy);
    }
    void gradF(const Vec& x, Vec& g) const override
    {
        g = { 2.0 * (x[0] - cx), 2.0 * (x[1] - cy) };
    }
    void hessF(const Vec&, Mat& H) const override
    {
        resetMat(H, 2, 2);
        auto h = H.getManipulator();
        h(0, 0) = 2.0; h(1, 1) = 2.0;
    }
    void constraints(const Vec& x, Vec& h) const override
    {
        h = { x[0] * x[0] + x[1] * x[1] - 1.0 };
    }
    void jacobian(const Vec& x, Mat& J) const override
    {
        resetMat(J, 1, 2);
        auto j = J.getManipulator();
        j(0, 0) = 2.0 * x[0]; j(0, 1) = 2.0 * x[1];
    }
    void hessConstraint(size_t, const Vec&, Mat& H) const override
    {
        resetMat(H, 2, 2);
        auto h = H.getManipulator();
        h(0, 0) = 2.0; h(1, 1) = 2.0;
    }
    Vec initialPoint() const override { return { -1.0, 1.5 }; }

    bool referenceSolution(Vec& xs, Vec& ls) const override
    {
        const double r = std::sqrt(cx * cx + cy * cy);
        xs = { cx / r, cy / r };
        ls = { r - 1.0 };
        return true;
    }

    const IPlanarProblem* asPlanar() const override { return this; }
    void plotWindow(double& x0, double& x1, double& y0, double& y1) const override
    {
        x0 = -2.0; x1 = 2.8; y0 = -1.8; y1 = 2.2;
    }
};

// ------------------------------------------------------------
// Linear objective on a circle (Nocedal & Wright, Example 17.1)
//   min x1 + x2   s.t.  x1^2 + x2^2 - 2 = 0
//   x* = (-1,-1),  lambda* = 1/2
// The penalty function is non-convex for small mu, so the inner
// Newton solver has to use its Hessian modification here.
// ------------------------------------------------------------
class LinearOnCircleProblem : public IProblem, public IPlanarProblem
{
public:
    std::string name() const override { return "Linear objective on circle (2-D)"; }
    std::string description() const override
    {
        return "min x1 + x2  s.t.  x1^2 + x2^2 = 2   (Nocedal-Wright 17.1)";
    }
    size_t dim() const override { return 2; }
    size_t numConstraints() const override { return 1; }

    double f(const Vec& x) const override { return x[0] + x[1]; }
    void gradF(const Vec&, Vec& g) const override { g = { 1.0, 1.0 }; }
    void hessF(const Vec&, Mat& H) const override { resetMat(H, 2, 2); }

    void constraints(const Vec& x, Vec& h) const override
    {
        h = { x[0] * x[0] + x[1] * x[1] - 2.0 };
    }
    void jacobian(const Vec& x, Mat& J) const override
    {
        resetMat(J, 1, 2);
        auto j = J.getManipulator();
        j(0, 0) = 2.0 * x[0]; j(0, 1) = 2.0 * x[1];
    }
    void hessConstraint(size_t, const Vec&, Mat& H) const override
    {
        resetMat(H, 2, 2);
        auto h = H.getManipulator();
        h(0, 0) = 2.0; h(1, 1) = 2.0;
    }
    Vec initialPoint() const override { return { 1.0, 0.5 }; }

    bool referenceSolution(Vec& xs, Vec& ls) const override
    {
        xs = { -1.0, -1.0 };
        ls = { 0.5 };
        return true;
    }

    const IPlanarProblem* asPlanar() const override { return this; }
    void plotWindow(double& x0, double& x1, double& y0, double& y1) const override
    {
        x0 = -2.2; x1 = 2.2; y0 = -2.2; y1 = 2.2;
    }
};

// ------------------------------------------------------------
// Hock & Schittkowski problem 7
//   min ln(1 + x1^2) - x2   s.t.  (1 + x1^2)^2 + x2^2 - 4 = 0
//   x* = (0, sqrt 3),  lambda* = 1 / (2 sqrt 3)
// ------------------------------------------------------------
class HockSchittkowski7Problem : public IProblem, public IPlanarProblem
{
public:
    std::string name() const override { return "Hock-Schittkowski 7 (2-D)"; }
    std::string description() const override
    {
        return "min ln(1+x1^2) - x2  s.t.  (1+x1^2)^2 + x2^2 = 4";
    }
    size_t dim() const override { return 2; }
    size_t numConstraints() const override { return 1; }

    double f(const Vec& x) const override { return std::log(1.0 + x[0] * x[0]) - x[1]; }
    void gradF(const Vec& x, Vec& g) const override
    {
        g = { 2.0 * x[0] / (1.0 + x[0] * x[0]), -1.0 };
    }
    void hessF(const Vec& x, Mat& H) const override
    {
        resetMat(H, 2, 2);
        auto h = H.getManipulator();
        const double q = 1.0 + x[0] * x[0];
        h(0, 0) = (2.0 - 2.0 * x[0] * x[0]) / (q * q);
    }
    void constraints(const Vec& x, Vec& h) const override
    {
        const double q = 1.0 + x[0] * x[0];
        h = { q * q + x[1] * x[1] - 4.0 };
    }
    void jacobian(const Vec& x, Mat& J) const override
    {
        resetMat(J, 1, 2);
        auto j = J.getManipulator();
        j(0, 0) = 4.0 * x[0] * (1.0 + x[0] * x[0]);
        j(0, 1) = 2.0 * x[1];
    }
    void hessConstraint(size_t, const Vec& x, Mat& H) const override
    {
        resetMat(H, 2, 2);
        auto h = H.getManipulator();
        h(0, 0) = 4.0 + 12.0 * x[0] * x[0];
        h(1, 1) = 2.0;
    }
    Vec initialPoint() const override { return { 2.0, 2.0 }; }

    bool referenceSolution(Vec& xs, Vec& ls) const override
    {
        xs = { 0.0, std::sqrt(3.0) };
        ls = { 1.0 / (2.0 * std::sqrt(3.0)) };
        return true;
    }

    const IPlanarProblem* asPlanar() const override { return this; }
    void plotWindow(double& x0, double& x1, double& y0, double& y1) const override
    {
        x0 = -2.5; x1 = 2.5; y0 = -1.0; y1 = 3.0;
    }
};

// ------------------------------------------------------------
// Equality-constrained convex QP in R^4 with two linear constraints
//   min 1/2 x'Ax - b'x   s.t.  Cx = d
// Reference solution from the KKT system
//   [A  C'] [x     ]   [b]
//   [C  0 ] [lambda] = [d]
// ------------------------------------------------------------
class EqualityQPProblem : public IProblem
{
    static constexpr int n = 4, m = 2;
    double A[n][n] = { { 4, 1, 0, 0 }, { 1, 3, 1, 0 }, { 0, 1, 5, 1 }, { 0, 0, 1, 2 } };
    double b[n]    = { 1, -2, 3, 1 };
    double C[m][n] = { { 1, 1, 1, 1 }, { 1, -1, 2, 0 } };
    double d[m]    = { 1, 0.5 };

public:
    std::string name() const override { return "Equality-constrained QP (4-D)"; }
    std::string description() const override
    {
        return "min 1/2 x'Ax - b'x  s.t.  Cx = d   (n = 4, m = 2)";
    }
    size_t dim() const override { return n; }
    size_t numConstraints() const override { return m; }

    double f(const Vec& x) const override
    {
        double s = 0;
        for (int i = 0; i < n; ++i)
        {
            double ax = 0;
            for (int j = 0; j < n; ++j) ax += A[i][j] * x[j];
            s += 0.5 * x[i] * ax - b[i] * x[i];
        }
        return s;
    }
    void gradF(const Vec& x, Vec& g) const override
    {
        g.assign(n, 0.0);
        for (int i = 0; i < n; ++i)
        {
            for (int j = 0; j < n; ++j) g[i] += A[i][j] * x[j];
            g[i] -= b[i];
        }
    }
    void hessF(const Vec&, Mat& H) const override
    {
        resetMat(H, n, n);
        auto h = H.getManipulator();
        for (int i = 0; i < n; ++i)
            for (int j = 0; j < n; ++j) h(i, j) = A[i][j];
    }
    void constraints(const Vec& x, Vec& h) const override
    {
        h.assign(m, 0.0);
        for (int i = 0; i < m; ++i)
        {
            for (int j = 0; j < n; ++j) h[i] += C[i][j] * x[j];
            h[i] -= d[i];
        }
    }
    void jacobian(const Vec&, Mat& J) const override
    {
        resetMat(J, m, n);
        auto j = J.getManipulator();
        for (int i = 0; i < m; ++i)
            for (int k = 0; k < n; ++k) j(i, k) = C[i][k];
    }
    void hessConstraint(size_t, const Vec&, Mat& H) const override { resetMat(H, n, n); }

    Vec initialPoint() const override { return Vec(n, 0.0); }

    bool referenceSolution(Vec& xs, Vec& ls) const override
    {
        Mat K = zeroMat(n + m, n + m);
        auto k = K.getManipulator();
        for (int i = 0; i < n; ++i)
            for (int j = 0; j < n; ++j) k(i, j) = A[i][j];
        for (int i = 0; i < m; ++i)
            for (int j = 0; j < n; ++j) { k(n + i, j) = C[i][j]; k(j, n + i) = C[i][j]; }
        Vec rhs(n + m);
        for (int i = 0; i < n; ++i) rhs[i] = b[i];
        for (int i = 0; i < m; ++i) rhs[n + i] = d[i];
        Vec sol;
        if (!solveLinear(K, rhs, sol)) return false;
        xs.assign(sol.begin(), sol.begin() + n);
        ls.assign(sol.begin() + n, sol.end());
        return true;
    }
};

} // namespace pm
