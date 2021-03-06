#include <fstream>
#include "constraints.h"
#include "schema.h"
#include "util.h"

void Constraints::readJson (const json& pj) {
  MachineSchema::validateOrDie ("constraints", pj);
  if (pj.count("norm"))
    for (const auto& n: pj.at("norm")) {
      vguard<string> cons;
      for (const auto& p: n)
	cons.push_back (p.get<string>());
      norm.push_back (cons);
    }
  if (pj.count("prob"))
    for (const auto& p: pj.at("prob"))
      prob.push_back (p.get<string>());
  if (pj.count("rate"))
    for (const auto& r: pj.at("rate"))
      rate.push_back (r.get<string>());
}

void Constraints::writeJson (ostream& out) const {
  out << "{";
  size_t l = 0;
  if (norm.size()) {
    ++l;
    out << "\"norm\":[";
    size_t nc = 0;
    for (auto& c: norm) {
      out << (nc++ ? "," : "") << "[";
      size_t nj = 0;
      for (auto& p: c)
	out << (nj++ ? "," : "") << "\"" << p << "\"";
      out << "]";
    }
    out << "]";
  }
  if (prob.size()) {
    out << (l++ ? ",\n " : "")
	<< "\"prob\":[";
    size_t np = 0;
    for (auto& p: prob)
      out << (np++ ? "," : "") << "\"" << p << "\"";
    out << "]";
  }
  if (rate.size()) {
    out << (l++ ? ",\n " : "")
	<< "\"rate\":[";
    size_t nr = 0;
    for (auto& r: rate)
      out << (nr++ ? "," : "") << "\"" << r << "\"";
    out << "]";
  }
  out << "}" << endl;
}

Params Constraints::defaultParams() const {
  Params params;
  for (auto& c: norm)
    for (auto& cp: c)
      params.defs[cp] = 1. / (double) c.size();
  for (auto& pp: prob)
    params.defs[pp] = .5;
  for (auto& rp: rate)
    params.defs[rp] = 1;
  return params;
}
