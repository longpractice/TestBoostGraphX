#pragma once
#include "Timer.h"

#include <BoostGraphX/Common.h>
#include <boost/container/pmr/list.hpp>
#include <boost/container/pmr/monotonic_buffer_resource.hpp>
#include <boost/container/pmr/polymorphic_allocator.hpp>
#include <boost/container/pmr/unsynchronized_pool_resource.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/copy.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/property_map/property_map.hpp>
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
    boost::vecS,
    boost::vecS,
    boost::bidirectionalS,
    VertexProperty,
    EdgeProperty>;

using Vertex = Dag::vertex_descriptor;
using Edge = Dag::edge_descriptor;
using EdgeIt = Dag::edge_iterator;

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

//Some assumptions here have to be taken here. It is not in any docu, but for this type of
//adjacency_list, we the VertexProperty and EdgeProperty template does not affect
//the vertex/edge iterators and the null vertex, we use this feasure to point one
using DagAuxPure = boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS>;
using VAux = DagAuxPure::vertex_descriptor;
using EAux = DagAuxPure::edge_descriptor;
using EList = std::list<Edge>;
using EListIt = std::list<Edge>::iterator;
using EAuxList = std::list<EAux>;
using VAuxList = std::list<VAux>;
using EAuxListIt = EAuxList::iterator;
using VAuxListIt = VAuxList::iterator;

static auto nullVAux()
{
    return boost::graph_traits<DagAuxPure>::null_vertex();
}

struct EAuxProp {
    //EAuxProp() = default;
    Edge eOrigin;
    std::optional<EListIt> eWalkListIt;
};

struct VAuxProp {
    //VAuxProp() = default;
    Vertex vOrigin;
    size_t firstUnVisitedEdgeId { 0 };
    //we could not use EAuxListIt here, since once it is default constructed
    //it is a "singular" iterator that we could not even assign to it
    std::optional<VAuxListIt> vUnfinishedListIt;
};

using DagAux = boost::adjacency_list<
    boost::vecS,
    boost::vecS,
    boost::directedS,
    VAuxProp,
    EAuxProp>;

using VAux = DagAux::vertex_descriptor;
using EAux = DagAux::edge_descriptor;

//return the start edge of the new circle from v,
//the second is true if the start edge corresponds to markEdge
std::pair<EListIt, bool> discoverNewClosedWalk(
    VAux v,
    DagAux& aux,
    VAuxList& unfinishedVList,
    EList& oldClosedWalk,
    std::optional<EAux> markEdge = {})
{
    EList newClosedWalk;
    auto vFirst = v;
    if (boost::out_degree(v, aux) == aux[v].firstUnVisitedEdgeId) {
        //we could not find any exit and thus no new closed walk from this v
        return { {}, false };
    }
    std::unique_ptr<EListIt> prevVisitedEOut;
    auto outEdges = boost::out_edges(v, aux);
    auto firstUnvisitedE = *(outEdges.first + aux[v].firstUnVisitedEdgeId);
    bool isMarked = markEdge ? firstUnvisitedE == markEdge.value() : false;

    if (aux[v].firstUnVisitedEdgeId != 0) {
        const auto& visitedE = *(outEdges.first + aux[v].firstUnVisitedEdgeId - 1);
        const auto& itVisitedE = aux[visitedE].eWalkListIt;
        prevVisitedEOut = std::make_unique<EListIt>(*itVisitedE);
    }

    do {
        //find the first exit for current vertex
        auto& vProp = aux[v];
        auto outDeg = boost::out_degree(v, aux);
        auto itEOut = boost::out_edges(v, aux).first + vProp.firstUnVisitedEdgeId;
        auto eOut = *itEOut;
        //this exist edge is now visited, move it from unvisited to visited
        ++vProp.firstUnVisitedEdgeId;

        newClosedWalk.emplace_back(aux[eOut].eOrigin);
        aux[eOut].eWalkListIt = { --newClosedWalk.end() };

        //v was partially visited before, if we have finished the last
        bool justBecomeUnfinished = vProp.firstUnVisitedEdgeId == 1 && vProp.firstUnVisitedEdgeId != outDeg;
        if (justBecomeUnfinished) {
            unfinishedVList.emplace_back(v);
            vProp.vUnfinishedListIt = { --unfinishedVList.end() };
        }

        //it was partially visited before and now need to be moved out
        bool justMovedOutOfUnfinishedVList = vProp.firstUnVisitedEdgeId == outDeg && outDeg > 1;
        if (justMovedOutOfUnfinishedVList) {
            unfinishedVList.erase(*vProp.vUnfinishedListIt);
        }
        v = boost::target(eOut, aux);
    } while (v != vFirst);

    auto eListIt = newClosedWalk.begin();
    if (prevVisitedEOut) {
        //the visited out edge, which need to break the previous linked list
        oldClosedWalk.splice(*prevVisitedEOut, newClosedWalk);
    } else {
        //old closed walk must be empty
        std::swap(oldClosedWalk, newClosedWalk);
    }
    return { eListIt, isMarked };
}

