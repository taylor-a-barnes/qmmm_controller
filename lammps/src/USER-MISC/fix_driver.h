/* -*- c++ -*- ----------------------------------------------------------
   LAMMPS - Large-scale Atomic/Molecular Massively Parallel Simulator
   http://lammps.sandia.gov, Sandia National Laboratories
   Steve Plimpton, sjplimp@sandia.gov

   Copyright (2003) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under
   the GNU General Public License.

   See the README file in the top-level LAMMPS directory.
------------------------------------------------------------------------- */


#ifdef FIX_CLASS

FixStyle(driver,FixDriver)

#else

#ifndef LMP_FIX_DRIVER_H
#define LMP_FIX_DRIVER_H

#include "fix.h"

namespace LAMMPS_NS {

class FixDriver : public Fix {
 public:
  FixDriver(class LAMMPS *, int, char **);
  ~FixDriver();
  int setmask();
  void init();

  // receive and update forces
  void setup(int);
  void post_force(int);

  double *add_force; // stores forces added using +FORCE command

 protected:
  void exchange_forces();       // collected forces from QM and MM slave

};

}

#endif
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 2
// indent-tabs-mode: nil
// End:
