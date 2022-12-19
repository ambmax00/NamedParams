#include "../NamedParams.h"
#include <string>
#include <vector>

struct Wavefunctiom
{
};

struct Atom
{
};

struct Basis
{
};

void doWayTooMuchInOneFunction(Wavefunctiom* _wavefunction, 
                               const std::vector<Atom>& _atoms,
                               const Basis& _basis,
                               const int _method,
                               std::optional<Basis> _dfBasis,
                               std::optional<std::string> _jMethod,
                               std::optional<std::string> _kMethod,
                               std::optional<std::string> _eris,
                               std::optional<double> _threshold,
                               std::optional<bool> _doDiis,
                               std::optional<bool> _doLocal,
                               std::optional<int> _scfMaxIter,
                               std::optional<int> _diisMaxIter,
                               std::optional<int> _nbBatches, 
                               std::optional<std::string> _guess,
                               std::optional<double> _diagThreshold,
                               std::optional<double> _scaling,
                               std::optional<double> _orthoDiis
                               )
{
  // do stuff ...

  // shut up compiler warnings...
  (void)_wavefunction;
  (void)_atoms;
  (void)_basis;
  (void)_method;
  (void)_dfBasis;
  (void)_jMethod;
  (void)_kMethod;
  (void)_eris;
  (void)_threshold;
  (void)_doDiis;
  (void)_doLocal;
  (void)_scfMaxIter;
  (void)_diisMaxIter;
  (void)_nbBatches;
  (void)_guess;
  (void)_diagThreshold;
  (void)_scaling;
  (void)_orthoDiis;
}

#define VARS (kWaveFunction, kAtoms, kBasis, kMethod, kDfBasis, kJMethod, kKMethod, kEris, \
              kThreshold, kDoDiis, kDoLocal, kScfMaxIter, kDiisMaxIter, kNbBatches, kGuess, \
              kDiagThreshold, kScaling, kOrthoDiis)

NAMEDPARAMS_PARAMETRIZE(namedFunction, &doWayTooMuchInOneFunction, VARS)

int main()
{
  Wavefunctiom wFunction;
  std::vector<Atom> atoms;
  Basis basis;

  namedFunction(kWaveFunction = &wFunction, kBasis = basis, kMethod = 0, kJMethod = "full", 
                kAtoms = atoms, kOrthoDiis = 1e-6, kDoDiis = true);

}