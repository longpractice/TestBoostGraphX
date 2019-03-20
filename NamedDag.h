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
}

//a closed walk travels one edge only once(times of travelling cross an edge is not limited)
//the start terminal vertex and the end terminal vertex is the same
struct ClosedWalkInfo {
    std::vector<Edge> eList;
    std::set<Edge> eSet;
};
}

namespace std {
template <>
struct less<bglx::ClosedWalkInfo> {
    bool operator()(const bglx::ClosedWalkInfo& lhs, const bglx::ClosedWalkInfo& rhs) const
    {
        return lhs.eList < rhs.eList;
    }
};
}

namespace bglx {

// pathInfo is a partial path starting from vStart and end at vStart
// it is not yet a complete path and we need to add more edges to this path and in the process
// we create more paths
// The returned might still partial, we need to recall this method until all the edges are found
std::set<ClosedWalkInfo> discoverNewClosedWalks(const Dag& dag, const ClosedWalkInfo& closedWalkInfo, Vertex vStart)
{
    //edge list in the original closed walk
    const auto& eList = closedWalkInfo.eList;
    const auto& eSet = closedWalkInfo.eSet;

    auto oldPathInfo = closedWalkInfo;

    std::set<ClosedWalkInfo> newClosedWalks;

    Vertex vLast;
    if (!eList.empty()) {
        //get the last edge
        auto eLast = *(eList.end() - 1);
        //find the last vertex
        vLast = boost::target(eLast, dag);

    } else {
        vLast = vStart;
    }

    //traverse
    for (auto eOut : Range { boost::out_edges(vLast, dag) }) {
        //no edge should be repeated
        if (eSet.find(eOut) == eSet.end()) {
            auto newPathInfo = closedWalkInfo;
            newPathInfo.eSet.insert(eOut);
            newPathInfo.eList.push_back(eOut);
            newClosedWalks.insert(newPathInfo);
        }
    }

    if (newClosedWalks.empty())
        return { oldPathInfo };
    return newClosedWalks;
}

static std::set<ClosedWalkInfo> findAllEulerPaths(const Dag& dag, Vertex vStart)
{
    std::set<ClosedWalkInfo> paths = discoverNewClosedWalks(dag, {}, vStart);
    std::set<ClosedWalkInfo> wellMadePaths;
    while (!paths.empty()) {
        auto itFirstPath = paths.begin();
        auto generatedPaths = discoverNewClosedWalks(dag, *itFirstPath, vStart);

        if (generatedPaths.size() == 1 && generatedPaths.begin()->eList == itFirstPath->eList) {
            //nothing is changed, we have reached the end
            wellMadePaths.insert(*itFirstPath);
            paths.erase(itFirstPath);
        } else {
            //we have not reached the end
            paths.erase(*itFirstPath);
            for (auto info : generatedPaths) {
                paths.insert(info);
            }
        }
    }

    auto fullPaths = std::set<ClosedWalkInfo> {};
    auto nrEdges = boost::num_edges(dag);

    for (auto [eList, eSet] : wellMadePaths) {
        if (eList.size() == nrEdges)
            fullPaths.insert({ eList, eSet });
    }

    return fullPaths;
}

}
