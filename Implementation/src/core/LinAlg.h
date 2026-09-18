#pragma once
#include <dense/Matrix.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

// ============================================================
// LinAlg: small dense linear-algebra toolkit for the solvers.
//
// Storage uses natID dense::DblMatrix (column-major, accessed
// through manipulators).  Vectors are plain std::vector<double>.
//
// What comes from natID:
//   * DblMatrix::solve(): general in-place linear solve (LU)
//   * DblMatrix::invert(): explicit inverse for cond_inf / cond_1
//
// What is written here from scratch:
//   * Cholesky factorisation (positive-definiteness test used
//     by the modified Newton method; natID dense has no Cholesky)
//   * Jacobi eigenvalue iteration for symmetric matrices, used
//     for the spectral condition number kappa_2 (natID eig() and
//     svd() are not available in this SDK version)
//   * matrix norms, products and the condition-number estimates
// ============================================================
namespace pm
{

using Vec = std::vector<double>;
using Mat = dense::DblMatrix;

constexpr double kInf = std::numeric_limits<double>::infinity();

enum class CondNorm : int { Inf = 0, One = 1, Two = 2 };

// ---------- vectors -----------------------------------------
inline double dot(const Vec& a, const Vec& b)
{
    double s = 0;
    for (size_t i = 0; i < a.size(); ++i) s += a[i] * b[i];
    return s;
}

inline double norm2(const Vec& a) { return std::sqrt(dot(a, a)); }

inline double normInf(const Vec& a)
{
    double m = 0;
    for (double v : a) m = (std::max)(m, std::abs(v));
    return m;
}

inline double distance2(const Vec& a, const Vec& b)
{
    double s = 0;
    for (size_t i = 0; i < a.size(); ++i) { double d = a[i] - b[i]; s += d * d; }
    return std::sqrt(s);
}

inline bool allFinite(const Vec& a)
{
    for (double v : a) if (!std::isfinite(v)) return false;
    return true;
}

// ---------- matrices ----------------------------------------
inline td::UINT4 rows(const Mat& A) { return A.getNoOfRows(); }
inline td::UINT4 cols(const Mat& A) { return A.getNoOfCols(); }

inline Mat zeroMat(size_t r, size_t c)
{
    Mat A((td::UINT4) r, (td::UINT4) c);
    A.zeros();
    return A;
}

// Guarantees an n x n zero matrix that does not share storage
// with any other DblMatrix (DblMatrix copies are reference counted).
inline void resetMat(Mat& A, size_t r, size_t c)
{
    A = zeroMat(r, c);
}

// y = A x
inline void mulVec(const Mat& A, const Vec& x, Vec& y)
{
    auto a = A.getManipulator();
    const td::UINT4 m = rows(A), n = cols(A);
    y.assign(m, 0.0);
    for (td::UINT4 i = 0; i < m; ++i)
    {
        double s = 0;
        for (td::UINT4 j = 0; j < n; ++j) s += a(i, j) * x[j];
        y[i] = s;
    }
}

// y = A^T x
inline void mulTransVec(const Mat& A, const Vec& x, Vec& y)
{
    auto a = A.getManipulator();
    const td::UINT4 m = rows(A), n = cols(A);
    y.assign(n, 0.0);
    for (td::UINT4 j = 0; j < n; ++j)
    {
        double s = 0;
        for (td::UINT4 i = 0; i < m; ++i) s += a(i, j) * x[i];
        y[j] = s;
    }
}

// H += s * A^T A     (A is m x n, H is n x n)
inline void addScaledGram(Mat& H, const Mat& A, double s)
{
    auto h = H.getManipulator();
    auto a = A.getManipulator();
    const td::UINT4 m = rows(A), n = cols(A);
    for (td::UINT4 p = 0; p < n; ++p)
        for (td::UINT4 q = 0; q < n; ++q)
        {
            double acc = 0;
            for (td::UINT4 i = 0; i < m; ++i) acc += a(i, p) * a(i, q);
            h(p, q) += s * acc;
        }
}

// H += s * B   (same shape)
inline void addScaled(Mat& H, const Mat& B, double s)
{
    auto h = H.getManipulator();
    auto b = B.getManipulator();
    for (td::UINT4 i = 0; i < rows(H); ++i)
        for (td::UINT4 j = 0; j < cols(H); ++j)
            h(i, j) += s * b(i, j);
}

inline void addDiagonal(Mat& H, double tau)
{
    auto h = H.getManipulator();
    for (td::UINT4 i = 0; i < rows(H); ++i) h(i, i) += tau;
}

// max absolute row sum
inline double matNormInf(const Mat& A)
{
    auto a = A.getManipulator();
    double best = 0;
    for (td::UINT4 i = 0; i < rows(A); ++i)
    {
        double s = 0;
        for (td::UINT4 j = 0; j < cols(A); ++j) s += std::abs(a(i, j));
        best = (std::max)(best, s);
    }
    return best;
}

// max absolute column sum
inline double matNorm1(const Mat& A)
{
    auto a = A.getManipulator();
    double best = 0;
    for (td::UINT4 j = 0; j < cols(A); ++j)
    {
        double s = 0;
        for (td::UINT4 i = 0; i < rows(A); ++i) s += std::abs(a(i, j));
        best = (std::max)(best, s);
    }
    return best;
}

inline bool matAllFinite(const Mat& A)
{
    auto a = A.getManipulator();
    for (td::UINT4 i = 0; i < rows(A); ++i)
        for (td::UINT4 j = 0; j < cols(A); ++j)
            if (!std::isfinite(a(i, j))) return false;
    return true;
}

// ---------- linear solve (natID) ----------------------------
// Solves A x = b.  A is left untouched (solve() factorises in place,
// so a private copy is made first).
inline bool solveLinear(const Mat& A, const Vec& b, Vec& x)
{
    const td::UINT4 n = rows(A);
    Mat M = A.makeCopy();
    Mat B(n, 1);
    auto bb = B.getFirstColumnManipulator();
    for (td::UINT4 i = 0; i < n; ++i) bb(i) = b[i];

    if (!M.solve(B)) return false;

    auto xx = B.getFirstColumnManipulator();
    x.assign(n, 0.0);
    for (td::UINT4 i = 0; i < n; ++i) x[i] = xx(i);
    return allFinite(x);
}

// ---------- Cholesky (from scratch) -------------------------
// Returns true when A (symmetric) is numerically positive definite.
inline bool isPositiveDefinite(const Mat& A)
{
    const size_t n = rows(A);
    auto a = A.getManipulator();
    std::vector<double> L(n * n, 0.0);
    for (size_t j = 0; j < n; ++j)
    {
        double d = a((td::UINT4) j, (td::UINT4) j);
        for (size_t k = 0; k < j; ++k) d -= L[j * n + k] * L[j * n + k];
        if (!(d > 0.0) || !std::isfinite(d)) return false;
        const double ljj = std::sqrt(d);
        L[j * n + j] = ljj;
        for (size_t i = j + 1; i < n; ++i)
        {
            double s = a((td::UINT4) i, (td::UINT4) j);
            for (size_t k = 0; k < j; ++k) s -= L[i * n + k] * L[j * n + k];
            L[i * n + j] = s / ljj;
        }
    }
    return true;
}

// ---------- symmetric eigenvalues (Jacobi, from scratch) ----
// Cyclic Jacobi rotations.  Returns eigenvalues (unsorted);
// if pV != nullptr, eigenvectors are returned column-wise in *pV
// as a flat n*n column-major array.
inline Vec symmetricEigen(const Mat& A, std::vector<double>* pV = nullptr)
{
    const size_t n = rows(A);
    auto a = A.getManipulator();
    std::vector<double> S(n * n);
    for (size_t i = 0; i < n; ++i)
        for (size_t j = 0; j < n; ++j)
            S[i * n + j] = 0.5 * (a((td::UINT4) i, (td::UINT4) j) + a((td::UINT4) j, (td::UINT4) i));

    std::vector<double> V(n * n, 0.0);
    for (size_t i = 0; i < n; ++i) V[i * n + i] = 1.0;

    for (int sweep = 0; sweep < 100; ++sweep)
    {
        double off = 0, total = 0;
        for (size_t i = 0; i < n; ++i)
            for (size_t j = 0; j < n; ++j)
            {
                total += S[i * n + j] * S[i * n + j];
                if (i != j) off += S[i * n + j] * S[i * n + j];
            }
        if (off <= 1e-30 * total || off == 0.0) break;

        for (size_t p = 0; p < n; ++p)
            for (size_t q = p + 1; q < n; ++q)
            {
                const double apq = S[p * n + q];
                if (apq == 0.0) continue;
                const double theta = (S[q * n + q] - S[p * n + p]) / (2.0 * apq);
                const double t = (theta >= 0 ? 1.0 : -1.0) / (std::abs(theta) + std::sqrt(theta * theta + 1.0));
                const double c = 1.0 / std::sqrt(t * t + 1.0);
                const double s = t * c;
                for (size_t k = 0; k < n; ++k)
                {
                    const double skp = S[k * n + p], skq = S[k * n + q];
                    S[k * n + p] = c * skp - s * skq;
                    S[k * n + q] = s * skp + c * skq;
                }
                for (size_t k = 0; k < n; ++k)
                {
                    const double spk = S[p * n + k], sqk = S[q * n + k];
                    S[p * n + k] = c * spk - s * sqk;
                    S[q * n + k] = s * spk + c * sqk;
                }
                for (size_t k = 0; k < n; ++k)
                {
                    const double vkp = V[k * n + p], vkq = V[k * n + q];
                    V[k * n + p] = c * vkp - s * vkq;
                    V[k * n + q] = s * vkp + c * vkq;
                }
            }
    }

    Vec eig(n);
    for (size_t i = 0; i < n; ++i) eig[i] = S[i * n + i];
    if (pV)
    {
        // convert row-major V[k*n+p] (row k, column p) to column-major
        pV->assign(n * n, 0.0);
        for (size_t col = 0; col < n; ++col)
            for (size_t row = 0; row < n; ++row)
                (*pV)[col * n + row] = V[row * n + col];
    }
    return eig;
}

// ---------- condition number --------------------------------
// cond_p(A) = ||A||_p * ||A^-1||_p   for p = inf, 1   (natID invert())
// cond_2(A) = max|lambda| / min|lambda| for symmetric A (Jacobi)
inline double conditionNumber(const Mat& A, CondNorm norm)
{
    if (!matAllFinite(A)) return kInf;

    if (norm == CondNorm::Two)
    {
        Vec ev = symmetricEigen(A);
        double lo = kInf, hi = 0;
        for (double v : ev) { lo = (std::min)(lo, std::abs(v)); hi = (std::max)(hi, std::abs(v)); }
        if (!(lo > 0.0)) return kInf;
        return hi / lo;
    }

    auto [Ainv, ok] = A.invert();
    if (!ok || !matAllFinite(Ainv)) return kInf;

    if (norm == CondNorm::One)
        return matNorm1(A) * matNorm1(Ainv);
    return matNormInf(A) * matNormInf(Ainv);
}

inline const char* condNormName(CondNorm n)
{
    switch (n)
    {
        case CondNorm::One: return "1-norm";
        case CondNorm::Two: return "2-norm";
        default:            return "inf-norm";
    }
}

} // namespace pm
