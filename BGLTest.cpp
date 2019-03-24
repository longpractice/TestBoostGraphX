// BGLTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING
#include "NamedDag.h"
#include "Timer.h"
#include <BoostGraphX/EulerGraph.h>
#include <iostream>
namespace bglx {
std::vector<std::string> expandString(const std::vector<std::string>& names)
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

std::pair<std::string, std::string> nextName(std::string name)
{
    name.erase(0, 1);
    return { name + "0", name + "1" };
}

Dag makeSequence()
{
    auto names = std::vector<std::string> { "0", "1" };
    for (int i = 0; i < 19; i++) {
        names = expandString(names);
    }
    std::cout << names.size() << std::endl;
    auto dag = Dag {};
    std::unordered_map<std::string, Vertex> nameToVertex;
    for (auto name : names) {
        auto v = boost::add_vertex(dag);
        nameToVertex[name] = v;
    }
    for (auto name : names) {
        auto v = nameToVertex[name];
        auto [nextName1, nextName2] = nextName(name);
        auto vNext1 = nameToVertex[nextName1];
        auto vNext2 = nameToVertex[nextName2];

        auto eName1 = name + "0";
        auto eName2 = name + "1";

        auto [e1, ok] = boost::add_edge(v, vNext1, dag);
        dag[e1].name = eName1;
        auto [e2, ok2] = boost::add_edge(v, vNext2, dag);
        dag[e2].name = eName2;
    }
    return dag;
}

Dag make_dag()
{
    auto dag = Dag {};
    std::unordered_map<std::string, Vertex> nameToVertex;
    //points

    std::vector<std::tuple<std::string, std::string, std::string>> edges = {
        { "000", "000", "e1" },
        //{ "000", "001", "e2" },

        { "100", "000", "e16" },

        { "100", "001", "e7" },

        { "001", "011", "e8" },
        { "001", "010", "e3" },
        { "010", "100", "e6" },
        { "110", "100", "e15" },

        { "010", "101", "e4" },
        { "101", "010", "e5" },

        { "101", "011", "e11" },
        { "110", "101", "e10" },

        { "011", "110", "e9" },

        { "011", "111", "e12" },
        { "111", "110", "e14" },

        { "111", "111", "e13" }
    };

    for (auto [source, target, name] : edges) {
        if (nameToVertex.find(source) == nameToVertex.end()) {
            auto v = boost::add_vertex(dag);
            nameToVertex[source] = v;
            dag[v].name = source;
        }
        if (nameToVertex.find(target) == nameToVertex.end()) {
            auto v = boost::add_vertex(dag);
            nameToVertex[target] = v;
            dag[v].name = target;
        }

        auto [edge, ok] = boost::add_edge(nameToVertex[source], nameToVertex[target], dag);
        dag[edge].name = name;
        assert(ok);
    }

    return dag;
}

std::unordered_map<std::string, Vertex>
vertexByName(const Dag& dag)
{
    std::unordered_map<std::string, Vertex> lookup;
    for (auto v : boost::make_iterator_range(boost::vertices(dag))) {
        auto name = dag[v].name;
        lookup[name] = v;
    }
    return lookup;
}

void printEulerLine(const std::list<Edge> edges, const Dag& dag)
{
    for (auto e : edges) {
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
    std::list<Edge> path;
    std::list<Edge> tour;
    {
        AutoProfiler timer { "time total" };
        //path = bglx::find_one_directed_euler_cycle(dag, *boost::vertices(dag).first);
        tour = bglx::find_one_directed_euler_tour(dag, nameToVertex["001"], nameToVertex["000"]);
    }

    printEulerLine(tour, dag);
    return 0;
}
