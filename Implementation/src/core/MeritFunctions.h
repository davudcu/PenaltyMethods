#pragma once
#include "IProblem.h"

// ============================================================
// Unconstrained merit functions minimised by the inner Newton
// solver.  Both penalty and augmented Lagrangian are expressed
// through one formula; the quadratic penalty is the special case
// lambda = 0.
//
//   L_A(x; lambda, mu) = f(x) + sum lambda_i h_i(x) + mu/2 sum h_i(x)^2
//
//   grad L_A = grad f + J' (lambda + mu h)
//   Hess L_A = Hess f + mu J'J + sum_i (lambda_i + mu h_i) Hess h_i
//
//   Q(x; mu) = L_A(x; 0, mu) = f(x) + mu/2 sum h_i(x)^2
// ============================================================
namespace pm
{

class ITwiceDifferentiable
{
public:
    virtual ~ITwiceDifferentiable() = default;
    virtual size_t dim() const = 0;
    virtual double value(const Vec& x) const = 0;
    virtual void   gradient(const Vec& x, Vec& g) const = 0;
    virtual void   hessian(const Vec& x, Mat& H) const = 0;
};

class AugmentedLagrangianMerit : public ITwiceDifferentiable
{
protected:
    const IProblem& _p;
    Vec    _lambda;
    double _mu;

public:
    AugmentedLagrangianMerit(const IProblem& p, const Vec& lambda, double mu)
    : _p(p), _lambda(lambda), _mu(mu)
    {
        if (_lambda.size() != _p.numConstraints()) _lambda.assign(_p.numConstraints(), 0.0);
    }

    size_t dim() const override { return _p.dim(); }
    double mu() const { return _mu; }
    const Vec& lambda() const { return _lambda; }

    double value(const Vec& x) const override
    {
        Vec h;
        _p.constraints(x, h);
        double v = _p.f(x);
        for (size_t i = 0; i < h.size(); ++i) v += _lambda[i] * h[i] + 0.5 * _mu * h[i] * h[i];
        return v;
    }

    void gradient(const Vec& x, Vec& g) const override
    {
        Vec h, w, jt;
        Mat J;
        _p.gradF(x, g);
        _p.constraints(x, h);
        _p.jacobian(x, J);
        w.resize(h.size());
        for (size_t i = 0; i < h.size(); ++i) w[i] = _lambda[i] + _mu * h[i];
        mulTransVec(J, w, jt);
        for (size_t k = 0; k < g.size(); ++k) g[k] += jt[k];
    }

    void hessian(const Vec& x, Mat& H) const override
    {
        Vec h;
        Mat J, Hi;
        _p.hessF(x, H);
        _p.constraints(x, h);
        _p.jacobian(x, J);
        addScaledGram(H, J, _mu);
        for (size_t i = 0; i < h.size(); ++i)
        {
            const double w = _lambda[i] + _mu * h[i];
            if (w == 0.0) continue;
            _p.hessConstraint(i, x, Hi);
            addScaled(H, Hi, w);
        }
    }
};

class QuadraticPenaltyMerit : public AugmentedLagrangianMerit
{
public:
    QuadraticPenaltyMerit(const IProblem& p, double mu)
    : AugmentedLagrangianMerit(p, Vec(p.numConstraints(), 0.0), mu)
    {}
};

} // namespace pm
