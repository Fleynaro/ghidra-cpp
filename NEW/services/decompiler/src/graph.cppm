// Port provenance: declarations and implementation originate from
// Ghidra/Features/Decompiler/src/decompile/cpp/graph.hh and graph.cc.
export module ghidra.decompiler.graph;
import std;
import ghidra.decompiler;

export namespace ghidra {
/// Export the data-flow graph in Renoir's columnar graph format.
void dump_dataflow_graph(Funcdata& data, ostream& s);

/// Export a control-flow graph in Renoir's columnar graph format.
void dump_controlflow_graph(const string& name, const BlockGraph& graph, ostream& s);

/// Export a dominator graph in Renoir's columnar graph format.
void dump_dom_graph(const string& name, const BlockGraph& graph, ostream& s);
} // namespace ghidra

// The original implementation is included only as a module implementation
// detail. It is not an active legacy source file or header in this target.
#define __GRAPH_HH__
#include "../../../../Ghidra/Features/Decompiler/src/decompile/cpp/graph.cc"
#undef __GRAPH_HH__
