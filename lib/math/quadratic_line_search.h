/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 18/06/2013

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __math_quadratic_line_search_h__
#define __math_quadratic_line_search_h__


#include "progressbar.h"


namespace MR
{
  namespace Math
  {
    /** \addtogroup Optimisation
    @{ */

    //! Computes the minimum of a 1D function using a quadratic line search.
    /*! This functor operates on a cost function class that must define a
     *  operator() const method. The method must take a single ValueType
     *  argument x and return the cost of the function at x.
     *
     *  This line search is fast for functions that are smooth and convex.
     *  Functions that do not obey these criteria may not converge.
     *
     *  The min_bound and max_bound arguments define values that are used to
     *  initialise the search. If these bounds do not bracket the minimum,
     *  then the search will return NAN. Furthermore, if the relevant function
     *  is not sufficiently smooth, and the search begins to diverge before
     *  finding a local minimum to within the specified tolerance, then the
     *  search will also return NAN.
     *
     *  This effect can be cancelled by calling:
     *  \code
     *  set_exit_if_outside_bounds (false);
     *  \endcode
     *  That way, if the estimated minimum is outside the current
     *  bracketed area, the search area will be widened accordingly, and the
     *  process repeated until a local minimum is found to within the specified
     *  tolerance. Beware however; there is no guarantee that the search will
     *  converge in all cases, so be conscious of the nature of your data.
     *
     *
     * Typical usage:
     * \code
     * CostFunction cost_function();
     * QuadraticLineSearch<double> line_search (-1.0, 1.0);
     * line_search.set_tolerance (0.01);
     * line_search.set_message ("optimising... ");
     * const double optimal_value = line_search (cost_function);
     *
     * \endcode
     */




    template <typename ValueType>
    class QuadraticLineSearch
    {

      public:

        // TODO Return error code if converging toward a maxima instead of a minima
        // TODO Separate return codes for above & below domain
        enum return_t {SUCCESS, EXECUTING, OUTSIDE_BOUNDS, NONCONVEX, NONCONVERGING};

        QuadraticLineSearch (const ValueType lower_bound, const ValueType upper_bound) :
          init_lower (lower_bound),
          init_mid   (0.5 * (lower_bound + upper_bound)),
          init_upper (upper_bound),
          value_tolerance (0.001 * (upper_bound - lower_bound)),
          function_tolerance (0.0),
          exit_outside_bounds (true),
          max_iters (50),
          status (SUCCESS) { }


        void set_lower_bound            (const ValueType i)    { init_lower = i; }
        void set_init_estimate          (const ValueType i)    { init_mid = i; }
        void set_upper_bound            (const ValueType i)    { init_upper = i; }
        void set_value_tolerance        (const ValueType i)    { value_tolerance = i; }
        void set_function_tolerance     (const ValueType i)    { function_tolerance = i; }
        void set_exit_if_outside_bounds (const bool i)         { exit_outside_bounds = i; }
        void set_max_iterations         (const size_t i)       { max_iters = i; }
        void set_message                (const std::string& i) { message = i; }

        return_t get_status() const { return status; }


