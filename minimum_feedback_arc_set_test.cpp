#include "NamedDag.h"
#include <optional>
#include <boost/graph/copy.hpp>
#include <stack>
#include "catch.hpp"

namespace bglx::test
{
	struct G_Eades
	{
		//just to break the dependency cycle
		using G_Pure = boost::adjacency_list<boost::listS, boost::listS, boost::bidirectionalS>;

		using V_Pure = G_Pure::vertex_descriptor;
		using V_List_Pure_It = std::list<V_Pure>::iterator;

		struct V_Prop
		{
			Vertex v_origin;
			V_List_Pure_It v_list_it;
			int deg_diff{};
		};

		struct E_Prop
		{
			Edge e_origin;
		};

		using G = boost::adjacency_list<boost::listS, boost::listS, boost::bidirectionalS, V_Prop, E_Prop>;

		using V = G::vertex_descriptor;
		using E = G::edge_descriptor;

		using V_List = std::list<V>;
		using V_Res_List = std::list<Vertex>;
		using E_Res_List = std::list<Edge>;

		struct Bin
		{
			V_List v_list;
			Bin* p_prev_occupied_bin;
			Bin* p_next_occupied_bin;
		};

		Bin* p_last_occupied_bin{nullptr};

		using Bins = std::vector<Bin>;

		G g;
		Bins bins;

		//max_out_deg + max_in_deg would be maximum number of bins
		//out_deg - in_deg is within range of -max_in_deg and max_out_deg
		int max_out_deg;
		int max_in_deg;

		V_Res_List s1;
		V_Res_List s2;

		std::queue<V> unprocessed_sources;
		std::stack<V> unprocessed_sinks;

		E_Res_List feedback_edges_res;


		void init_v_to_bin(V v)
		{
			auto deg_in = boost::in_degree(v, g);
			auto deg_out = boost::out_degree(v, g);

			if (deg_in == 0)
			{
				//source
				unprocessed_sources.push(v);
			}
			else if (deg_out == 0)
			{
				//sink
				unprocessed_sinks.push(v);
			}
			else
			{
				int deg_diff = static_cast<int>(deg_out) - static_cast<int>(deg_in);
				auto& bin = get_bin(deg_diff);
				bin.v_list.emplace_back(v);

				auto& vProp = g[v];
				vProp.deg_diff = deg_diff;
				vProp.v_list_it = std::prev(bin.v_list.end());
			}
		}


		void init_occupied_bin_list_prev()
		{
			Bin* current_last_occupied_bin = nullptr;
			for (auto& bin : bins)
			{
				if (bin.v_list.empty())
				{
					continue;
				}
				bin.p_prev_occupied_bin = current_last_occupied_bin;
				current_last_occupied_bin = &bin;
			}
			p_last_occupied_bin = current_last_occupied_bin;
		}

		void init_occupied_bin_list_next()
		{
			Bin* current_first_occupied_bin = nullptr;
			for (auto itBin = bins.rbegin(); itBin != bins.rend(); ++itBin)
			{
				auto& bin = *itBin;
				if (bin.v_list.empty())
				{
					continue;
				}
				bin.p_next_occupied_bin = current_first_occupied_bin;
				current_first_occupied_bin = &bin;
			}
		}

		void init(const Dag& g_origin)
		{
			auto max_in_degree = std::numeric_limits<int>::min();
			auto max_out_degree = std::numeric_limits<int>::min();
			auto v_copier = [&](const Vertex v_origin, const V v)
			{
				g[v].v_origin = v_origin;
			};

			auto e_copier = [&](const Edge& e_origin, const E& e)
			{
				g[e].e_origin = e_origin;
			};
			boost::copy_graph(g_origin, g, boost::vertex_copy(v_copier).edge_copy(e_copier));
			for (auto v : boost::make_iterator_range(boost::vertices(g)))
			{
				auto in_deg = static_cast<int>(boost::in_degree(v, g));
				auto out_deg = static_cast<int>(boost::out_degree(v, g));
				if (in_deg > max_in_degree)
				{
					max_in_degree = in_deg;
				}
				if (out_deg > max_out_degree)
				{
					max_out_degree = out_deg;
				}
			}
			max_out_deg = max_out_degree;
			max_in_deg = max_in_degree;
			bins.resize(max_in_deg + max_in_deg + 1);
			for (auto v : boost::make_iterator_range(boost::vertices(g)))
			{
				init_v_to_bin(v);
			}

			init_occupied_bin_list_prev();
			init_occupied_bin_list_next();
		}

