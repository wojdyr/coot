
ligand_check_refmac_mtz = False
ligand_check_refmac_fobs_col = False
ligand_check_refmac_sigfobs_col = False
ligand_check_refmac_r_free_col = False

def ligand_check_refmac_columns(f_list, sigf_list, rfree_list):

    # dummy function, not needed anywhere (as is)
    # need the above as globals...

    # happy path

    # Using the first sigf (there should only be one typically)
    # get the F label (Fx) from x/y/SIGFPx
    #
    pass
    

if enhanced_ligand_coot_p():

    global use_mogul
    if use_mogul:
        def import_from_3d_generator_from_mdl(dummy1, dummy2):
            # here we could have something... to redefine and use mogul
            pass
    
    if (use_gui_qm != 2):
        menu = coot_menubar_menu("Ligand")
        
        add_simple_coot_menu_menuitem(
          menu,
          "SMILES -> 2D",
          lambda func:
          generic_single_entry("SMILES string",
                               "", " Send to 2D Viewer ",
                               lambda text: smiles_to_ligand_builder(text)
                               )
          )


        add_simple_coot_menu_menuitem(
            menu,
            "Residue -> 2D",
            lambda func:
            using_active_atom(residue_to_ligand_builder,
                              "aa_imol", "aa_chain_id", "aa_res_no",
                              "aa_ins_code", 0.015)
            )


        def flev_rdkit_func():
            with UsingActiveAtom() as [aa_imol, aa_chain_id, aa_res_no,
                                       aa_ins_code, aa_atom_name, aa_alt_conf]:
                fle_view_with_rdkit(aa_imol, aa_chain_id, aa_res_no,
                                    aa_ins_code, 4.2)
                set_flev_idle_ligand_interactions(1)
            
        add_simple_coot_menu_menuitem(
            menu,
            "FLEV this residue",
            lambda func: flev_rdkit_func()
            )


        add_simple_coot_menu_menuitem(
            menu,
            "Toggle Ligand Interactions",
            lambda func: toggle_idle_ligand_interactions()
            )


        def go_solid_func(state):
            set_display_generic_objects_as_solid(state)
            graphics_draw()

        add_simple_coot_menu_menuitem(
            menu, "Solid Generic Objects",
            lambda func: go_solid_func(1))

        add_simple_coot_menu_menuitem(
            menu, "Unsolid Generic Objects",
            lambda func: go_solid_func(0))
        

        def show_chem_func():
            set_display_generic_objects_as_solid(1) # there may be consequences...
            with UsingActiveAtom() as [aa_imol, aa_chain_id, aa_res_no,
                                       aa_ins_code, aa_atom_name, aa_alt_conf]:
                show_feats(aa_imol, aa_chain_id, aa_res_no, aa_ins_code)
            
        add_simple_coot_menu_menuitem(
            menu,
            "Show Chemical Features",
            lambda func: show_chem_func()
            )


        def tab_ligand_distorsions_func():
            with UsingActiveAtom() as [aa_imol, aa_chain_id, aa_res_no,
                                       aa_ins_code, aa_atom_name, aa_alt_conf]:
                print_residue_distortions(aa_imol, aa_chain_id, aa_res_no, aa_ins_code)

        add_simple_coot_menu_menuitem(
            menu,
            "Tabulate Ligand Distorsions",
            lambda func: tab_ligand_distorsions_func()
            )


        def write_sdf_func():
            with UsingActiveAtom() as [aa_imol, aa_chain_id, aa_res_no,
                                       aa_ins_code, aa_atom_name, aa_alt_conf]:
                rn = residue_name(aa_imol, aa_chain_id, aa_res_no, aa_ins_code)
                file_name = rn + ".sdf"
                residue_to_sdf_file(aa_imol, aa_chain_id, aa_res_no, aa_ins_code,
                                    file_name)

        add_simple_coot_menu_menuitem(
            menu,
            "write sdf file",
            lambda func: write_sdf_func()
            )


        def density_ligand_score_func():
            with UsingActiveAtom() as [aa_imol, aa_chain_id, aa_res_no,
                                       aa_ins_code, aa_atom_name, aa_alt_conf]:
               spec = [aa_chain_id, aa_res_no, aa_ins_code]
               r = density_score_residue(aa_imol, spec, imol_refinement_map())
               # BL says:: maybe a dialog for this?!
               print "density at ligand atoms:", r

        add_simple_coot_menu_menuitem(
            menu,
            "Density Score Ligand",
            lambda func: density_ligand_score_func()
            )


        def fetch_ligand_pdbe_func():
            with UsingActiveAtom() as [aa_imol, aa_chain_id, aa_res_no,
                               aa_ins_code, aa_atom_name, aa_alt_conf]:
               comp_id = residue_name(aa_imol, aa_chain_id, aa_res_no, aa_ins_code)
               print "here with residue name", comp_id
               s = get_SMILES_for_comp_id_from_pdbe(comp_id)
               if (isinstance(s, str)):
                   pdbe_cif_file_name = os.path.join("coot-download", "PDBe-" + comp_id + ".cif")
                   import_from_3d_generator_from_mdl(pdbe_cif_file_name, comp_id)

        add_simple_coot_menu_menuitem(
            menu,
            "### [Fetch ligand description & generate restraints]",
            lambda func: fetch_ligand_pdbe_func()
            )


        def probe_ligand_func():
            with UsingActiveAtom() as [aa_imol, aa_chain_id, aa_res_no,
                               aa_ins_code, aa_atom_name, aa_alt_conf]:
               ss = "//" + aa_chain_id + "/" + str(aa_res_no)
               imol_selection = new_molecule_by_atom_selection(aa_imol, ss)
               work_dir = get_directory("coot-molprobity")
               tmp_selected_ligand_for_probe_pdb = os.path.join(work_dir,
                                                   "tmp-selected-ligand-for-probe.pdb")
               tmp_protein_for_probe_pdb = os.path.join(work_dir,
                                                    "tmp-protein-for-probe.pdb")
               probe_dots_file_name = os.path.join(work_dir, "probe.dots")

               set_mol_displayed(imol_selection, 0)
               set_mol_active(imol_selection, 0)
               # BL comment: we assume H and view is correct.
               #set_go_to_atom_molecule(imol)
               #rc = residue_centre(imol, chain_id, res_no, ins_code)
               #set_rotation_centre(*rc)
               #hydrogenate_region(6)
               write_pdb_file(imol_selection, tmp_selected_ligand_for_probe_pdb)
               write_pdb_file(imol, tmp_protein_for_probe_pdb)
               popen_command(probe_command, ["-u", "-once", str(aa_res_no), # -once or -both
                                             "not " + str(aa_res_no),
                                             "-density60",
                                             tmp_selected_ligand_for_probe_pdb,
                                             tmp_protein_for_probe_pdb],
                                             [], probe_dots_file_name, False)
               handle_read_draw_probe_dots_unformatted(dots_file_name, aa_imol, 0)
               graphics_draw()



