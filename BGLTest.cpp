// BGLTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <iostream>
#include "NamedDag.h"

namespace bglx {
	Dag make_dag()
	{
		auto dag = Dag{};
		std::unordered_map <std::string, Vertex> nameToVertex;
		//points

		std::vector<std::tuple<std::string, std::string, std::string>> edges =
		{
			{"000", "000", "e1"},

			{"000", "001", "e2"},
			{"100", "000", "e16"},

			{"100", "001", "e7"},

			{"001", "011", "e8"},
			{"001", "010", "e3"},
			{"010", "100", "e6"},
			{"110", "100", "e15"},

			{"010", "101", "e4"},
			{"101", "010", "e5"},

			{"101", "011", "e11"},
			{"110", "101", "e10"},

			{"011", "110", "e9"},

			{"011", "111", "e12"},
			{"111", "110", "e14"},

			{"111", "111", "e13"}
		};

		for (auto[source, target, name] : edges)
		{
			if (nameToVertex.find(source) == nameToVertex.end())
			{
				auto v = boost::add_vertex(dag);
				nameToVertex[source] = v;
				dag[v].name = source;
			}
			if (nameToVertex.find(target) == nameToVertex.end())
			{
				auto v = boost::add_vertex(dag);
				nameToVertex[target] = v;
				dag[v].name = target;
			}

			auto[edge, ok] = boost::add_edge(nameToVertex[source], nameToVertex[target], dag);
			dag[edge].name = name;
			assert(ok);
		}

		return dag;
	}

	std::unordered_map<std::string, Vertex>
		vertexByName(const Dag& dag)
	{
		std::unordered_map<std::string, Vertex> lookup;
		for (auto v : Range{ boost::vertices(dag) })
		{
			auto name = dag[v].name;
			lookup[name] = v;
		}
		return lookup;
	}

	void printEulerLine(const std::vector<Edge> edges, const Dag& dag)
	{
		for (auto e : edges)
		{
			std::cout << dag[e].name << " -> ";
		}
		std::cout << std::endl;
	}



}


int main()
{
	using namespace bglx;
	std::cout << "Hello World!\n";
	auto dag = bglx::make_dag();
	auto nameToVertex = bglx::vertexByName(dag);

	auto path = findEulerCycle_Hierholzer(dag, nameToVertex["000"]);
	printEulerLine(path, dag);
	return 0;
}