		void process_sources()
		{
			while(!unprocessed_sources.empty())
			{
				auto src = unprocessed_sources.front();
				unprocessed_sources.pop();
				for(auto tgt : boost::make_iterator_range(boost::adjacent_vertices(src, g)))
				{
					decrease_in_degree_on_v(tgt);
				}
				add_to_s1(src);
				boost::clear_vertex(src, g);
				boost::remove_vertex(src, g);
			}
		}

		void process_sinks()
		{
			while(!unprocessed_sinks.empty())
			{
				auto tgt = unprocessed_sinks.top();
				unprocessed_sinks.pop();
				for(auto src : boost::make_iterator_range(boost::inv_adjacent_vertices(tgt, g)))
				{
					decrease_out_degree_on_v(src);
				}
				add_to_s2(tgt);
				boost::clear_vertex(tgt, g);
				boost::remove_vertex(tgt, g);
			}
		}

		void process_max_delta_vertex(V v)
		{
			auto deg_diff = g[v].deg_diff;
			auto& bin = get_bin(deg_diff);
			bin.v_list.erase(g[v].v_list_it);
			for (auto tgt : boost::make_iterator_range(boost::adjacent_vertices(v, g)))
			{
				decrease_in_degree_on_v(tgt);
			}
			for(const auto& e : boost::make_iterator_range(boost::in_edges(v, g)))
			{
				//these are the feedback arcs
				feedback_edges_res.emplace_back(g[e].e_origin);
				auto src = boost::source(e, g);
				decrease_out_degree_on_v(src);
			}
			add_to_s1(v);

			if (bin.v_list.empty())
			{
				handle_emptied_bin(bin);
			}

			boost::clear_vertex(v, g);
			boost::remove_vertex(v, g);
		}

		void process()
		{
			if(!p_last_occupied_bin)
			{
				//empty graph
				return;
			}

			while(boost::num_vertices(g) != 0)
			{
				while(!(unprocessed_sources.empty() && unprocessed_sinks.empty()))
				{
					process_sources();
					process_sinks();
				}
				if(p_last_occupied_bin)
				{
					auto v = p_last_occupied_bin->v_list.back();
					process_max_delta_vertex(v);
				}
			}
		}


		int get_bin_id(int deg_diff)
		{
			return deg_diff + max_in_deg;
		}

		Bin& get_bin(int deg_diff)
		{
			auto id = get_bin_id(deg_diff);
			return bins[id];
		}


		//non-sinks: emplace to the back of s1
		void add_to_s1(V v)
		{
			//new sources are attached to the back
			s1.emplace_back(g[v].v_origin);
		}

		//sinks: emplace to the front of s2
		void add_to_s2(V v)
		{
			//new sinks are attached to the front
			s2.emplace_front(g[v].v_origin);
		}


		void handle_emptied_bin(Bin& bin)
		{
			//this bin has become empty, link the two neighbors of the linked list
			if (bin.p_prev_occupied_bin)
			{
				bin.p_prev_occupied_bin->p_next_occupied_bin = bin.p_next_occupied_bin;
			}

			if (bin.p_next_occupied_bin)
			{
				bin.p_next_occupied_bin->p_prev_occupied_bin = bin.p_prev_occupied_bin;
			}
			else
			{
				p_last_occupied_bin = bin.p_prev_occupied_bin;
			}
		}

