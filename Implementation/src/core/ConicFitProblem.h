#pragma once
#include "IProblem.h"
#include <cmath>
#include <random>
#include <algorithm>
#include <string>
#include <vector>

// ============================================================
// Demonstration problem: conic (ellipse) fitting to scattered
// data under a normalisation constraint.
//
// A conic through a point (u, v) satisfies
//     d(u,v)' theta = 0,   d = (u^2, uv, v^2, u, v, 1)
// The algebraic least-squares fit is
//     min  f(theta) = theta' M theta,   M = (1/N) sum d_i d_i'
//     s.t. h(theta) = theta' theta - 1 = 0
// Without the constraint theta = 0 would be the trivial minimiser.
//
//   grad f = 2 M theta        Hess f = 2 M
//   grad h = 2 theta          Hess h = 2 I
//
// KKT: M theta = -lambda theta, so theta* is the eigenvector of M
// for its smallest eigenvalue and lambda* = -sigma_min.  The
// reference solution is computed with the Jacobi eigen-solver.
//
// Data are centred and scaled (Hartley normalisation) before the
// design vectors are built, which keeps M well scaled.
// Noise is generated with std::mt19937 + Box-Muller, which gives
// identical data on every platform for the same seed.
// ============================================================
namespace pm
{

class ConicFitProblem : public IProblem, public ICurveFitProblem
{
    std::vector<Point> _data;
    double _mx = 0, _my = 0, _scale = 1;   // normalisation
    double _M[6][6] = {};
    ProblemConfig _cfg;

    static void design(double u, double v, double d[6])
    {
        d[0] = u * u; d[1] = u * v; d[2] = v * v; d[3] = u; d[4] = v; d[5] = 1.0;
    }

    void generate()
    {
        std::mt19937 gen(_cfg.seed);
        auto uniform = [&gen]() { return (static_cast<double>(gen()) + 0.5) / 4294967296.0; };
        auto gauss = [&uniform]()
        {
            const double u1 = uniform(), u2 = uniform();
            return std::sqrt(-2.0 * std::log(u1)) * std::cos(6.283185307179586 * u2);
        };

        // true ellipse: centre (1.5, -0.5), semi-axes 3 and 1.5, rotated 30 degrees
        const double cx = 1.5, cy = -0.5, a = 3.0, b = 1.5;
        const double phi = 30.0 * 3.141592653589793 / 180.0;
        const double c = std::cos(phi), s = std::sin(phi);

        const int N = (std::max)(6, _cfg.nPoints);
        _data.clear();
        for (int i = 0; i < N; ++i)
        {
            const double t = 6.283185307179586 * (i + 0.5 * uniform()) / N;
            const double ex = a * std::cos(t), ey = b * std::sin(t);
            Point p;
            p.x = cx + c * ex - s * ey + _cfg.noise * gauss();
            p.y = cy + s * ex + c * ey + _cfg.noise * gauss();
            _data.push_back(p);
        }

        // Hartley normalisation: zero mean, mean distance sqrt(2)
        _mx = _my = 0;
        for (auto& p : _data) { _mx += p.x; _my += p.y; }
        _mx /= N; _my /= N;
        double meanDist = 0;
        for (auto& p : _data) meanDist += std::hypot(p.x - _mx, p.y - _my);
        meanDist /= N;
        _scale = meanDist > 0 ? meanDist / std::sqrt(2.0) : 1.0;

        for (auto& row : _M) for (double& v : row) v = 0;
        for (auto& p : _data)
        {
            double d[6];
            design((p.x - _mx) / _scale, (p.y - _my) / _scale, d);
            for (int i = 0; i < 6; ++i)
                for (int j = 0; j < 6; ++j) _M[i][j] += d[i] * d[j] / N;
        }
    }

public:
    explicit ConicFitProblem(const ProblemConfig& cfg)
    : _cfg(cfg)
    {
        generate();
    }

