#include "NamedDag.h"
#include <optional>
#include <boost/graph/copy.hpp>
#include <stack>
#include "catch.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/erdos_renyi_generator.hpp>
#include <boost/graph/graph_concepts.hpp>
#include <boost/random/linear_congruential.hpp>
#include "Timer.h"
#include <BoostGraphX/minimum_feedback_arc_set.h>

namespace bglx::test
{
	typedef boost::erdos_renyi_iterator<boost::minstd_rand, Dag> ERGen;
	TEST_CASE("")
	{
		boost::minstd_rand gen;
		// Create graph with 100 nodes and edges with probability 0.05
		Dag g(ERGen(gen, 32555, 0.00003034467), ERGen(), 100);
		//Dag g(ERGen(gen, 1000, 0.001), ERGen(), 100);

		//Dag g;
		/*
		for(int i = 0; i < 8; ++i)
		{
			boost::add_vertex(g);
		}
		boost::add_edge(0, 3, g);

		boost::add_edge(1, 3, g);
		boost::add_edge(1, 4, g);

		boost::add_edge(2, 4, g);

		boost::add_edge(3, 5, g);
		boost::add_edge(3, 6, g);
		boost::add_edge(3, 7, g);

		boost::add_edge(4, 6, g);

		boost::add_edge(5, 0, g);
		boost::add_edge(7, 2, g);
		boost::add_edge(4, 1, g);
*/
		auto nrEdges = boost::num_edges(g);
		auto nrVertices = boost::num_vertices(g);
		std::cout << "Nr edges " << nrEdges << std::endl;
		std::cout << "Nr vertices" << nrVertices << std::endl;
		std::unique_ptr<Minimum_Feedback_Arc_Set_Solver_Eades_Lin<Dag>> p_eades;
		{
			AutoProfiler timer("Time taken.");
			p_eades = std::make_unique<Minimum_Feedback_Arc_Set_Solver_Eades_Lin<Dag>>(g);
			std::cout << "Nr feedback edges: " << p_eades->feedback_arc_set().size() << std::endl;
		}
		auto& solver = *p_eades;
		REQUIRE(solver.topological_sorted_vertices().size() == nrVertices);
		std::unordered_map<Vertex, int> vertexOrder;
		int iV = 0;
		for (auto v : solver.topological_sorted_vertices())
		{
			if (vertexOrder.find(v) != vertexOrder.end())
			{
				throw std::exception("Error. Dup v.");
			}
			vertexOrder[v] = iV++;
		}

		auto nrFeedbackEdges = solver.feedback_arc_set().size();
		bool ifMatchHeuristicWorstCase = nrFeedbackEdges <= nrEdges / 2 - nrVertices / 6;
		REQUIRE(ifMatchHeuristicWorstCase);
		std::set<std::pair<Vertex, Vertex>> feedback_edges;
		for (auto [v1, v2] : solver.feedback_arc_set())
		{
			if (vertexOrder.at(v1) <= vertexOrder.at(v2))
			{
				throw std::exception("Not feedback.");
			}
			auto [it, ok] = feedback_edges.emplace(std::pair<Vertex, Vertex>{v1, v2});
			if (!ok)
			{
				throw std::exception("Feedback arc dup.");
			}
		}

		for (auto e : boost::make_iterator_range(boost::edges(g)))
		{
			auto src = boost::source(e, g);
			auto dst = boost::target(e, g);
			//if it is not feedback
			if (feedback_edges.find(std::pair<Vertex, Vertex>{src, dst}) == feedback_edges.end())
			{
				if (!(vertexOrder.at(src) < vertexOrder.at(dst)))
				{
					throw std::exception("Feedback edge not listed!");
				}
			}
		}
	}
}
