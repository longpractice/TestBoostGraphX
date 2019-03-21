#pragma once
#include <BoostGraphX/Common.h>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/copy.hpp>
#include <boost/graph/graph_utility.hpp>
#include <iostream>
#include <queue>
#include <string>
#include <utility>
#include <vector>
#include <boost/property_map/property_map.hpp>

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
	using DagAuxPure = boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS>;
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
		EAuxProp() = default;
		Edge eOrigin;
		std::shared_ptr<EAuxListIt> eWalkListIt{ nullptr };
	};

	struct VAuxProp {
		VAuxProp() = default;
		Vertex vOrigin;
		std::list<EAux> unVisitedOutEdges;
		std::list<EAux> visitedOutEdges;
		//we could not use EAuxListIt here, since once it is default constructed
		//it is a "singular" iterator that we could not even assign to it
		std::shared_ptr<VAuxListIt> vUnfinishedListIt{ nullptr };
	};

	using DagAux = boost::adjacency_list<
		boost::vecS,
		boost::vecS,
		boost::bidirectionalS,
		VAuxProp,
		EAuxProp>;

	using VAux = DagAux::vertex_descriptor;
	using EAux = DagAux::edge_descriptor;

	//the new closed walk must be starting from the edge that is refered to by
	//itUnVisitedOutEdge, which points to the element of the aux[v].unVisitedOutEdges
	void insertNewClosedWalk(
		VAux v,
		std::list<EAux>& oldClosedWalk,
		std::list<EAux>& newClosedWalk,
		DagAux& aux)
	{
		auto& prop = aux[v];

		if (!prop.visitedOutEdges.empty()) {
			//the visited out edge, which need to break the previous linked list
			auto eOutVisited = *prop.visitedOutEdges.begin();
			auto eOutVisitedIt = aux[eOutVisited].eWalkListIt;
			oldClosedWalk.splice(*eOutVisitedIt, newClosedWalk);
		}
		else {
			//old closed walk must be empty
			std::swap(oldClosedWalk, newClosedWalk);
		}
	}

	void discoverNewClosedWalk(VAux v, DagAux& aux, VAuxList& unfinishedVList, EAuxList& oldClosedWalk)
	{
		EAuxList newClosedWalk;
		auto vFirst = v;
		if (aux[v].unVisitedOutEdges.empty()) {
			//we could not find any exit and thus no new closed walk from this v
			return;
		}
		std::unique_ptr<EAuxListIt> prevVisitedVOut;
		if (!aux[v].visitedOutEdges.empty())
		{
			auto firstE = aux[v].visitedOutEdges.front();
			auto itE = aux[firstE].eWalkListIt;
			prevVisitedVOut = std::make_unique<EAuxListIt>(*itE);
		}

		//use the first excape way
		do {
			//find the first exit for current vertex
			auto& vProp = aux[v];
			auto itEOut = vProp.unVisitedOutEdges.begin();
			auto eOut = *itEOut;

			//this exist edge is now visited, move it from unvisited to visited
			vProp.unVisitedOutEdges.erase(itEOut);
			vProp.visitedOutEdges.emplace_back(eOut);

			newClosedWalk.emplace_back(eOut);
			auto p = std::make_shared<EAuxListIt>(--newClosedWalk.end());
			auto& sh = aux[eOut].eWalkListIt;
			sh = p;

			//v was partially visited before, if we have finished the last
			bool justBecomeUnfinished = vProp.visitedOutEdges.size() == 1 && !vProp.unVisitedOutEdges.empty();
			if (justBecomeUnfinished) {
				unfinishedVList.emplace_back(v);
				vProp.vUnfinishedListIt = std::make_shared<VAuxListIt>(--unfinishedVList.end() );
			}

			//it was partially visited before and now need to be moved out
			bool justMovedOutOfUnfinishedVList = vProp.unVisitedOutEdges.empty() && vProp.visitedOutEdges.size() > 1;
			if (justMovedOutOfUnfinishedVList)
			{
				unfinishedVList.erase(*vProp.vUnfinishedListIt);
			}
			v = boost::target(eOut, aux);
		} while (v != vFirst);

		if (prevVisitedVOut) {
			//the visited out edge, which need to break the previous linked list
			oldClosedWalk.splice(*prevVisitedVOut, newClosedWalk);
		}
		else {
			//old closed walk must be empty
			std::swap(oldClosedWalk, newClosedWalk);
		}
	}

	//insert handle once once
	void expandClosedWalk(VAux v, DagAux& aux, VAuxList& unfinishedVList, EAuxList& closedWalk)
	{
		auto newClosedWalk = EAuxList{};
		discoverNewClosedWalk(v, aux, unfinishedVList, newClosedWalk);
		insertNewClosedWalk(v, closedWalk, newClosedWalk, aux);
	}


	struct VCopier
	{
		VCopier(Dag& from, DagAux& to) :from(from), to(to) {};
		Dag& from;
		DagAux& to;

		void operator()(Vertex input, VAux output) const {
			to[output].vOrigin = input;
			to[output].vUnfinishedListIt = nullptr;
		}
	};

	struct ECopier
	{
		ECopier(Dag& from, DagAux& to) :from(from), to(to) {};

		Dag& from;
		DagAux& to;

		void operator()(Edge input, EAux output) const {
			to[output].eOrigin = input;
			to[output].eWalkListIt = nullptr;
		}
	};


	void fillInAuxDagInfo(DagAux& aux)
	{
		for (auto v : Range{ boost::vertices(aux) })
		{
			auto& prop = aux[v];
			for (auto e : Range{ boost::out_edges(v, aux) })
			{
				prop.unVisitedOutEdges.emplace_back(e);
			}
		}
	}

	void makeAuxDag(Dag& dag, DagAux& dagAux)
	{
		boost::copy_graph(
			dag,
			dagAux,
			boost::vertex_copy(VCopier(dag, dagAux)).edge_copy(ECopier(dag, dagAux))
		);
		fillInAuxDagInfo(dagAux);
	}

	std::vector<Edge> findEulerCycle_Hierholzer(Dag& dag, Vertex firstV)
	{
		//make an auxiliary dag containing additional informations
		auto dagAux = DagAux{};
		makeAuxDag(dag, dagAux);
		auto nrEdges = boost::num_edges(dag);
		VAuxList unfinishedVList;
		EAuxList closedWalk;

		discoverNewClosedWalk(firstV, dagAux, unfinishedVList, closedWalk);

		while (closedWalk.size() < nrEdges)
		{
			firstV = unfinishedVList.front();
			discoverNewClosedWalk(firstV, dagAux, unfinishedVList, closedWalk);
		}

		std::vector<Edge> edges;
		edges.reserve(nrEdges);
		for (auto eAux : closedWalk)
		{
			edges.emplace_back(dagAux[eAux].eOrigin);
		}

		return edges;
	}
}