    std::string name() const override { return "Conic curve fit (6-D)"; }
    std::string description() const override
    {
        return "min theta'M theta  s.t.  |theta|^2 = 1   (algebraic ellipse fit)";
    }
    size_t dim() const override { return 6; }
    size_t numConstraints() const override { return 1; }

    double f(const Vec& x) const override
    {
        double s = 0;
        for (int i = 0; i < 6; ++i)
            for (int j = 0; j < 6; ++j) s += x[i] * _M[i][j] * x[j];
        return s;
    }
    void gradF(const Vec& x, Vec& g) const override
    {
        g.assign(6, 0.0);
        for (int i = 0; i < 6; ++i)
            for (int j = 0; j < 6; ++j) g[i] += 2.0 * _M[i][j] * x[j];
    }
    void hessF(const Vec&, Mat& H) const override
    {
        resetMat(H, 6, 6);
        auto h = H.getManipulator();
        for (int i = 0; i < 6; ++i)
            for (int j = 0; j < 6; ++j) h(i, j) = 2.0 * _M[i][j];
    }
    void constraints(const Vec& x, Vec& h) const override
    {
        h = { dot(x, x) - 1.0 };
    }
    void jacobian(const Vec& x, Mat& J) const override
    {
        resetMat(J, 1, 6);
        auto j = J.getManipulator();
        for (int i = 0; i < 6; ++i) j(0, i) = 2.0 * x[i];
    }
    void hessConstraint(size_t, const Vec&, Mat& H) const override
    {
        resetMat(H, 6, 6);
        auto h = H.getManipulator();
        for (int i = 0; i < 6; ++i) h(i, i) = 2.0;
    }

    // unit circle in normalised coordinates: u^2 + v^2 - 1 = 0
    Vec initialPoint() const override
    {
        const double k = 1.0 / std::sqrt(3.0);
        return { k, 0.0, k, 0.0, 0.0, -k };
    }

    bool referenceSolution(Vec& xs, Vec& ls) const override
    {
        Mat M = zeroMat(6, 6);
        auto m = M.getManipulator();
        for (int i = 0; i < 6; ++i)
            for (int j = 0; j < 6; ++j) m(i, j) = _M[i][j];
        std::vector<double> V;
        Vec ev = symmetricEigen(M, &V);
        size_t k = 0;
        for (size_t i = 1; i < ev.size(); ++i) if (ev[i] < ev[k]) k = i;
        xs.assign(6, 0.0);
        for (size_t i = 0; i < 6; ++i) xs[i] = V[k * 6 + i];
        const double nrm = norm2(xs);
        for (double& v : xs) v /= nrm;
        ls = { -ev[k] };
        return true;
    }

    // theta and -theta describe the same conic
    double solutionError(const Vec& x, const Vec& xs) const override
    {
        Vec neg(xs.size());
        for (size_t i = 0; i < xs.size(); ++i) neg[i] = -xs[i];
        return (std::min)(distance2(x, xs), distance2(x, neg));
    }

    const ICurveFitProblem* asCurveFit() const override { return this; }

    // ICurveFitProblem
    const std::vector<Point>& dataPoints() const override { return _data; }

    double curveValue(const Vec& theta, double px, double py) const override
    {
        double d[6];
        design((px - _mx) / _scale, (py - _my) / _scale, d);
        double s = 0;
        for (int i = 0; i < 6; ++i) s += d[i] * theta[i];
        return s;
    }

    void plotWindow(double& x0, double& x1, double& y0, double& y1) const override
    {
        x0 = y0 = kInf; x1 = y1 = -kInf;
        for (auto& p : _data)
        {
            x0 = (std::min)(x0, p.x); x1 = (std::max)(x1, p.x);
            y0 = (std::min)(y0, p.y); y1 = (std::max)(y1, p.y);
        }
        const double mx = 0.15 * (x1 - x0), my = 0.15 * (y1 - y0);
        x0 -= mx; x1 += mx; y0 -= my; y1 += my;
    }
};

} // namespace pm
