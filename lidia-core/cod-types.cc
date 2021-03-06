
#ifdef MAKE_ENHANCED_LIGAND_TOOLS

#include <fstream>

#include "utils/coot-utils.hh"
#include "rdkit-interface.hh"
#include "cod-types.hh"
#include "GraphMol/Rings.h"
#include "GraphMol/RingInfo.h"
#include "boost/dynamic_bitset.hpp"

std::ostream &
cod::operator<<(std::ostream &s, const cod::bond_table_record_t &btr) {

   s << btr.cod_type_1 << " " << btr.cod_type_2 << " " << btr.mean
     << " " << btr.std_dev << " " << btr.count;
   return s;
}

bool
cod::bond_record_container_t::write(const std::string file_name) const {

   bool status = false;
   std::ofstream f(file_name.c_str(), std::ios::binary);
   if (f) {
      for (unsigned int i=0; i<bonds.size(); i++) {
	 const bond_table_record_t &btr = bonds[i];
	 // f << bonds[i] << "\n";
	 f << btr.cod_type_1 << btr.cod_type_2 << btr.mean;
      }
      f.close();
      status = true;
   }
   return status;
}



// can throw a std::runtime_error
//
// rdkit_mol is not const because there is no const beginAtoms() operator.
std::vector<std::string>
cod::get_cod_atom_types(RDKit::ROMol &rdkm, bool add_name_as_property) {

   std::vector<std::string> v;

   RDKit::RingInfo* ring_info_p = rdkm.getRingInfo();
   unsigned int n_rings = ring_info_p->numRings();

   // Maybe add a vector of ring sizes to the atoms (most atoms will
   // not have a vector added (because they are not part of rings)).
   // 
   std::vector<std::vector<int> > atomRings = ring_info_p->atomRings();

   // Now sort ring_info so that the rings with more atoms are at
   // the top.  Practically 6-rings should come above 5-rings.
   //
   std::vector<std::vector<int> > sorted_atomRings = atomRings;
   // 
   // what is operator< for a vector of ints? Does this sort work!?
   // 
   std::sort(sorted_atomRings.begin(), sorted_atomRings.end());

   
   for (unsigned int i_ring=0; i_ring<n_rings; i_ring++) {
      std::vector<int> ring_atom_indices = sorted_atomRings[i_ring];

      int arom_flag = coot::is_aromatic_ring(ring_atom_indices, rdkm); // 0 or 1
      unsigned int n_ring_atoms = ring_atom_indices.size();

      // don't include macrocycle ring info (and the like (like cycloheptane))
      if (n_ring_atoms <= 6) {
	 for (unsigned int iat=0; iat<n_ring_atoms; iat++) { 
	    try {
	       // fill these by reference
	       std::vector<int> ring_size_vec;
	       std::vector<int> ring_arom_vec;

	       // when we add a ring to ring_size, we add flag for the
	       // aromaticity also.

	       rdkm[ring_atom_indices[iat]]->getProp("ring_size", ring_size_vec);
	       ring_size_vec.push_back(n_ring_atoms);
	       rdkm[ring_atom_indices[iat]]->setProp("ring_size", ring_size_vec);

	       rdkm[ring_atom_indices[iat]]->getProp("ring_arom", ring_arom_vec);
	       ring_arom_vec.push_back(arom_flag);
	       rdkm[ring_atom_indices[iat]]->setProp("ring_arom", ring_arom_vec);
	    }
	    catch (const KeyErrorException &err) {
	       // new ring info for this atom
	       std::vector<int> ring_size_vec(1);
	       std::vector<int> ring_arom_vec(1);
	       ring_size_vec[0] = n_ring_atoms;
	       ring_arom_vec[0] = arom_flag;
	       rdkm[ring_atom_indices[iat]]->setProp("ring_size", ring_size_vec);
	       rdkm[ring_atom_indices[iat]]->setProp("ring_arom", ring_arom_vec);
	    }
	 }
      }
   }

   boost::dynamic_bitset<> fusDone(n_rings);
   unsigned int curr = 0;
   RDKit::INT_INT_VECT_MAP neighMap; // std::map<int, std::vector<int> >
   RingUtils::makeRingNeighborMap(sorted_atomRings, neighMap);
   RDKit::INT_VECT fused;
   while (curr < n_rings) {
      fused.clear();
      RingUtils::pickFusedRings(curr, neighMap, fused, fusDone);
      std::vector<std::vector<int> > fused_rings;
      fused_rings.reserve(fused.size());
      for (RDKit::INT_VECT_CI rid = fused.begin(); rid != fused.end(); ++rid) {
	 fused_rings.push_back(sorted_atomRings[*rid]);
      }

      if (fused_rings.size() > 1)
	 handle_bigger_rings_from_fused_rings(rdkm, fused_rings);
      curr++;
   }
   
   RDKit::ROMol::AtomIterator ai;
   for(ai=rdkm.beginAtoms(); ai!=rdkm.endAtoms(); ai++) {
      
      std::pair<std::string, std::list<third_neighbour_info_t> > s_pair =
	 get_cod_atom_type(0, *ai, *ai, rdkm); // full-spec

      std::string s = s_pair.first;
      v.push_back(s);
      if (add_name_as_property)
	 (*ai)->setProp("CODAtomName", s);
   }
   
   if (v.size() != rdkm.getNumAtoms())
      throw std::runtime_error("mismatch size in get_cod_atom_types()");
      
   return v;
}

