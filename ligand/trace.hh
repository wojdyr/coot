

#ifndef LIGAND_TRACE_HH 
#define LIGAND_TRACE_HH

#include <map>
#include <clipper/core/xmap.h>

#include "../mini-mol/mini-mol.hh"


namespace coot {


   // This should be inside trace (it's too generic a name)
   
   // This is a node in the map, i.e. there is a vector of these for each atom_index
   // 
   class scored_node_t {
   public:
      unsigned int atom_idx;
      double spin_score;
      double alpha; // angle at which spin_score is recorded
      scored_node_t(unsigned int idx_in, double score_in, double angle_in) {
	 atom_idx = idx_in;
	 spin_score = score_in;
	 alpha = angle_in;
      }
      scored_node_t() {}
      bool operator==(const scored_node_t &other) const
      { return (other.atom_idx == atom_idx); }
      static bool sort_scores(const scored_node_t &s1, const scored_node_t &s2) {
	 return (s2.spin_score < s1.spin_score);
      }
      static bool sort_pair_scores(const std::pair<unsigned int, scored_node_t> &s1, const std::pair<unsigned int, scored_node_t> &s2) {
	 return (s2.second.spin_score < s1.second.spin_score);
      }
   };

   
   class trace {

      clipper::Xmap<float> xmap; // a copy
      float rmsd_cut_off;
      float flood_atom_mask_radius;
      coot::minimol::molecule get_flood_molecule() const;
      std::vector<std::pair<unsigned int, unsigned int> >
      atom_pairs_within_distance(const minimol::molecule &flood_mol,
				 double trans_dist,
				 double trans_dist_variation);
      // this sets members atom_selection and n_selected_atoms (if mol).
      std::vector<std::pair<unsigned int, unsigned int> >
      atom_pairs_within_distance(mmdb::Manager *mol,
				 double trans_dist,
				 double trans_dist_variation);
      
      // fill the tr map with scores
      void spin_score_pairs(const std::vector<std::pair<unsigned int, unsigned int> > &apwd);
      
      std::pair<unsigned int, scored_node_t>
	 spin_score(unsigned int idx_1, unsigned int idx_2) const;

      // no longer use sas, because the indexing is strange.  We will
      // use pure mmdb mol and atom selection
      // 
      // minimol::molecule mol_for_sas;
      // std::vector<minimol::atom *> sas;

      mmdb::Manager *mol;
      mmdb::PAtom *atom_selection;
      int n_selected_atoms;
      int selhnd; // for atom_selection

      // presumes atom selection has been set
      clipper::Coord_orth index_to_pos(unsigned int idx) const {
	 return clipper::Coord_orth(atom_selection[idx]->x,
				    atom_selection[idx]->y,
				    atom_selection[idx]->z);
      }
      // presumes atom selection has been set
      std::string index_to_name(unsigned int idx) const {
	 return std::string(atom_selection[idx]->name);
      } 
      
      void trace_graph();
      std::map<unsigned int, std::vector<scored_node_t> > tr; // which atoms are connected to which other atoms
                                                              // backwards and forwards

      void
      next_vertex(const std::vector<scored_node_t> &path,
		  unsigned int depth, scored_node_t this_scored_vertex);

      std::vector<scored_node_t> get_neighbours_of_vertex_excluding_path(unsigned int this_vertex,
									const std::vector<scored_node_t> &path);
      void print_tree(const std::vector<unsigned int> &path) const;

      // accumlate interesting trees here
      std::vector<std::vector<scored_node_t> > interesting_trees;
      // 
      void add_tree_maybe(const std::vector<scored_node_t> &path);
      double path_candidate_angle(const std::vector<scored_node_t> &path,
				  unsigned int candidate_vertex) const;
      void print_interesting_trees() const;
      void sort_filter_interesting_trees(); // long trees to the top

      static bool sort_trees_by_length(const std::vector<scored_node_t> &v1,
				       const std::vector<scored_node_t> &v2) {
	 return (v1.size() > v2.size());
      }

      bool add_atom_names_in_map_output;; 

   public:
      trace(const clipper::Xmap<float> &xmap_in);
      void set_atom_mask_radius(float r) { flood_atom_mask_radius = r; }
      void action();

      // testing
      void test_model(mmdb::Manager *mol);

      ~trace() {
	 // crash.  Why?
	 //  if (atom_selection)
	 // mol->DeleteSelection(selhnd);
      }

   };

   class trace_path_eraser {
   public:
      std::vector<scored_node_t> lp;
      unsigned int crit_for_match;
      trace_path_eraser(const std::vector<scored_node_t> &long_path,
			unsigned int crit_for_match_in) {
	 lp = long_path;
	 crit_for_match = crit_for_match_in;
      }
      bool operator() (const std::vector<scored_node_t> &interesting_path) const {
	 bool r = false;
	 unsigned int n_match = 0;
	 for (unsigned int i=0; i<interesting_path.size(); i++) {
	    for (unsigned int j=0; j<lp.size(); j++) { 
	       if (lp[j] == interesting_path[i]) {
		  n_match++;
	       }
	    }
	    if (n_match >= crit_for_match) {
	       r = true;
	       break;
	    }
	    if (n_match >= (interesting_path.size() -1)) {
	       r = true;
	       break;
	    }
	 }
	 return r;
      }
   };
}

#endif // LIGAND_TRACE_HH

