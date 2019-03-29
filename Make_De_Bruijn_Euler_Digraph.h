#pragma once
#include "NamedDag.h"
#include <Boost/timer/timer.hpp>
#include <BoostGraphX/euler_graph.h>
#include <iostream>
#include <catch.hpp>

namespace bglx::test
{
	static std::vector<std::string> expandString(const std::vector<std::string>& names)
	{
		auto ch1 = '0';
		auto ch2 = '1';
		std::vector<std::string> expandedNames;
		expandedNames.reserve(names.size() * 2);
		for (auto name : names) {
			expandedNames.emplace_back(name + ch1);
			expandedNames.emplace_back(name + ch2);
		}
		return expandedNames;
	}

	static std::pair<std::string, std::string> nextName(std::string name)
	{
		name.erase(0, 1);
		return { name + "0", name + "1" };
	}

	static Dag make_De_Bruigin_graph(int order)
	{
		auto names = std::vector<std::string>{ "0", "1" };
		for (int i = 0; i < order; i++) {
			names = expandString(names);
		}
		std::cout << names.size() << std::endl;
		auto dag = test::Dag{};
		std::unordered_map<std::string, test::Vertex> nameToVertex;
		for (auto name : names) {
			auto v = boost::add_vertex(dag);
			dag[v].name = name;
			nameToVertex[name] = v;
		}
		for (auto name : names) {
			auto v = nameToVertex[name];
			auto[nextName1, nextName2] = nextName(name);
			auto vNext1 = nameToVertex[nextName1];
			auto vNext2 = nameToVertex[nextName2];

			auto eName1 = name + "0";
			auto eName2 = name + "1";

			auto[e1, ok] = boost::add_edge(v, vNext1, dag);
			dag[e1].name = eName1;
			auto[e2, ok2] = boost::add_edge(v, vNext2, dag);
			dag[e2].name = eName2;
		}
		return dag;
	}
}