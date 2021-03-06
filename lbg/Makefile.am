
lib_LTLIBRARIES=libcoot-lidia.la

AM_CPPFLAGS = \
	-I$(top_srcdir) \
        $(GSL_CFLAGS) \
        $(GOOCANVAS_CFLAGS) \
        $(GTK_CFLAGS) \
	$(CLIPPER_CXXFLAGS) $(CCP4SRS_CXXFLAGS) $(MMDB_CXXFLAGS) \
	$(COOT_WINDOWS_CFLAGS) \
	$(ENHANCED_LIGAND_TOOLS_CFLAGS)      \
	$(RDKIT_CXXFLAGS) \
        $(PYTHON_CFLAGS)  

AM_CXXFLAGS = \
	-DPKGDATADIR='"$(pkgdatadir)"'


bin_PROGRAMS = lidia

libcoot_lidia_la_SOURCES = lbg.cc wmolecule.cc  \
    lbg-search.cc  residue-circles.cc pi-stacking.cc \
    flev-annotations.cc lbg-drag-and-drop.cc qed-interface.cc alerts.cc alerts-list.cc

# qed-interface.cc uses a boost::python function directly, so add L_BOOST_PYTHON
# Thanks Marcin.
libcoot_lidia_la_LIBADD = \
	$(top_builddir)/coot-utils/libcoot-coord-utils.la \
	$(top_builddir)/geometry/libcoot-geometry.la \
	$(top_builddir)/mini-mol/libcoot-mini-mol.la \
	$(top_builddir)/lidia-core/libcoot-lidia-core.la \
	$(top_builddir)/utils/libcoot-utils.la \
	$(CLIPPER_LIBS) $(CCP4SRS_LIBS) \
	$(MMDB_LIBS) $(GSL_LIBS) $(RDKIT_LIBS) $(L_BOOST_PYTHON) $(GOOCANVAS_LIBS) $(GTK_LIBS) $(PYTHON_LIBS)

# Bernie-mode: libcoot_lidia_la_LDFLAGS = $(SHARED_LDFLAGS)  -Wl,--no-undefined
# 
libcoot_lidia_la_LDFLAGS = $(SHARED_LDFLAGS)

lidia_SOURCES = lidia-main.cc lbg-callbacks-outer.cc 

pkginclude_HEADERS = lbg.hh wmolecule.hh  flev-annotations.hh graphics-c-interface-functions.hh \
	lbg-drag-and-drop.hh qed-interface.hh alerts.hh pi-stacking.hh
pkgincludedir = $(includedir)/coot/lbg

# 20140521
# lidia_LDFLAGS = -Wl,--export-dynamic
# 
# Now we use EXPORT_DYNAMIC_FLAG_SPEC which usually evaluates to ${wl}--export-dynamic
# (but not for MacOSX, where it Does The Right Thing (hopefully)).
# We manually set the value of wl here, but maybe the Right Thing To Do for that is 
# run (eval) $($LIBTOOL --config | grep ^wl).  How do I do that here though?  
# I don't understand.  Maybe the setting of wl should be done in configure?  But then
# I have to force the output of libtool.  Bleugh.  Hmm...
# Anyway, thankyou Gary Vaughan.
wl=-Wl,
lidia_LDFLAGS = $(EXPORT_DYNAMIC_FLAG_SPEC)

# res spec output operator means we include libcoot-coord-utils
# (if that is annoying, then remove it and expand it by hand)
#
lidia_LDADD = \
	./libcoot-lidia.la \
	$(top_builddir)/coot-utils/libcoot-coord-utils.la \
	$(top_builddir)/geometry/libcoot-geometry.la \
	$(top_builddir)/mini-mol/libcoot-mini-mol.la \
	$(top_builddir)/utils/libcoot-utils.la \
	$(top_builddir)/lidia-core/libcoot-lidia-core.la \
	$(GLOB_LIBS) $(CLIPPER_LIBS) $(MMDB_LIBS) $(GSL_LIBS) \
	$(RDKIT_LIBS) $(L_BOOST_PYTHON) $(GOOCANVAS_LIBS) $(GTK_LIBS) $(PYTHON_LIBS)

EXTRA_DIST=lbg.glade $(appicon_DATA) lbg-callbacks.cc

subpkgdatadir = $(pkgdatadir)
subpkgdata_DATA = lbg.glade lbg-callbacks.cc

check_PROGRAMS = test_lbg_functions
test_lbg_functions_SOURCES = test_lbg_functions.cc  graphics-c-interface-functions-blanks.cc
test_lbg_functions_LDADD = \
       ./libcoot-lidia.la \
	$(top_builddir)/lidia-core/libcoot-lidia-core.la \
	$(top_builddir)/geometry/libcoot-geometry.la \
	$(top_builddir)/utils/libcoot-utils.la \
	$(RDKIT_LIBS) \
	$(PYTHON_LIBS)


appicondir=$(pkgdatadir)/pixmaps
appicon_DATA = \
	3-c-new.svg    6-c-shadow-experiment.svg	   single.svg                           \
	3-c-other.svg  6-c.svg				   sp3.svg                              \
	3-c.svg        7-c.svg				   stereo-out.svg                       \
	4-c.svg        8-c.svg				   TheStructorr_magnifying_glass-32.svg \
	5-c.svg        Anonymous_Architetto_--_Chiave.svg  TheStructorr_magnifying_glass.svg    \
	6-c-arom.svg   charge.svg			   triple.svg                           \
	6-c-exp-2.svg  double.svg 			   Trash-hydrogens-Andy_Trash_Can.svg   \
	face-smile.svg 05_Hotel_Icon_Hair_Salon.svg

## #else 

##libcoot_lidia_la_SOURCES = flev-annotations.cc
## include_HEADERS = flev-annotations.hh

## endif