// ret
void
cod::handle_bigger_rings_from_fused_rings(RDKit::ROMol &rdkm,
					  const std::vector<std::vector<int> > &fused_rings) {


   // first make a connection "tree".  What is bonded to what (in the
   // rings).  bond map is a list of atom indices that are bonded to
   // the key (an atom index)
   //
   std::map<int, std::vector<int> > bond_map;
   std::map<int, std::vector<int> >::iterator it;

   for (unsigned int iat=0; iat<rdkm.getNumAtoms(); iat++) {
      if (is_ring_member(iat, fused_rings)) {
	 RDKit::Atom *this_at = rdkm[iat].get();
	 unsigned int idx_c = this_at->getIdx();
	 RDKit::ROMol::ADJ_ITER nbrIdx, endNbrs;
	 boost::tie(nbrIdx, endNbrs) = rdkm.getAtomNeighbors(this_at);
	 while(nbrIdx != endNbrs) {
	    if (is_ring_member(*nbrIdx, fused_rings)) { 
	       RDKit::ATOM_SPTR at = rdkm[*nbrIdx];
	       RDKit::Bond *bond = rdkm.getBondBetweenAtoms(idx_c, *nbrIdx);
	       if (bond) {
		  it = bond_map.find(iat);
		  if (it == bond_map.end()) {
		     // not there, add it.
		     bond_map[iat].push_back(*nbrIdx);
		  } else {
		     std::vector<int> &v = it->second;
		     if (std::find(v.begin(), v.end(), *nbrIdx) == v.end())
			v.push_back(*nbrIdx);
		  }
	       }
	    }
	    nbrIdx++;
	 }
      }
   }

   // check the bond map (debugging output)
   //
   if (0) { 
      for(it=bond_map.begin(); it!=bond_map.end(); it++) {
	 try {
	    std::string name;
	    rdkm[it->first]->getProp("name", name);
	    std::cout << "   " << it->first << " " << name <<  " : ";
	    for (unsigned int i=0; i<it->second.size(); i++)
	       std::cout << it->second[i] << " ";
	    std::cout << " :::: ";
	    for (unsigned int i=0; i<it->second.size(); i++) {
	       rdkm[it->second[i]]->getProp("name", name);
	       std::cout << name << " ";
	    }
	    std::cout << std::endl;
	 }
	 catch (const KeyErrorException &kee) {
	 }
      }
   }


   unsigned int n_max_bonds = 6; // max ring size of interest (for COD)

   // we need to rewrite this part // FIXME
   // 
   if (false) { 
      // Return all the paths by which we get back to idx (using upto
      // (and including) n_max_bonds bonds (include backward paths also).
      // 
      for(it=bond_map.begin(); it!=bond_map.end(); it++) {
	 unsigned int idx = it->first;
	 // this returns all the paths! (in forward and backward directions)
	 std::vector<std::vector<int> > paths = trace_path(idx, bond_map, n_max_bonds);
	 if (paths.size() > 1) {
	    std::vector<int> path_sizes(paths.size());
	    for (unsigned int ii=0; ii<paths.size(); ii++)
	       path_sizes[ii] = paths[ii].size();
	    std::sort(path_sizes.begin(), path_sizes.end());

	    // dangerous interger arithmetic?
	    // we want every other path, not 5(f), 5(b), 6(f), 6(b), so
	    // sort them (smallest first) and take every other one.
	    // std::cout << "here are the path sizes: " << std::endl;
	    // for (unsigned int jj=0; jj<path_sizes.size(); jj++) { 
	    // std::cout << "   " << jj << " " << path_sizes[jj] << std::endl;
	    
	    // std::cout << "... about to do dangerous " << std::endl;
	    std::vector<int> filtered_path_sizes(path_sizes.size()/2);
	    for (unsigned int ii=0; ii<filtered_path_sizes.size(); ii++) { // every other
	       filtered_path_sizes[ii] = path_sizes[ii*2];
	       // std::cout << "set filtered_path_sizes[" << ii << "] to  path_sizes[ii*2] "
	       // << path_sizes[ii*2] << std::endl;
	    }

	    rdkm[it->first]->setProp("ring", filtered_path_sizes);
	 
	    // std::cout << "..... done dangerous" << std::endl;
	 }
      }
   }
}

