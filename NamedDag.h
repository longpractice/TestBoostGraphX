#pragma once
#include <BoostGraphX/Common.h>
#include <boost/graph/adjacency_list.hpp>
#include <iostream>
#include <queue>
#include <string>
#include <utility>
#include <vector>

namespace bglx {
struct VertexProperty {
    std::string name;
};

struct EdgeProperty {
    std::string name;
};

using Dag = boost::adjacency_list<
    boost::setS,
    boost::setS,
    boost::bidirectionalS,
    VertexProperty,
    EdgeProperty>;

using Vertex = Dag::vertex_descriptor;
using Edge = Dag::edge_descriptor;

static void printVertex(const Dag& dag, Vertex v)
{
    std::cout << "  " << dag[v].name;
}

static void printEdge(const Dag& dag, Edge edge)
{
    auto source = boost::source(edge, dag);
    auto target = boost::target(edge, dag);
    std::cout << "Edge: " << dag[source].name << " -> " << dag[target].name << std::endl;
}

static void printEdges(const Dag& dag, const std::set<Edge>& edges)
{
    std::cout << "--------------------------------------------" << std::endl;
    for (auto& edge : edges) {
        printEdge(dag, edge);
    }
    std::cout << "--------------------------------------------" << std::endl;
};

//some assumptions here have to be taken here. It is not in any docu, but for this type of 
//adjacency_list, we the VertexProperty and EdgeProperty template does not affect
//the vertex/edge iterators and the null vertex, we use this feasure to point one 
using DagAuxPure = boost::adjacency_list<boost::setS, boost::setS, boost::bidirectionalS>;
using VAux = DagAuxPure::vertex_descriptor;
using EAux = DagAuxPure::edge_descriptor;
using EAuxList = std::list<EAux>;
using VAuxList = std::list<VAux>;
using EAuxListIt = EAuxList::iterator;
using VAuxListIt = VAuxList::iterator;

static auto nullVAux()
{
	return boost::graph_traits<DagAuxPure>::null_vertex();
}

struct EAuxProp {
    Edge eOrigin;
    //hack here since we could not easily put EAux here for linked list
    //it is only type erased, but the underlying type is still a EAux
    //not violating strict-aliasing-rule
	EAuxListIt eWalkListIt;
};

struct VAuxProp {
    Vertex vOrigin;
    std::list<EAux> unVisitedOutEdges;
	std::list<EAux> visitedOutEdges;
	VAuxListIt vUnfinishedListIt;
};

using DagAux = boost::adjacency_list<
    boost::setS,
    boost::setS,
    boost::bidirectionalS,
    VAuxProp,
    EAuxProp>;

using VAux = DagAux::vertex_descriptor;
using EAux = DagAux::edge_descriptor;


//the new closed walk must be starting from the edge that is refered to by 
//itUnVisitedOutEdge, which points to the element of the aux[v].unVisitedOutEdges
void insertNewClosedWalk(
	VAux v, 
	EAuxListIt itUnVisitedOutEdge,
	std::list<EAux>& oldClosedWalk,
	std::list<EAux>& newClosedWalk,
	DagAux& aux
)
{
	auto& prop = aux[v];

	if (!prop.visitedOutEdges.empty())
	{
		//the visited out edge, which need to break the previous linked list
		auto eOutVisited = *prop.visitedOutEdges.begin();
		auto eOutVisitedIt = aux[eOutVisited].eWalkListIt;
		oldClosedWalk.splice(eOutVisitedIt, newClosedWalk);
	}
	else
	{
		//old closed walk must be empty
		std::swap(oldClosedWalk, newClosedWalk);
	}
	prop.unVisitedOutEdges.erase(itUnVisitedOutEdge);
}

std::pair<EAuxList> discoverNewClosedWalk(VAux v, DagAux& aux, VAuxList& unfinishedVList)
{
	auto vFirst = v;
	auto& prop = aux[v];

	if (aux[v].unVisitedOutEdges.empty())
	{
		//we could not find any new walks from this v
		return { aux[v].unVisitedOutEdges.end(), {} };
	}

	//use the first excape way
	EAuxList newClosedWalk;

	do {
		auto& vProp = aux[v];
		auto itEOut = vProp.unVisitedOutEdges.begin();
		auto eOut = *itEOut;
		vProp.unVisitedOutEdges.erase(itEOut);
		vProp.visitedOutEdges.emplace_back(eOut);
		newClosedWalk.emplace_back(eOut);
		aux[eOut].eWalkListIt = --newClosedWalk.end();


		//v was partially visited before, if we have finished the last 
		if (vProp.vUnfinishedListIt != unfinishedVList.end())
		{

		}

	} while (vLast != v);
	
}

DagAux makeAuxDag(const Dag& dag)
{
    auto dagAux = DagAux {};
    std::unordered_map<Vertex, VAux> vToVAux;

    for (auto v : Range { boost::vertices(dag) }) {
        auto vAux = boost::add_vertex(dagAux);
        //set the origin vertex
        dagAux[vAux].vOrigin = v;
        vToVAux[v] = vAux;
    }

    for (auto e : Range { boost::edges(dag) }) {
        auto src = boost::source(e, dag);
        auto tgt = boost::target(e, dag);
        auto srcAux = vToVAux.find(src)->second;
        auto tgtAux = vToVAux.find(tgt)->second;
        auto [eAux, ok] = boost::add_edge(srcAux, tgtAux, dagAux);
        auto& eAuxProp = dagAux[eAux];
        eAuxProp.eOrigin = e;
        eAuxProp.pEAuxNext = nullptr;
    }
    return dagAux;
}

auto findEulerCycle_Hierholzer(const Dag& dag, Vertex vStart)
{
    //make an auxiliary dag containing additional informations
    auto dagAux = makeAuxDag(dag);
    auto nrEdges = boost::num_edges(dag);

    //to be pointed at by the EAuxProp
    std::vector<EAux> eAuxNexts;
    eAuxNexts.reserve(nrEdges);

    //we need to clean up this one
    //seed an aribitrary vertex as a start
    std::set<VAux> visitedVerticesWithUnvisitedEdges { *boost::vertices(dagAux).first };

    while (!visitedVerticesWithUnvisitedEdges.empty()) {
        auto v = *visitedVerticesWithUnvisitedEdges.begin();
        //travel from v, we may need to break a previous edge chain
        if (!dagAux[v].visitedOutEdges.empty()) {
            //we have an old edge chain here, we need to break this chain
        }
    }
}

}
