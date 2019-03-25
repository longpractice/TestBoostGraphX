#pragma once
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

namespace bglx::test
{
	using Dag_VSet = boost::adjacency_list<boost::setS, boost::setS, boost::directedS>;
	
	
	struct VertexProperty
	{
		std::string name;
	};

	struct EdgeProperty
	{
		std::string name;
	};

	using Dag = boost::adjacency_list<
		boost::setS,
		boost::vecS,
		boost::directedS,
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

}