bool
cod::is_ring_member(unsigned int iat_ui,
		    const std::vector<std::vector<int> > &fused_rings) {

   int iat = iat_ui;
   for (unsigned int ir=0; ir<fused_rings.size(); ir++) { 
      for (unsigned int ii=0; ii<fused_rings[ir].size(); ii++) {
	 if (fused_rings[ir][ii] == iat)
	    return true;
      }
   }
   return false;
}

std::vector<std::vector<int> >
cod::trace_path(unsigned int idx,
		const std::map<int, std::vector<int> > &bond_map,
		unsigned int n_max_bonds) {

   std::vector<int> in_path;
   return trace_path(idx, in_path, idx, bond_map, n_max_bonds);
}

std::vector<std::vector<int> > 
cod::trace_path(unsigned int idx,
		std::vector<int> in_path_indices,
		unsigned int target_idx,
		const std::map<int, std::vector<int> > &bond_map,
		unsigned int level) {

   std::vector<std::vector<int> > vr;
   if (level == 0) {
      return vr; // empty
   } else {

      in_path_indices.push_back(idx);
      std::map<int, std::vector<int> >::const_iterator it_1 = bond_map.find(idx);
      const std::vector<int> &neighbs = it_1->second;
      for (unsigned int in=0; in<neighbs.size(); in++) {

	 bool do_recursion = false; // clumsy setting of "shall we look deeper"

	 if (neighbs[in] == int(target_idx)) {
	    if (in_path_indices.size() > 2) { // not just out and back
	       vr.push_back(in_path_indices);
	       return vr;
	    }
	 } else { 
	    if (std::find(in_path_indices.begin(), in_path_indices.end(), neighbs[in]) ==
		in_path_indices.end())
	       do_recursion = true; // only if the forward neighbor is not in current path shall
	                            // we continue deeper.
	 }
	 
	 if (do_recursion) {
	    std::vector<std::vector<int> > v = trace_path(neighbs[in],
							  in_path_indices,
							  target_idx,
							  bond_map,
							  level-1);
		  
	    if (v.size() > 0)
	       for (unsigned int i=0; i<v.size(); i++)
		  vr.push_back(v[i]);
	 }
      }
   }
   return vr;
}




