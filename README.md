<div align="center">

# Penalty Methods

**Quadratic penalty method against the augmented Lagrangian method for equality-constrained optimisation, with a step-by-step ill-conditioning analysis, in a cross-platform C++ desktop application built on natID.**

Numerical Optimisations · Data Science and Artificial Intelligence · Faculty of Electrical Engineering, University of Sarajevo

![C++](https://img.shields.io/badge/C++-20-blue)
![CMake](https://img.shields.io/badge/CMake-3.17+-green)
![natID](https://img.shields.io/badge/natID-framework-orange)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20macOS%20%7C%20Linux-lightgray)

</div>

| | |
|---|---|
| **Course** | Numerical Optimisations |
| **Professor** | Izudin Džafić |
| **Student** | Davud Ćuprija |
| **Student ID** | 20154 |
| **Academic year** | 2025/26 |
| **Paper** | [Penalty Methods - Paper.pdf](docs/Penalty%20Methods%20-%20Paper.pdf) |

## Overview

The application solves

$$
\min_{x \in \mathbb{R}^n} f(x) \quad \text{subject to} \quad h(x) = 0, \qquad h : \mathbb{R}^n \to \mathbb{R}^m,
$$

with two methods that share one Newton solver:

| Method | Subproblem | Update |
|---|---|---|
| Quadratic penalty | $\min_x Q(x;\mu_k) = f(x) + \frac{\mu_k}{2}\lVert h(x)\rVert^2$ | $\mu_{k+1} = \rho\,\mu_k$ |
| Augmented Lagrangian | $\min_x \mathcal{L}_A(x;\lambda_k,\mu) = f(x) + \lambda_k^\top h(x) + \frac{\mu}{2}\lVert h(x)\rVert^2$ | $\lambda_{k+1} = \lambda_k + \mu\,h(x_k)$ |

The Hessian of the penalty function is $\nabla^2 Q = \nabla^2_{xx} L + \mu\,J^\top J$. As $\mu$ grows it acquires $m$ eigenvalues of order $\mu$, so its condition number grows like $O(\mu)$ and Newton's method works with increasingly ill-conditioned matrices. The augmented Lagrangian reaches feasibility through the multiplier update with a fixed $\mu$, so its Hessian stays well conditioned. The application records the condition number of the Hessian after every outer iteration so this can be observed directly.

Everything is written from scratch: problem derivatives, the modified Newton method (Cholesky test with a diagonal shift, Armijo line search), both outer loops, and the condition numbers $\kappa_\infty$ and $\kappa_1$ (through the natID matrix inverse) and $\kappa_2$ (Jacobi eigenvalue method).

## Test problems

| Problem | $n$ | $m$ | Solution |
|---|---|---|---|
| Conic curve fit: $\min \theta^\top M\theta$ s.t. $\theta^\top\theta = 1$ | 6 | 1 | eigenvector of $M$ for its smallest eigenvalue |
| Circle projection | 2 | 1 | $x^\ast = (2,1)/\sqrt5$ |
| Linear objective on a circle (Nocedal and Wright, Example 17.1) | 2 | 1 | $x^\ast = (-1,-1)$ |
| Hock-Schittkowski problem 7 | 2 | 1 | $x^\ast = (0,\sqrt3)$ |
| Equality-constrained QP | 4 | 2 | KKT system |

The curve fit uses 40 noisy points on an ellipse; the number of points, the noise level and the seed can be changed in the application.

## Features

- **Summary:** findings generated from the run and a scorecard of both methods (outcome, subproblems, Newton iterations, violation, error, largest condition number) with the better value marked.
- **Landscape:** maps of $Q$ and $\mathcal{L}_A$ side by side at a selected outer iteration, with the constraint, the Newton paths and $x^\ast$, above the constraint violation history; for the curve fit the data and the fitted conics of both methods.
- **Convergence:** constraint violation per outer iteration and distance to the solution against the Newton iterations spent, for both methods.
- **Conditioning:** condition number against $\mu$ (all problems after Run all problems) and against the outer iteration, Newton iterations per subproblem and the gradient norm Newton actually reached.
- **Data:** the full iteration log.
- **Controls:** sliders for $\mu_0$, growth factor, $\mu_{\max}$, the augmented Lagrangian $\mu$ with an optional adaptive rule, tolerances, iteration limits, the condition number norm and the curve-fit data. Runs happen in the background, can be stopped and can repeat on every change. Run all problems repeats the comparison on every test problem.
- **Export:** Export all writes the Summary, Landscape, Convergence and Conditioning pages as PDF and the data as CSV into a new time-stamped folder under `Documents/Penalty Methods/Exports`; single pages can be exported as PDF or SVG. No file dialogs are used.

Keyboard shortcuts (Ctrl on Windows and Linux, Cmd on macOS): R run, Shift+R run all problems, 1 to 5 switch pages, E export all.

## Build

Requirements: CMake 3.17 or newer, a C++20 compiler (MSVC 2022 or newer, Apple Clang 15+, GCC 12+) and the [natID SDK](https://github.com/idzafic/natID) in `$HOME/natID.SDK`. On Linux install `libgtk-4-dev` and `libadwaita-1-dev`.

```bash
cd Implementation
mkdir -p ~/natID.RAMDisk/Out

# Linux and macOS
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# Windows (Visual Studio)
cmake -B build -A x64
cmake --build build --config Release
```

natID places the executables in `~/natID.RAMDisk/Out/PenaltyMethods/Release/`, not in `build/`. The build produces two programs:

- `PenaltyMethods`, the application;
- `PenaltyMethodsTests`, a console program with 158 checks: analytic derivatives against finite differences, both methods against the reference solutions, and the conditioning trend. It exits with code 0 when all checks pass.

## Project structure

```
dsai_no_20154/
  Implementation/
    src/
      core/                        numerical core, no GUI code
        IProblem.h                 f, grad f, Hess f, h, J, Hess h_i
        AnalyticProblems.h         four problems with closed-form solutions
        ConicFitProblem.h          curve fit under a normalisation constraint
        ProblemRegistry.h          the one place where problems are listed
        MeritFunctions.h           Q and L_A with gradients and Hessians
        NewtonSolver.h             modified Newton method with diagnostics
        IConstrainedSolver.h       interface of the outer methods
        QuadraticPenaltySolver.h
        AugmentedLagrangianSolver.h
        SolveHistory.h             one record per outer iteration
        LinAlg.h                   natID matrices, Cholesky, Jacobi, condition numbers
        ComparisonRunner.h, ComparisonInsights.h, HistoryCsv.h
      ui/                          navigation rail, pages, charts, inspector
    tests/TestMain.cpp
    res/                           icons and translations
    CMakeLists.txt, PenaltyMethods.cmake
  docs/                            paper (PDF)
```

A new test problem implements `IProblem` and adds one entry to `ProblemRegistry.h`; solvers, pages and exports pick it up without changes.

## Results in brief

On all five problems the condition number of the penalty Hessian grows with a log-log slope of 1.000 against $\mu$ and reaches $10^{12}$ to $10^{13}$ at $\mu = 10^{12}$, while the augmented Lagrangian with $\mu = 10$ stays between 14 and 84. Beyond $\mu \approx 10^6$ to $10^8$ the inner Newton method can no longer meet its gradient tolerance. At default settings the augmented Lagrangian needs 92 instead of 127 Newton iterations over all problems. Formulas, figures and the full analysis are in the [paper](docs/Penalty%20Methods%20-%20Paper.pdf).
