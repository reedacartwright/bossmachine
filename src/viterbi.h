#ifndef VITERBI_INCLUDED
#define VITERBI_INCLUDED

#include "dpmatrix.h"

class ViterbiMatrix : public DPMatrix {
private:
  inline void pathIterate (double& bestLogLike, StateIndex& bestSource, EvaluatedMachineState::TransIndex& bestTransIndex, const EvaluatedMachineState::InOutStateTransMap& inOutStateTransMap, InputToken inTok, OutputToken outTok, InputIndex inPos, OutputIndex outPos) const {
    auto visit = [&] (StateIndex src, EvaluatedMachineState::TransIndex ti, double tll) {
      if (tll > bestLogLike) {
	bestLogLike = tll;
	bestSource = src;
	bestTransIndex = ti;
      }
    };
    iterate (inOutStateTransMap, inTok, outTok, inPos, outPos, visit);
  }

public:
  ViterbiMatrix (const EvaluatedMachine& machine, const SeqPair& seqPair);
  double logLike() const;
  MachinePath path (const Machine&) const;
};

#endif /* VITERBI_INCLUDED */