// can throw a std::runtime_error
//
// The lower the level the less
// sophisticated is the atom typing - level 0 is the lowest level atom
// type.

// nb_level is optional arg, default 0 (i.e. full atom type)
// 
// The nb_level is the neighbour level. 1 is first-neighbour
// (i.e. directly bonded) 2 is related via angle and 3 is related by
// torsion.  nb_level = 3 info is only needed for when the atom_p is
// in the ring or NB-1 of atom_p is in the ring and NB-2 is in the
// ring.
//
// We pass base because (if it is not null) we are not interested in
// tracing back up the tree (back to root via base atoms).
//
// NB-3 info should be kept separate
//
// atom_parent_p is the bonded parent of atom_p (used to check for
// back-tracking)
//
// atom_base_p: the centroal (level 0) atom for which we are
// calculating the atom types eventually.
// 
std::pair<std::string, std::list<cod::third_neighbour_info_t> >
cod::get_cod_atom_type(RDKit::Atom *atom_parent_p,
		       RDKit::Atom *atom_p,
		       RDKit::Atom *atom_base_p,
		       const RDKit::ROMol &rdkm,
		       int nb_level) {

   std::list<third_neighbour_info_t> tnil;

   std::string s;
   const RDKit::PeriodicTable *tbl = RDKit::PeriodicTable::getTable();
   int n = atom_p->getAtomicNum();
   std::string atom_ele = tbl->getElementSymbol(n);
   std::string atom_ring_string = "";
   // thses are data types that we can add to atom properties and are
   // linked together (by hand), i.e. whenever we add to
   // ring_size_info, we add to ring_atom_info.
   std::vector<int> ring_size_vec; // key: "ring_size"
   std::vector<int> ring_arom_vec; // key: "ring_arom"
   // std::vector<ring_info_t> ring_info_vec;

   if (nb_level != 3) {
      
      try {
	 atom_p->getProp("ring_size", ring_size_vec);
	 atom_p->getProp("ring_arom", ring_arom_vec);

	 // sort ring_info so that the rings with more atoms are at
	 // the top.  Practically 6-rings should come above 5-rings.

	 // sorting by ring size is done when the ring info is constructed
	 // and added to atoms - no here
	 // std::sort(ring_si.begin(), ring_info.end());
	 
	 atom_ring_string = "[";
	 for (unsigned int i_ring=0; i_ring<ring_size_vec.size(); i_ring++) {

	    if (i_ring > 0)
	       atom_ring_string += ",";

	    // If this ring is aromatic we need at append "a" to the (typically)
	    // n_atoms_in_ring = 6.
	    // 
	    int n_atoms_in_ring = ring_size_vec[i_ring];

	    atom_ring_string += coot::util::int_to_string(n_atoms_in_ring);
	    int arom = ring_arom_vec[i_ring];
	    if (arom) {
	       atom_ring_string += "a";
	    }
	 }
	 atom_ring_string += "]";
      }
      catch (const KeyErrorException &kee) {
	 // no ring info on that atom, that's fine
	 if (false)
	    std::cout << "  debug-info:: no ring info for atom " << atom_p
		      << std::endl;
      }
   }

   // recursion block
   //
   if (nb_level < 3) { // this is always true, I think
      
      std::vector<std::string> neighbour_types;

      RDKit::ROMol::ADJ_ITER nbrIdx, endNbrs;
      boost::tie(nbrIdx, endNbrs) = rdkm.getAtomNeighbors(atom_p);
      while (nbrIdx != endNbrs) {
	 RDKit::ATOM_SPTR at = rdkm[*nbrIdx];
	 RDKit::Atom *neigh_atom_p = at.get();

	 if (neigh_atom_p == atom_parent_p) {
	    // neighbour of central atom was back to parent.
	 } else {

	    // neighb_type is the type for the next level.  We don't want to
	    // add level_3 neighb_types (give nb_level == 2)
	    //
	    if (nb_level < 2) {
	       std::pair<std::string, std::list<third_neighbour_info_t> > s_pair =
		  get_cod_atom_type(atom_p, neigh_atom_p, atom_base_p, rdkm, nb_level+1);
	       std::string neighb_type = s_pair.first;
	       neighbour_types.push_back(neighb_type);

	       if (s_pair.second.size()) {
		  std::list<third_neighbour_info_t>::const_iterator it;
		  for (it=s_pair.second.begin(); it!=s_pair.second.end(); it++)
		     tnil.push_back(*it);
		  tnil.sort();
		  tnil.unique();
	       }
	    } else {

	       third_neighbour_info_t tni =
		  get_cod_nb_3_type(atom_p, neigh_atom_p, atom_base_p, rdkm);
	       if (! tni.ele.empty()) {
		  tnil.push_back(tni);
		  tnil.sort();
		  tnil.unique();
	       }
	    }
	 }
	 nbrIdx++;
      }
      atom_ele += atom_ring_string;

      tnil.sort();
      tnil.unique();
      s = make_cod_type(atom_base_p, atom_ele, neighbour_types, tnil, nb_level);
   }

   if (false)
      std::cout << "get_cod_atom_type():    level " << nb_level << " returning :"
		<< s << ":" << std::endl;

   return std::pair<std::string, std::list<cod::third_neighbour_info_t> > (s, tnil);

}

