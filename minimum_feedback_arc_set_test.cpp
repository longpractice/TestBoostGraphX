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
	TEST_CASE("MFAS: Edge-case: empty graph")
	{
		Dag g;
		Minimum_Feedback_Arc_Set_Solver_Eades_Lin<Dag> solver(g);
		REQUIRE(solver.feedback_arc_set().empty());
		REQUIRE(solver.topological_sorted_vertices().empty());
	}

	TEST_CASE("MFAS: Edge-case: isolated vertices")
	{
		SECTION("Single vertex")
		{
			Dag g;
			auto v = boost::add_vertex(g);
			Minimum_Feedback_Arc_Set_Solver_Eades_Lin<Dag> solver(g);
			REQUIRE(solver.feedback_arc_set().empty());
			REQUIRE(solver.topological_sorted_vertices().front() == v);
		}

		SECTION("Multiple vertices")
		{
			Dag g;
			auto v0 = boost::add_vertex(g);
			auto v1 = boost::add_vertex(g);
			auto v2 = boost::add_vertex(g);

			Minimum_Feedback_Arc_Set_Solver_Eades_Lin<Dag> solver(g);
			REQUIRE(solver.feedback_arc_set().empty());
			REQUIRE(solver.topological_sorted_vertices().size() == 3);

			std::unordered_set<Vertex> vertices(solver.topological_sorted_vertices().begin(),
			                                    solver.topological_sorted_vertices().end());
			REQUIRE(vertices == std::unordered_set<Vertex>{ v0, v1, v2 });
		}
	}

	TEST_CASE("MFAS:property map")
	{
		Dag_VSet dag;
		auto v0 = boost::add_vertex(dag);
		auto v1 = boost::add_vertex(dag);
		auto v2 = boost::add_vertex(dag);
		boost::add_edge(v1, v2, dag);
		boost::add_edge(v2, v0, dag);

		std::unordered_map<Dag_VSet::vertex_descriptor, size_t> vMap =
		{
			{v0, 1},
			{v1, 2},
			{v2, 0}
		};

		auto iMap = boost::associative_property_map<
			std::unordered_map<Dag_VSet::vertex_descriptor, size_t>
		>(vMap);

		Minimum_Feedback_Arc_Set_Solver_Eades_Lin<Dag_VSet> solver(dag, iMap);
		REQUIRE(solver.feedback_arc_set().empty());
		REQUIRE(
			solver.topological_sorted_vertices()
			== std::list<Dag_VSet::vertex_descriptor>{v1, v2, v0}
		);
	}

	TEST_CASE("MFAS: Simple graph")
	{
		Dag g;
		auto v0 = boost::add_vertex(g);
		auto v1 = boost::add_vertex(g);
		auto v2 = boost::add_vertex(g);

		SECTION("Parallel edges")
		{
			boost::add_edge(v1, v0, g);
			boost::add_edge(v1, v0, g);
			boost::add_edge(v2, v1, g);

			Minimum_Feedback_Arc_Set_Solver_Eades_Lin<Dag> solver(g);
			REQUIRE(solver.feedback_arc_set().empty());
			REQUIRE(solver.topological_sorted_vertices() == std::list<Vertex>{v2, v1, v0});
		}

		SECTION("Parallel edges, with feedback")
		{
			boost::add_edge(v1, v0, g);
			boost::add_edge(v1, v0, g);
			boost::add_edge(v2, v1, g);
			boost::add_edge(v0, v2, g);

			Minimum_Feedback_Arc_Set_Solver_Eades_Lin<Dag> solver(g);
			REQUIRE(solver.feedback_arc_set().size() == 1);
			REQUIRE(solver.topological_sorted_vertices().size() == 3);
		}
	}


	TEST_CASE("MFAS: Erdos-Renyi-model test")
	{
		typedef boost::erdos_renyi_iterator<boost::minstd_rand, Dag> ERGen;
		boost::minstd_rand gen;
		// Create graph with 300000 nodes and edges with probability 0.00003
		Dag g(ERGen(gen, 300000, 0.00003), ERGen(), 100);
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
			//erdos-renyi model does not introduce any parallel edges
			if (vertexOrder.at(v1) <= vertexOrder.at(v2))
			{
				throw std::exception("Not feedback.");
			}
			auto [it, ok] = feedback_edges.emplace(std::pair<Vertex, Vertex>{v1, v2});
			if (!ok)
			{
				throw std::exception("Feedback arc dup for non-parallel edge Erdos-Renyi model.");
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
