// BGLTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
#define CATCH_CONFIG_MAIN
#define _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING
#include "NamedDag.h"
#include <Boost/timer/timer.hpp>
#include <BoostGraphX/euler_graph.h>
#include <iostream>
#include <catch.hpp>
#include "Make_De_Bruijn_Euler_Digraph.h"
#include "Timer.h"
namespace bglx::test {
	std::unordered_map<std::string, test::Vertex>
		vertex_by_name(const test::Dag& dag)
	{
		std::unordered_map<std::string, test::Vertex> lookup;
		for (auto v : boost::make_iterator_range(boost::vertices(dag))) {
			auto name = dag[v].name;
			lookup[name] = v;
		}
		return lookup;
	}

	void printEulerLine(const std::list<test::Edge> edges, const test::Dag& dag)
	{
		for (auto e : edges) {
			std::cout << dag[e].name << " -> ";
		}
		std::cout << std::endl;
	}

	//tesing a euler cycle or a tour, for a cycle(use start_v the same as last_v)
	void verify_euler_cycle_tour(std::list<Edge> cycle, Vertex start_v, Vertex last_v, const Dag& dag)
	{
		//for a circle, we require that the number of edges covers all the edges
		if (cycle.size() != boost::num_edges(dag))
			throw std::exception("Number of the edges miss match.");

		if (boost::source(*cycle.begin(), dag) != start_v)
			throw std::exception("The cycle does not start from the starting vertex.");

		std::set<Edge> visited_edges;
		//the last one is the prev of the first one
		auto prev_v = start_v;
		for (auto e : cycle)
		{
			if (visited_edges.find(e) != visited_edges.end())
				throw std::exception("Edge revisited.");
			visited_edges.insert(e);
			auto start = boost::source(e, dag);
			if (start != prev_v)
				throw std::exception("Uncontinuous cycle");
			prev_v = boost::target(e, dag);
		}

		auto last_e = cycle.back();
		if (boost::target(last_e, dag) != last_v)
			throw std::exception("Not going back to the first vertex.");
	}

	//a directed acyclic type with the vertex container as a set, and it does not have vertex index by default
	//we use this to test our methods accepting a property map



	TEST_CASE("Euler cycle")
	{
		SECTION("Euler Cycle/tour Simple")
		{
			auto dag = Dag{};
			auto v1 = boost::add_vertex(dag);
			auto cycle = bglx::find_one_directed_euler_cycle_hierholzer(
				dag, v1
			);
			REQUIRE(cycle.empty());


			auto v2 = boost::add_vertex(dag);
			auto[e, ok] = boost::add_edge(v1, v2, dag);
			auto tour = bglx::find_one_directed_euler_trail_hierholzer(
				dag, v1, v2
			);
			bool tourRight = tour == std::list<Edge>{e};
			REQUIRE(tourRight);

		}

		SECTION("Euler Cycle De Bruigin")
		{
			//65536 vertices and 131072 edges
			auto dag = make_De_Bruigin_graph(19);
			std::cout << "Nr vertices" << boost::num_vertices(dag) << std::endl;
			std::list<Edge> cycle;
			auto start_v = *boost::vertices(dag).first;
			{
				AutoProfiler timer{ "Euler cycle takes time: " };// boost::timer::cpu_timer timer{};
				cycle = bglx::find_one_directed_euler_cycle_hierholzer(
					dag, start_v
				);
			}
			verify_euler_cycle_tour(cycle, start_v, start_v, dag);
		}

		SECTION("Euler Tour De Bruigin")
		{
			auto dag = make_De_Bruigin_graph(3);
			{
				auto name_vtable = vertex_by_name(dag);
				auto v_start = name_vtable["0110"];
				auto v_last = name_vtable["0011"];
				auto[e, found] = boost::edge(v_last, v_start, dag);
				REQUIRE(found);
				boost::remove_edge(e, dag);
			}

			//the vertex descriptors got disabled after erasing edges
			{
				auto name_vtable = vertex_by_name(dag);
				auto v_start = name_vtable["0110"];
				auto v_last = name_vtable["0011"];
				auto tour = bglx::find_one_directed_euler_trail_hierholzer(
					dag, v_start, v_last
				);
				verify_euler_cycle_tour(tour, v_start, v_last, dag);
			}
		}

		SECTION("Euler cycle with property map")
		{
			Dag_VSet dag;
			auto v0 = boost::add_vertex(dag);
			auto v1 = boost::add_vertex(dag);
			auto v2 = boost::add_vertex(dag);
			auto[e01, okO1] = boost::add_edge(v0, v1, dag);
			auto[e12, ok12] = boost::add_edge(v1, v2, dag);
			auto[e20, ok20] = boost::add_edge(v2, v0, dag);

			std::unordered_map<Dag_VSet::vertex_descriptor, size_t> vMap =
			{
				{v0, 1},
				{v1, 2},
				{v2, 0}
			};

			auto iMap = boost::associative_property_map<
				std::unordered_map<Dag_VSet::vertex_descriptor, size_t>
			>(vMap);

			auto cycle = bglx::find_one_directed_euler_cycle_hierholzer(dag, v1, iMap);
			auto cycleRight = cycle == std::list<Dag_VSet::edge_descriptor>{e12, e20, e01};
			REQUIRE(cycleRight);
		}

		SECTION("Euler tour with property map")
		{
			Dag_VSet dag;
			auto v0 = boost::add_vertex(dag);
			auto v1 = boost::add_vertex(dag);
			auto v2 = boost::add_vertex(dag);
			auto[e12, ok12] = boost::add_edge(v1, v2, dag);
			auto[e20, ok20] = boost::add_edge(v2, v0, dag);

			std::unordered_map<Dag_VSet::vertex_descriptor, size_t> vMap =
			{
				{v0, 1},
				{v1, 2},
				{v2, 0}
			};

			auto iMap = boost::associative_property_map<
				std::unordered_map<Dag_VSet::vertex_descriptor, size_t>
			>(vMap);

			auto cycle = 
				bglx::find_one_directed_euler_trail_hierholzer(dag, v1, v0, iMap);
			auto cycleRight = cycle == std::list<Dag_VSet::edge_descriptor>{e12, e20};
			REQUIRE(cycleRight);
		}


	}
}