// atom_p is the current atom for which we want the 3rd-neighbour
// types, atom_base_p is the original (central, level 0) atom and we
// need that because NB-3 info is only given for atoms that share the
// ring with atom_base_p or if atom_p is a bonded neighbour of an atom
// that shares a ring with NB-3.
// 
cod::third_neighbour_info_t
cod::get_cod_nb_3_type(RDKit::Atom *atom_parent_p,
		       RDKit::Atom *atom_p,
		       RDKit::Atom *atom_base_p, // the original atom 
		       const RDKit::ROMol &rdkm) {
   
   third_neighbour_info_t tni;

   bool in_ring = false;

   if (atom_base_p) { // should always be true

      try {
	 std::vector<int> ring_size_vec;
	 atom_base_p->getProp("ring_size", ring_size_vec);
	 in_ring = true;
      }
      catch (const KeyErrorException &kee) {
	 // no ring info on that atom, that's fine
      }
      
      if (in_ring) {

	 bool match_for_3rd_nb_info_flag =
	    check_for_3rd_nb_info(atom_parent_p, atom_p, atom_base_p, rdkm);
	 
	 if (match_for_3rd_nb_info_flag) {
	    
	    // i.e. in same ring or atom_p is attached to an atom in
	    // the same ring as atom_base_p

	    // what is the element and degree of atom_p?
	    
	    const RDKit::PeriodicTable *tbl = RDKit::PeriodicTable::getTable();
	    int n = atom_p->getAtomicNum();
	    std::string atom_ele = tbl->getElementSymbol(n);
	    unsigned int degree = atom_p->getDegree();

	    tni = third_neighbour_info_t(atom_p, atom_ele, degree);
	 }
      }
   }
   return tni;
}

bool 
cod::check_for_3rd_nb_info(RDKit::Atom *atom_parent_p,
			   RDKit::Atom *atom_p,
			   RDKit::Atom *atom_base_p,
			   const RDKit::ROMol &rdkm) {

   bool related = false;

   // are atom_parent_p and atom_base_p in the same ring?

   RDKit::RingInfo* ring_info_p = rdkm.getRingInfo();
   unsigned int n_rings = ring_info_p->numRings();
   std::vector<std::vector<int> > atomRings = ring_info_p->atomRings();
   for (unsigned int i_ring=0; i_ring<n_rings; i_ring++) {
      const std::vector<int> &ring_atom_indices = atomRings[i_ring];
      unsigned int n_ring_atoms = ring_atom_indices.size();
      bool found_parent = false;
      bool found_base   = false;
      bool found_this   = false;
      for (unsigned int iat=0; iat<n_ring_atoms; iat++) {
	 RDKit::Atom *ring_atom_p = rdkm[ring_atom_indices[iat]].get();

	 if (ring_atom_p == atom_parent_p)
	    found_parent = true;
	 if (ring_atom_p == atom_base_p)
	    found_base = true;
	 if (ring_atom_p == atom_p)
	    found_this = true;
      }

      if (found_parent && found_base) {
	 related = true;
	 break;
      }
   }
   

   // or

   // something else?

   return related;

}