struct VCopier {
    VCopier(Dag& from, DagAux& to)
        : from(from)
        , to(to) {};
    Dag& from;
    DagAux& to;

    void operator()(Vertex input, VAux output) const
    {
        to[output].vOrigin = input;
    }
};

struct ECopier {
    ECopier(Dag& from, DagAux& to)
        : from(from)
        , to(to) {};

    Dag& from;
    DagAux& to;

    void operator()(Edge input, EAux output) const
    {
        to[output].eOrigin = input;
    }
};

void makeAuxDag(Dag& dag, DagAux& dagAux)
{
    {
        AutoProfiler timer { "time copying" };
        boost::copy_graph(
            dag,
            dagAux,
            boost::vertex_copy(VCopier(dag, dagAux)).edge_copy(ECopier(dag, dagAux)));
    }
}
std::list<Edge> findEulerCycle_Hierholzer(DagAux& dagAux, Vertex firstV, EAux eMark)
{
    AutoProfiler profiler { "Time without copying" };

    /*
		boost::container::pmr::pool_options eListOptions;
		eListOptions.max_blocks_per_chunk = nrEdges;
		eListOptions.largest_required_pool_block = 8 *sizeof(EAux);
		auto eListMemoryResource = boost::container::pmr::unsynchronized_pool_resource{ eListOptions };
		auto eListAllocator = boost::container::pmr::polymorphic_allocator<EAux>{ &eListMemoryResource };

		boost::container::pmr::pool_options vListOptions;
		vListOptions.max_blocks_per_chunk = nrVertices;
		vListOptions.largest_required_pool_block = 8 * sizeof(VAux);
		auto vListMemoryResource = boost::container::pmr::unsynchronized_pool_resource{ vListOptions };
		auto vListAllocator = boost::container::pmr::polymorphic_allocator<VAux>{ &vListMemoryResource };

		VAuxList unfinishedVList(vListAllocator);
		EAuxList closedWalk(eListAllocator);
		*/

    VAuxList unfinishedVList;
    EList closedWalk;
	auto eMarkedIt = closedWalk.end();

    while (closedWalk.size() < boost::num_edges(dagAux)) {
		if (unfinishedVList.empty() || unfinishedVList.front() == firstV)
		{
			auto[eItAdd, ifMarked] = discoverNewClosedWalk(firstV, dagAux, unfinishedVList, closedWalk, { eMark });
			if (ifMarked)
			{
				eMarkedIt = eItAdd;
			}
		}
		else
		{
			auto v = unfinishedVList.front();
			discoverNewClosedWalk(v, dagAux, unfinishedVList, closedWalk);
		}
    }
	EList closedWalkTmp;
	closedWalkTmp.splice(closedWalkTmp.begin(), closedWalk, eMarkedIt, closedWalk.end());
	closedWalkTmp.splice(closedWalkTmp.end(), closedWalk);
	return closedWalkTmp;
}
}
