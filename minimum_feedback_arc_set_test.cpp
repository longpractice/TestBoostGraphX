﻿#include "NamedDag.h"
#include <optional>

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
			int deg_diff{};
			//we could not put default constructed it here, since such a thing is a "singular iterator"
			//which could not be assigned
			//we could only have a work around to use the memory to avoid any additional memory cost
			std::aligned_storage<sizeof(V_List_Pure_It), alignof(V_List_Pure_It)> v_list_it_storage;
			void assign_v_list_it(V_List_Pure_It it)
			{
				::new ((void *)::std::addressof(v_list_it_storage)) V_List_Pure_It( it );
			}
			V_List_Pure_It v_list_it()
			{
				return *reinterpret_cast<V_List_Pure_It*>((void*)std::addressof(v_list_it_storage));
			}
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
			Bin* prev_occupied_bin;
			Bin* next_occupied_bin;
		};

		Bin* last_occupied_bin{nullptr};

		using Bins = std::vector<Bin>;

		G g;
		Bins bins;

		//max_out_deg + max_in_deg would be maximum number of bins
		//out_deg - in_deg is within range of -max_in_deg and max_out_deg
		int max_out_deg;
		int max_in_deg;

		V_Res_List s1;
		V_Res_List s2;
		E_Res_List feedback_edges_res;


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

		void init_v_to_bin(V v)
		{
			auto deg_in = boost::in_degree(v, g);
			auto deg_out = boost::out_degree(v, g);
			int deg_diff = static_cast<int>(deg_out) - static_cast<int>(deg_in);
			auto& bin = get_bin(deg_diff);
			bin.v_list.emplace_back(v);

			auto& vProp = g[v];
			vProp.deg_diff = deg_diff;
			vProp.assign_v_list_it(std::prev(bin.v_list.end()));
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
				bin.prev_occupied_bin = current_last_occupied_bin;
				current_last_occupied_bin = &bin;
			}
			last_occupied_bin = current_last_occupied_bin;
		}

		void int_occupied_bin_list_next()
		{
			Bin* current_last_occupied_bin = nullptr;
			for (auto itBin = bins.rbegin(); itBin != bins.rend(); ++itBin)
			{
				auto& bin = *itBin;
				if (bin.v_list.empty())
				{
					continue;
				}
				bin.next_occupied_bin = current_last_occupied_bin;
				current_last_occupied_bin = &bin;
			}
		}

		void handle_emptied_bin(Bin& bin)
		{
			//this bin has become empty, link the two neighbors of the linked list
			if (bin.prev_occupied_bin)
			{
				bin.prev_occupied_bin->next_occupied_bin = bin.next_occupied_bin;
			}

			if (bin.next_occupied_bin)
			{
				bin.next_occupied_bin->prev_occupied_bin = bin.prev_occupied_bin;
			}
			else
			{
				last_occupied_bin = bin.prev_occupied_bin;
			}
		}

		void handle_newly_occupied_bin_downgrade_v(Bin& bin, Bin& next_occupied_bin)
		{
			//From linked list A<--->B to A <---> C <---> B
			//two tasks, 1)set the next and prev of C
			//2) set the next of A and the prev of B
			bin.next_occupied_bin = &next_occupied_bin;
			bin.prev_occupied_bin = next_occupied_bin.prev_occupied_bin;

			if (next_occupied_bin.prev_occupied_bin)
			{
				next_occupied_bin.prev_occupied_bin->next_occupied_bin = &bin;
			}
			next_occupied_bin.prev_occupied_bin = &bin;
		}

		void handle_newly_occupied_bin_upgrade_v(Bin& bin, Bin& prev_occupied_bin)
		{
			bin.next_occupied_bin = prev_occupied_bin.next_occupied_bin;
			bin.prev_occupied_bin = &prev_occupied_bin;
			if (prev_occupied_bin.next_occupied_bin)
			{
				prev_occupied_bin.next_occupied_bin->prev_occupied_bin = &bin;
			}
			else
			{
				last_occupied_bin = &bin;
			}
			prev_occupied_bin.next_occupied_bin = &bin;
		}


		//if return tree, the user got to remove the vertex from graph also
		bool decrease_in_degree_on_v(V v)
		{
			auto& vProp = g[v];
			auto old_deg_diff = vProp.deg_diff;
			auto new_deg_diff = old_deg_diff + 1;
			auto old_deg = boost::in_degree(v, g);
			auto old_bin = get_bin(vProp.deg_diff);

			//becomes a source
			//transfer the vertex to his new home, s1 or new bin
			if (old_deg == 1)
			{
				old_bin.v_list.erase(vProp.v_list_it());
				add_to_s1(v);
			}
			else
			{
				auto& new_bin = get_bin(new_deg_diff);
				bool was_new_bin_empty = new_bin.v_list.empty();
				new_bin.v_list.splice(new_bin.v_list.end(), old_bin.v_list, vProp.v_list_it());
				if (was_new_bin_empty)
				{
					handle_newly_occupied_bin_upgrade_v(new_bin, old_bin);
				}
			}
			if (old_bin.v_list.empty())
			{
				handle_emptied_bin(old_bin);
			}
			return old_deg == 1;
		}

		bool decrease_out_degree_on_v(V v)
		{
			auto& vProp = g[v];
			auto old_deg_diff = vProp.deg_diff;
			auto new_deg_diff = old_deg_diff - 1;
			auto old_deg = boost::out_degree(v, g);
			auto old_bin = get_bin(vProp.deg_diff);

			//becomes a source
			//transfer the vertex to his new home, s2 or new bin
			if (old_deg == 1)
			{
				old_bin.v_list.erase(vProp.v_list_it());
				add_to_s2(v);
			}
			else
			{
				auto& new_bin = get_bin(new_deg_diff);
				bool was_new_bin_empty = new_bin.v_list.empty();
				new_bin.v_list.splice(new_bin.v_list.end(), old_bin.v_list, vProp.v_list_it());
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
}