bool
cod::neighbour_sorter(const std::string &a, const std::string &b) {
   if (a.length() > b.length())
      return true;
   if (a.length() < b.length())
      return false;
   return (a < b);
} 


std::vector<std::string>
cod::sort_neighbours(const std::vector<std::string> &neighbours_in, int level) {

   std::vector<std::string> n = neighbours_in;
   std::sort(n.begin(), n.end(), neighbour_sorter);
   return n;
}

// we are making cod types for which base atom?
// base_atom_p.
//
std::string
cod::make_cod_type(RDKit::Atom *base_atom_p,
		   const std::string &atom_ele,
		   const std::vector<std::string> &neighbour_types,
		   const std::list<third_neighbour_info_t> &tnil,
		   int nb_level) {

   std::string s;
   std::vector<std::string> n = sort_neighbours(neighbour_types, nb_level);

   if (false) { 
      std::string name;
      if (base_atom_p)
	 base_atom_p->getProp("name", name);
      std::cout << "---- in make_cod_type() atom name: " << name
		<< "  atom_ele " << atom_ele
		<< " level " << nb_level
		<< " has sorted neighbour_types:\n";
      for (unsigned int ii=0; ii<n.size(); ii++)
	 std::cout << "   " << n[ii] << std::endl;
   
      if (tnil.size()) {
	 std::cout << " and has the following 3rd neighbours:\n";
	 std::list<third_neighbour_info_t>::const_iterator it;

	 // debug input to this function
	 std::string name;
	 for (it=tnil.begin(); it!=tnil.end(); it++) {
	    it->atom_p->getProp("name", name);
	    std::cout << name << " "  << it->atom_p << "   \"" << it->ele << "\" "
		      << it->degree << std::endl;
	 }
      }
   }

   if (n.size() == 0) {
      s = atom_ele;
   } else {
      s = atom_ele;
      std::vector<std::string> done_same;
      for (unsigned int i=0; i<n.size(); i++) {

	 // don't do anything if this neighbour_type is already in
	 // done_same (if it is of course, it has already been
	 // handled)
	 // 
	 std::vector<std::string>::const_iterator it =
	    std::find(done_same.begin(), done_same.end(), n[i]);
	 if (it == done_same.end()) {
	    // was not in done_same

	    std::vector<int> same; // indices of atoms with same type as this neighbour_type
	    for (unsigned int j=i+1; j<n.size(); j++)
	       if (n[j] == n[i])
		  same.push_back(j);

	    // now add neighbour info to s:
	    //
	    if (same.size() == 0) {
	       if (nb_level == 0) { 
		  s += "(";
		  s += n[i];
		  s += ")";
	       } else {
		  s += n[i];
	       }
	    } else {

	       // compression means "Fe2" instead of "FeFe" or "H3"
	       // instead of "HHH".  We don't want to do compression
	       // for "HH", because that results in "H2" - and thus
	       // doesn't make the string shorter (sigh).
	       bool do_compression = true;

	       if ((n[i].length() == 1) && (same.size() == 1))
		  do_compression = false;

	       // ----------------------- level 1 --------------------------------
	       // 
	       if (nb_level == 1) {
		  if (same.size() == 0) { 
		     s += n[i];
		  } else { 
		     if (do_compression) { 
			std::string save_s = s;
			s = "";
			s += save_s;
			s += n[i];
			s += coot::util::int_to_string(same.size()+1);
		     } else {
			std::string save_s = s;
			s = "";
			s += save_s;
			s += n[i];
			s += n[i];
		     }
		  }
	       }

	       // ----------------------- level 0 --------------------------------
	       // 
	       if (nb_level == 0) {
		  if (same.size() == 0) {
		     s += "(";
		     s += n[i];
		     s += ")";
		  } else {
		     s += "(";
		     s += n[i];
		     s+= ")"; // possibly don't do this if level is 0
		     s += coot::util::int_to_string(same.size()+1);

		  }
	       }
	    }
	    // we've done this (so that we pass over these later on in the n vector)
	    done_same.push_back(n[i]);
	 }
      }
   }

   if (! s.empty()) {
      if (nb_level == 0) {
	 std::string t = make_cod_3rd_neighb_info_type(tnil);
	 s += t;
      }
   }
   return s;
}


