#pragma once
#include "ComparisonInsights.h"
#include <ostream>
#include <string>

// ============================================================
// HistoryCsv: plain CSV export of run results for reports and
// spreadsheets.  GUI-free.
//
//   iterations.csv   one row per outer iteration, per method, per problem
//   summary.csv      one row per method and problem (scorecard values)
// ============================================================
namespace pm
{

class HistoryCsv
{
    static std::string quoted(const std::string& s)
    {
        std::string q = "\"";
        for (char c : s) { if (c == '"') q += '"'; q += c; }
        return q + "\"";
    }

public:
    static void writeIterations(std::ostream& o, const RunResult& r)
    {
        o.precision(10);
        o << "problem,method,k,mu,f,h_inf,h_2,kkt_inf,cond,newton_iterations,newton_status,newton_grad_inf,"
             "hessian_shift,linear_residual,x_error,lambda_error,f_error\n";
        for (auto& mc : r.runs)
            for (const SolveHistory* h : { &mc.penalty, &mc.al })
                for (auto& rec : h->outer)
                    o << quoted(mc.problem->name()) << ',' << quoted(h->method) << ',' << rec.k << ',' << rec.mu << ','
                      << rec.f << ',' << rec.hInf << ',' << rec.hNorm2 << ',' << rec.kktNorm << ',' << rec.cond << ','
                      << rec.newtonIters << ',' << rec.newtonStatus << ',' << rec.newtonGrad << ',' << rec.hessShift << ','
                      << rec.linResidual << ',' << rec.xError << ',' << rec.lambdaError << ',' << rec.fError << '\n';
    }

    static void writeSummary(std::ostream& o, const RunResult& r)
    {
        o.precision(10);
        o << "problem,method,status,subproblems,newton_iterations,final_h_inf,final_x_error,max_cond,"
             "cond_slope_vs_mu,final_mu,time_ms\n";
        for (auto& mc : r.runs)
            for (const SolveHistory* h : { &mc.penalty, &mc.al })
            {
                if (h->outer.empty()) continue;
                o << quoted(mc.problem->name()) << ',' << quoted(h->method) << ',' << quoted(h->message) << ','
                  << h->outer.size() << ',' << h->totalNewton << ',' << h->last().hInf << ',' << h->last().xError << ','
                  << h->maxCond() << ',' << ComparisonInsights::condSlope(*h) << ',' << h->last().mu << ','
                  << h->elapsedMs << '\n';
            }
    }
};

} // namespace pm
