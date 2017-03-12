#include <cstdlib>
#include <stdexcept>
#include <fstream>
#include <iomanip>
#include <random>
#include <boost/program_options.hpp>

#include "../src/vguard.h"
#include "../src/logger.h"
#include "../src/fastseq.h"
#include "../src/machine.h"
#include "../src/seqpair.h"
#include "../src/constraints.h"
#include "../src/params.h"
#include "../src/fitter.h"

using namespace std;

namespace po = boost::program_options;

int main (int argc, char** argv) {

  try {
    
    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()
      ("help,h", "display this help message")
      ("null,n", "create null transducer (default)")
      ("generate,g", po::value<string>(), "create generator from sequence")
      ("pipe,p", po::value<vector<string> >(), "pipe (compose) machine(s)")
      ("concat,c", po::value<vector<string> >(), "concatenate machine(s)")
      ("union,u", po::value<string>(), "take union with machine")
      ("weight,w", po::value<string>(), "parameterize union with probability")
      ("kleene,k", "make Kleene closure")
      ("loop,l", po::value<string>(), "Kleene closure with probability parameter")
      ("accept,a", po::value<string>(), "create acceptor from sequence")
      ("save,s", po::value<string>(), "save machine")
      ("fit,f", "fit parameters using Baum-Welch")
      ("params,P", po::value<string>(), "parameter file")
      ("constraints,C", po::value<string>(), "constraints file")
      ("data,d", po::value<string>(), "training sequence file")
      ("verbose,v", po::value<int>()->default_value(2), "verbosity level")
      ("log", po::value<vector<string> >(), "log everything in this function")
      ("nocolor", "log in monochrome")
      ;

    po::positional_options_description p;
    p.add("pipe", -1);

    po::variables_map vm;
    po::store (po::command_line_parser(argc,argv).options(desc).positional(p).run(), vm);
    po::notify(vm);    

    // parse args
    if (vm.count("help")) {
      cout << desc << "\n";
      return 1;
    }

    logger.parseLogArgs (vm);

    // create transducer
    Machine machine;

    // Null
    if (vm.count("null"))
      machine = Machine::null();

    // Compositions
    if (vm.count("pipe")) {
      const vector<string> machines = vm.at("pipe").as<vector<string> >();
      for (auto iter = machines.rbegin(); iter != machines.rend(); ++iter) {
	LogThisAt(2,"Loading transducer " << *iter << endl);
	const char* filename = (*iter).c_str();
	const Machine loaded = MachineLoader::fromFile(filename);
	machine = machine.nStates() ? Machine::compose (loaded, machine) : loaded;
      }
    }

    // Generator
    if (vm.count("generate")) {
      const NamedInputSeq inSeq = NamedInputSeq::fromFile (vm.at("generate").as<string>());
      LogThisAt(2,"Creating generator for sequence " << inSeq.name << endl);
      const Machine generator = Machine::generator (inSeq.name, inSeq.seq);
      machine = machine.nStates() ? Machine::compose (generator, machine) : generator;
    }

    // Concatenations
    if (vm.count("concat")) {
      const vector<string> machines = vm.at("concat").as<vector<string> >();
      for (const auto& filename: machines) {
	LogThisAt(2,"Concatenating transducer " << filename << endl);
	const Machine concat = MachineLoader::fromFile(filename);
	machine = machine.nStates() ? Machine::concatenate (machine, concat) : concat;
      }
    }

    // Union
    if (vm.count("union")) {
      const string filename = vm.at("union").as<string>();
      LogThisAt(2,"Taking union with transducer " << filename << endl);
      const Machine uni = MachineLoader::fromFile(filename);
      if (vm.count("weight"))
	machine = Machine::unionOf (uni, machine, WeightExpr(vm.at("weight").as<string>()));
      else
	machine = Machine::unionOf (uni, machine);
    }

    // Kleene closure
    Require (!(vm.count("kleene") && vm.count("loop")), "Can't specify both --kleene and --loop");
    if (vm.count("kleene")) {
      LogThisAt(2,"Making Kleene closure" << endl);
      machine = machine.kleeneClosure();
    } else if (vm.count("loop")) {
      const string geomParam = vm.at("loop").as<string>();
      LogThisAt(2,"Making Kleene closure with loop parameter " << geomParam << endl);
      machine = machine.kleeneClosure (WeightExpr(geomParam));
    }
    
    // Acceptor
    if (vm.count("accept")) {
      const NamedOutputSeq outSeq = NamedInputSeq::fromFile (vm.at("accept").as<string>());
      LogThisAt(2,"Creating acceptor for sequence " << outSeq.name << endl);
      const Machine acceptor = Machine::acceptor (outSeq.name, outSeq.seq);
      machine = machine.nStates() ? Machine::compose(machine,acceptor) : acceptor;
    }

    // default to null
    if (!machine.nStates())
      machine = Machine::null();
    
    // save transducer
    if (vm.count("save")) {
      const string savefile = vm.at("save").as<string>();
      ofstream out (savefile);
      machine.writeJson (out);
    } else if (!vm.count("fit"))
      machine.writeJson (cout);

    // fit parameters
    if (vm.count("fit")) {
      Require (vm.count("constraints") && vm.count("data"),
	       "To fit parameters, please specify a constraints file and a data file");
      MachineFitter fitter;
      fitter.machine = machine;
      fitter.constraints = Constraints::fromFile(vm.at("constraints").as<string>());
      fitter.seed = vm.count("params") ? Params::fromFile(vm.at("params").as<string>()) : fitter.constraints.defaultParams();
      const SeqPairList data = SeqPairList::fromFile(vm.at("data").as<string>());
      cout << fitter.fit(data).toJsonString() << endl;
    }
    
  } catch (const std::exception& e) {
    cerr << e.what() << endl;
    return EXIT_FAILURE;
  }
  
  return EXIT_SUCCESS;
}