// There are no repeats of atoms in tnil.
std::string
cod::make_cod_3rd_neighb_info_type(const std::list<third_neighbour_info_t> &tnil) {

   // but if we have 1|C<3> and another 1|C<3> say, we do need to
   // consolidate those to 2|C<3>.

   // and we need to put C atoms before H atoms

   std::string s;

   if (tnil.size()) {

      std::list<third_neighbour_info_t>::const_iterator it;
      std::map<std::string, int> type_map;
      std::map<std::string, int>::iterator it_map;
      for (it=tnil.begin(); it!=tnil.end(); it++) {
	 std::string key = it->ele;
	 key += "<";
	 key += coot::util::int_to_string(it->degree);
	 key += ">";
	 it_map = type_map.find(key);
	 if (it_map == type_map.end()) {
	    type_map[key] = 1;
	 } else {
	    it_map->second++;
	 } 
      }

      if (type_map.size() > 0) { 

	 s = "{";
	 std::map<std::string, int>::iterator it_last_value = type_map.end();
	 it_last_value--;
	 for(it_map=type_map.begin(); it_map!=type_map.end(); it_map++) {
	    s += coot::util::int_to_string(it_map->second);
	    s += "|";
	    s += it_map->first;
	    if (it_map != it_last_value)
	       s += ",";
	 }      
	 s += "}";
      }
   }

   return s;
}

cod::bond_record_container_t
cod::read_acedrg_table(const std::string &file_name) {

   bond_record_container_t brc;
   
   std::ifstream f(file_name.c_str());

   if (f) {
      std::vector<std::string> lines;
      std::string line;

      while (std::getline(f, line)) { 
         lines.push_back(line);
      }
      f.close();
      std::cout << "read " << lines.size() << " lines from "
		<< file_name << std::endl;

      for (unsigned int i=0; i<lines.size(); i++) { 
	 const std::string &line = lines[i];
	 std::vector<std::string> bits = coot::util::split_string_no_blanks(line);

	 if (bits.size() == 18) {
	    const std::string &cod_type_1  = bits[10];
	    const std::string &cod_type_2  = bits[11];
	    const std::string &mean_str    = bits[12];
	    const std::string &std_dev_str = bits[13];
	    const std::string &count_str   = bits[14];

	    try {
	       double mean   = coot::util::string_to_float(mean_str);
	       double stddev = coot::util::string_to_float(std_dev_str);
	       int count     = coot::util::string_to_int(count_str);

	       bond_table_record_t rec(cod_type_1, cod_type_2, mean, stddev, count);
	       brc.add(rec);
	    }
	    catch (const std::runtime_error &rte) {
	       std::cout << "error converting " << rte.what() << std::endl;
	    }
	 }
      }

   }
   return brc;
}

// return the consolidated table
// 
cod::bond_record_container_t
cod::read_acedrg_table_dir(const std::string &dir_name) {

   bond_record_container_t brc;
   std::string glob_pattern = "*.table";
   std::vector<std::string> tables = coot::util::glob_files(dir_name, glob_pattern);

   for (unsigned int i=0; i<tables.size(); i++) {
      const std::string &table = tables[i];
      bond_record_container_t single = read_acedrg_table(table);
      brc.add_table(single);
   }

   return brc;

}

#endif