		//this is due to the one of the in-edges is removed
		void handle_newly_occupied_bin_downgrade_v(Bin& bin, Bin& next_occupied_bin)
		{
			//
			// From linked list A<--->B to A <---> C <---> B
			// Two tasks:
			//
			// 1) set the next and prev of C
			// 2) set the next of A and the prev of B
			//
			bin.p_next_occupied_bin = &next_occupied_bin;
			bin.p_prev_occupied_bin = next_occupied_bin.p_prev_occupied_bin;

			if (next_occupied_bin.p_prev_occupied_bin)
			{
				next_occupied_bin.p_prev_occupied_bin->p_next_occupied_bin = &bin;
			}
			next_occupied_bin.p_prev_occupied_bin = &bin;
		}

		void handle_newly_occupied_bin_upgrade_v(Bin& bin, Bin& prev_occupied_bin)
		{
			bin.p_next_occupied_bin = prev_occupied_bin.p_next_occupied_bin;
			bin.p_prev_occupied_bin = &prev_occupied_bin;
			if (prev_occupied_bin.p_next_occupied_bin)
			{
				prev_occupied_bin.p_next_occupied_bin->p_prev_occupied_bin = &bin;
			}
			else
			{
				p_last_occupied_bin = &bin;
			}
			prev_occupied_bin.p_next_occupied_bin = &bin;
		}


		//if return true, the caller needs to remove the vertex from graph also
		bool decrease_in_degree_on_v(V v)
		{
			auto& v_prop = g[v];
			auto old_deg_diff = v_prop.deg_diff;
			auto new_deg_diff = old_deg_diff + 1;
			v_prop.deg_diff = new_deg_diff;
			auto old_in_deg = boost::in_degree(v, g);
			auto old_out_deg = boost::out_degree(v, g);

			if(old_out_deg == 0)
			{
				//treat as sinks
				return false;
			}

			auto& old_bin = get_bin(old_deg_diff);
			//transfer the vertex to his new home, s1 or new bin
			if (old_in_deg == 1)
			{
				//becomes a source
				old_bin.v_list.erase(v_prop.v_list_it);
				unprocessed_sources.push(v);
			}
			else
			{
				//upgrade to another bin
				auto& new_bin = get_bin(new_deg_diff);
				bool was_new_bin_empty = new_bin.v_list.empty();
				//splice from old bin to new bin, which maintains the iterator validation
				new_bin.v_list.splice(new_bin.v_list.end(), old_bin.v_list, v_prop.v_list_it);
				if (was_new_bin_empty)
				{
					handle_newly_occupied_bin_upgrade_v(new_bin, old_bin);
				}
			}
			if (old_bin.v_list.empty())
			{
				handle_emptied_bin(old_bin);
			}

			return old_in_deg == 1;
		}

		//if return true, the caller need to remove the vertex from the graph
		bool decrease_out_degree_on_v(V v)
		{
			auto& v_prop = g[v];
			auto old_deg_diff = v_prop.deg_diff;
			auto new_deg_diff = old_deg_diff - 1;
			v_prop.deg_diff = new_deg_diff;
			auto old_deg = boost::out_degree(v, g);
			auto& old_bin = get_bin(old_deg_diff);

			//transfer the vertex to his new home, s2 or new bin
			if (old_deg == 1)
			{
				//becomes a sink
				old_bin.v_list.erase(v_prop.v_list_it);
				unprocessed_sinks.push(v);
			}
			else
			{
				//downgrade the vertex to lower bin
				auto& new_bin = get_bin(new_deg_diff);
				bool was_new_bin_empty = new_bin.v_list.empty();
				new_bin.v_list.splice(new_bin.v_list.end(), old_bin.v_list, v_prop.v_list_it);
				if (was_new_bin_empty)
				{
					handle_newly_occupied_bin_downgrade_v(new_bin, old_bin);
				}
			}
			if (old_bin.v_list.empty())
			{
				handle_emptied_bin(old_bin);
			}
			return old_deg == 1;
		}
	};

	TEST_CASE("")
	{
		Dag g;
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

		
		G_Eades solver;
		solver.init(g);
		solver.process();
	}
}