        template <class Functor>
        ValueType operator() (Functor& functor) const
        {

          status = EXECUTING;

          std::unique_ptr<ProgressBar> progress (message.size() ? new ProgressBar (message) : nullptr);

          ValueType l = init_lower, m = init_mid, u = init_upper;
          ValueType fl = functor (l), fm = functor (m), fu = functor (u);
          // TODO Need to test if these bounds are producing a NAN CF
          size_t iters = 0;

          while (iters++ < max_iters) {

            // TODO When testing for nonconvexity, the problem may also arise due to quantisation
            //   in the cost function
            // Would like to have a fractional threshold on the cost function i.e. if it's really flat,
            //   return successfully
            // Difficult to do this without knowledge of the cost function
            if (fm > (fl + ((fu-fl)*(m-l)/(u-l)))) {
              if ((std::min(m-l, u-m) < value_tolerance) || (std::abs((fu-fl)/(0.5*(fu+fl))) < function_tolerance)) {
                status = SUCCESS;
                return m;
              }
              status = NONCONVEX;
              return NAN;
            }

            const ValueType sl = (fm-fl) / (m-l);
            const ValueType su = (fu-fm) / (u-m);

            const ValueType n = (0.5*(l+m)) - ((sl*(u-l)) / (2.0*(su-sl)));

            const ValueType fn = functor (n);
            if (!std::isfinite(fn))
              return m;

            if (n < l) {
              if (exit_outside_bounds) {
                status = OUTSIDE_BOUNDS;
                return NAN;
              }
              u = m; fu = fm;
              m = l; fm = fl;
              l = n; fl = fn;
            } else if (n < m) {
              if (fn > fm) {
                l = n; fl = fn;
              } else {
                u = m; fu = fm;
                m = n; fm = fn;
              }
            } else if (n == m) {
              return n;
            } else if (n < u) {
              if (fn > fm) {
                u = n; fu = fn;
              } else {
                l = m; fl = fm;
                m = n; fm = fn;
              }
            } else {
              if (exit_outside_bounds) {
                status = OUTSIDE_BOUNDS;
                return NAN;
              }
              l = m; fl = fm;
              m = u; fm = fu;
              u = n; fu = fn;
            }

            if (progress)
              ++(*progress);

            if ((u-l) < value_tolerance) {
              status = SUCCESS;
              return m;
            }

          }

          status = NONCONVERGING;
          return NAN;

        }



        template <class Functor>
        ValueType verbose (Functor& functor) const
        {

          status = EXECUTING;

          ValueType l = init_lower, m = init_mid, u = init_upper;
          ValueType fl = functor (l), fm = functor (m), fu = functor (u);
          std::cerr << "Initialising quadratic line search\n";
          std::cerr << "        Lower        Mid         Upper\n";
          std::cerr << "Pos     " << str (l)  << "           " << str(m)  << "        " << str(u)  << "\n";
          std::cerr << "Value   " << str (fl) << "           " << str(fm) << "        " << str(fu) << "\n";
          size_t iters = 0;

          while (iters++ < max_iters) {

            if (fm > (fl + ((fu-fl)*(m-l)/(u-l)))) {
              if (std::min(m-l, u-m) < value_tolerance) {
                std::cerr << "Returning due to nonconvexity, through successfully\n";
                status = SUCCESS;
                return m;
              }
              status = NONCONVEX;
              std::cerr << "Returning due to nonconvexity, unsuccessfully\n";
              return NAN;
            }

            const ValueType sl = (fm-fl) / (m-l);
            const ValueType su = (fu-fm) / (u-m);

            const ValueType n = (0.5*(l+m)) - ((sl*(u-l)) / (2.0*(su-sl)));

            const ValueType fn = functor (n);

            std::cerr << "  New point " << str(n) << ", value " << str(fn) << "\n";

            if (n < l) {
              if (exit_outside_bounds) {
                status = OUTSIDE_BOUNDS;
                return NAN;
              }
              u = m; fu = fm;
              m = l; fm = fl;
              l = n; fl = fn;
            } else if (n < m) {
              if (fn > fm) {
                l = n; fl = fn;
              } else {
                u = m; fu = fm;
                m = n; fm = fn;
              }
            } else if (n == m) {
              return n;
            } else if (n < u) {
              if (fn > fm) {
                u = n; fu = fn;
              } else {
                l = m; fl = fm;
                m = n; fm = fn;
              }
            } else {
              if (exit_outside_bounds) {
                status = OUTSIDE_BOUNDS;
                return NAN;
              }
              l = m; fl = fm;
              m = u; fm = fu;
              u = n; fu = fn;
            }

            std::cerr << "\n";
            std::cerr << "Pos     " << str (l)  << "           " << str(m)  << "        " << str(u)  << "\n";
            std::cerr << "Value   " << str (fl) << "           " << str(fm) << "        " << str(fu) << "\n";

            if ((u-l) < value_tolerance) {
              status = SUCCESS;
              std::cerr << "Returning successfully\n";
              return m;
            }

          }

          status = NONCONVERGING;
          std::cerr << "Returning due to too many iterations\n";
          return NAN;
        }



      private:
        ValueType init_lower, init_mid, init_upper, value_tolerance, function_tolerance;
        bool exit_outside_bounds;
        size_t max_iters;
        std::string message;

        mutable return_t status;


    };




  }
}

#endif
