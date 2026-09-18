#pragma once
#include "LinAlg.h"
#include <string>
#include <vector>

// ============================================================
// Equality-constrained problem interface
//
//      minimise f(x)   subject to   h_i(x) = 0,  i = 1..m
//
// A problem supplies first and second derivatives of f and of
// every constraint.  Solvers depend only on this interface, so a
// new test problem never requires touching solver code (OCP/DIP).
//
// Optional capabilities are separate small interfaces (ISP):
//   IPlanarProblem: 2-D problems whose landscape can be drawn
//   ICurveFitProblem: problems that fit a curve to scattered data
// ============================================================
namespace pm
{

class IPlanarProblem;
class ICurveFitProblem;

class IProblem
{
public:
    virtual ~IProblem() = default;

    virtual std::string name() const = 0;
    virtual std::string description() const = 0;

    virtual size_t dim() const = 0;              // n
    virtual size_t numConstraints() const = 0;   // m

    // objective
    virtual double f(const Vec& x) const = 0;
    virtual void   gradF(const Vec& x, Vec& g) const = 0;         // g in R^n
    virtual void   hessF(const Vec& x, Mat& H) const = 0;         // H: n x n (overwritten)

    // constraints
    virtual void   constraints(const Vec& x, Vec& h) const = 0;   // h in R^m
    virtual void   jacobian(const Vec& x, Mat& J) const = 0;      // J: m x n (overwritten)
    virtual void   hessConstraint(size_t i, const Vec& x, Mat& H) const = 0; // n x n

    virtual Vec    initialPoint() const = 0;

    // Known solution (closed form or computed independently).
    virtual bool referenceSolution(Vec& /*xStar*/, Vec& /*lambdaStar*/) const { return false; }

    // Distance to the reference solution.  Problems with symmetric
    // solution sets (e.g. x and -x) override this.
    virtual double solutionError(const Vec& x, const Vec& xStar) const { return distance2(x, xStar); }

    // optional capabilities
    virtual const IPlanarProblem*   asPlanar()   const { return nullptr; }
    virtual const ICurveFitProblem* asCurveFit() const { return nullptr; }
};

// A 2-D problem: provides a sensible window for landscape plots.
class IPlanarProblem
{
public:
    virtual ~IPlanarProblem() = default;
    virtual void plotWindow(double& xMin, double& xMax, double& yMin, double& yMax) const = 0;
};

// A curve-fitting problem: data points in the plane and an
// implicit curve c(theta; px, py) = 0 described by the parameters.
class ICurveFitProblem
{
public:
    struct Point { double x, y; };
    virtual ~ICurveFitProblem() = default;
    virtual const std::vector<Point>& dataPoints() const = 0;
    virtual double curveValue(const Vec& theta, double px, double py) const = 0;
    virtual void   plotWindow(double& xMin, double& xMax, double& yMin, double& yMax) const = 0;
};

// Problem-construction parameters coming from the GUI.
struct ProblemConfig
{
    int    nPoints = 40;      // curve fitting only
    double noise   = 0.05;    // curve fitting only
    unsigned seed  = 20154;   // curve fitting only
};

} // namespace pm
