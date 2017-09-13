/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   Copyright (c) 2012-2017 The plumed team
   (see the PEOPLE file at the root of the distribution for a list of names)

   See http://www.plumed.org for more information.

   This file is part of plumed, version 2.

   plumed is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   plumed is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with plumed.  If not, see <http://www.gnu.org/licenses/>.
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
#include "SymmetryFunctionBase.h"
#include "multicolvar/MultiColvarBase.h"
#include "core/ActionRegister.h"

#include <string>
#include <cmath>

using namespace std;

namespace PLMD {
namespace symfunc {

//+PLUMEDOC MCOLVAR TETRAHEDRAL
/*
Calculate the degree to which the environment about ions has a tetrahedral order.

We can measure the degree to which the first coordination shell around any atom, \f$i\f$ is
tetrahedrally ordered using the following function.

\f[
 s(i) = \frac{1}{\sum_j \sigma( r_{ij} )} \sum_j \sigma( r_{ij} )\left[ \frac{(x_{ij} + y_{ij} + z_{ij})^3}{r_{ij}^3} +
                                                                        \frac{(x_{ij} - y_{ij} - z_{ij})^3}{r_{ij}^3} +
                                                                        \frac{(-x_{ij} + y_{ij} - z_{ij})^3}{r_{ij}^3} +
                                                                        \frac{(-x_{ij} - y_{ij} + z_{ij})^3}{r_{ij}^3} \right]
\f]

Here \f$r_{ij}\f$ is the magnitude fo the vector connecting atom \f$i\f$ to atom \f$j\f$ and \f$x_{ij}\f$, \f$y_{ij}\f$ and \f$z_{ij}\f$
are its three components.  The function  \f$\sigma( r_{ij} )\f$ is a \ref switchingfunction that acts on the distance between
atoms \f$i\f$ and \f$j\f$.  The parameters of this function should be set so that the function is equal to one
when atom \f$j\f$ is in the first coordination sphere of atom \f$i\f$ and is zero otherwise.

\par Examples

The following command calculates the average value of the tetrahedrality parameter for a set of 64 atoms all of the same type
and outputs this quantity to a file called colvar.

\plumedfile
tt: TETRAHEDRAL SPECIES=1-64 SWITCH={RATIONAL D_0=1.3 R_0=0.2} MEAN
PRINT ARG=tt.mean FILE=colvar
\endplumedfile

The following command calculates the number of tetrahedrality parameters that are greater than 0.8 in a set of 10 atoms.
In this calculation it is assumed that there are two atom types A and B and that the first coordination sphere of the
10 atoms of type A contains atoms of type B.  The formula above is thus calculated for ten different A atoms and within
it the sum over \f$j\f$ runs over 40 atoms of type B that could be in the first coordination sphere.

\plumedfile
tt: TETRAHEDRAL SPECIESA=1-10 SPECIESB=11-40 SWITCH={RATIONAL D_0=1.3 R_0=0.2} MORE_THAN={RATIONAL R_0=0.8}
PRINT ARG=tt.* FILE=colvar
\endplumedfile

*/
//+ENDPLUMEDOC


class Tetrahedral : public SymmetryFunctionBase {
public:
  static void shortcutKeywords( Keywords& keys );
  static void expandShortcut( const std::string& lab, const std::vector<std::string>& words,
                              const std::map<std::string,std::string>& keys,
                              std::vector<std::vector<std::string> >& actions );
  static void registerKeywords( Keywords& keys );
  explicit Tetrahedral(const ActionOptions&);
  void compute( const double& val, const Vector& dir, MultiValue& myvals ) const ;
};

PLUMED_REGISTER_ACTION(Tetrahedral,"TETRAHEDRAL")
PLUMED_REGISTER_SHORTCUT(Tetrahedral,"TETRAHEDRAL")

void Tetrahedral::shortcutKeywords( Keywords& keys ) {
  SymmetryFunctionBase::shortcutKeywords( keys );
}

void Tetrahedral::expandShortcut( const std::string& lab, const std::vector<std::string>& words,
                                  const std::map<std::string,std::string>& keys,
                                  std::vector<std::vector<std::string> >& actions ) {
  SymmetryFunctionBase::expandMatrix( true, lab, words, keys, actions );
  std::vector<std::string> input; input.push_back(lab + ":"); input.push_back("TETRAHEDRAL");
  input.push_back("WEIGHT=" + lab + "_mat.w" ); input.push_back("VECTORS1=" + lab + "_mat.x" );
  input.push_back("VECTORS2=" + lab + "_mat.y" ); input.push_back("VECTORS3=" + lab + "_mat.z" ); 
  actions.push_back( input );
  // Input for denominator (coord)
  std::vector<std::string> d_input; d_input.push_back(lab + "_denom:"); d_input.push_back("COORDINATIONNUMBER");
  d_input.push_back("WEIGHT=" + lab + "_mat.w"); actions.push_back( d_input );
  // Input for matheval action
  std::vector<std::string> me_input; me_input.push_back(lab + "_n:"); me_input.push_back("MATHEVAL");
  me_input.push_back("ARG1=" + lab); me_input.push_back("ARG2=" + lab + "_denom"); me_input.push_back("FUNC=x/y");
  me_input.push_back("PERIODIC=NO"); actions.push_back(me_input);
  multicolvar::MultiColvarBase::expandFunctions( lab, lab + "_n", "", words, keys, actions );
}

void Tetrahedral::registerKeywords( Keywords& keys ) {
  SymmetryFunctionBase::registerKeywords( keys );
}

Tetrahedral::Tetrahedral(const ActionOptions&ao):
  Action(ao),
  SymmetryFunctionBase(ao)
{
  addValueWithDerivatives(); checkRead();
}

void Tetrahedral::compute( const double& val, const Vector& distance, MultiValue& myvals ) const {
  double sp1 = +distance[0]+distance[1]+distance[2];
  double sp2 = +distance[0]-distance[1]-distance[2];
  double sp3 = -distance[0]+distance[1]-distance[2];
  double sp4 = -distance[0]-distance[1]+distance[2];

  double sp1c = pow( sp1, 3 );
  double sp2c = pow( sp2, 3 );
  double sp3c = pow( sp3, 3 );
  double sp4c = pow( sp4, 3 );

  double d1 = distance.modulo();
  double r3 = pow( d1, 3 );
  double r5 = pow( d1, 5 );

  double tmp = sp1c/r3 + sp2c/r3 + sp3c/r3 + sp4c/r3;

  double t1=(3*sp1c)/r5; double tt1=((3*sp1*sp1)/r3);
  double t2=(3*sp2c)/r5; double tt2=((3*sp2*sp2)/r3);
  double t3=(3*sp3c)/r5; double tt3=((3*sp3*sp3)/r3);
  double t4=(3*sp4c)/r5; double tt4=((3*sp4*sp4)/r3);

  Vector myder;
  myder[0] = (tt1-(distance[0]*t1))  + (tt2-(distance[0]*t2))  + (-tt3-(distance[0]*t3))  + (-tt4-(distance[0]*t4));
  myder[1] = (tt1-(distance[1]*t1))  + (-tt2-(distance[1]*t2))  + (tt3-(distance[1]*t3))  + (-tt4-(distance[1]*t4));
  myder[2] = (tt1-(distance[2]*t1))  + (-tt2-(distance[2]*t2))  + (-tt3-(distance[2]*t3))  + (tt4-(distance[2]*t4));

  // Accumulate derivatives
  addToValue( 0, val*tmp, myvals ); 
  addWeightDerivative( 0, tmp, myvals );
  addVectorDerivatives( 0, val*myder, myvals ); 
}

}
}

