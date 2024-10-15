if (CONFIG_LVGL)
set(incs)
set(srcs
	)


if (CONFIG_FREETYPE)
set(incs
	./include
	)

set(srcs
	src/autofit/autofit.c
	src/bdf/bdf.c
	src/cff/cff.c
	src/cache/ftcache.c
	src/gzip/ftgzip.c
	src/bzip2/ftbzip2.c
	src/base/ftbase.c
	src/base/ftinit.c
	src/base/ftsystem.c
	src/base/ftglyph.c
	src/base/ftbitmap.c
	src/base/ftsynth.c
	src/base/ftbbox.c
	src/base/ftpatent.c
	src/base/ftmm.c
	src/base/ftcid.c
	src/base/ftfstype.c
	src/base/fttype1.c
	src/base/ftbdf.c
	src/base/ftgasp.c
	src/base/ftwinfnt.c
	src/base/ftpfr.c
	src/base/ftstroke.c
	src/base/ftotval.c
	src/base/ftgxval.c
	src/base/ftdebug.c
	src/base/ftfile.c
	src/base/ftmem.c
	src/base/ftobjs.c
	src/lzw/ftlzw.c
	src/pcf/pcf.c
	src/pfr/pfr.c
	src/psaux/psaux.c
	src/pshinter/pshinter.c
	src/psnames/psmodule.c
	src/psnames/psnames.c
	src/raster/raster.c
	src/sfnt/sfnt.c
	src/smooth/smooth.c
	src/truetype/truetype.c
	src/type1/type1.c
	src/cid/type1cid.c
	src/type42/type42.c
	src/winfonts/winfnt.c
	
	src/sdf/sdf.c
	src/svg/svg.c
#	src/dlg/dlg.c
	)
endif()
endif()

armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES bk_rtos bk_init bk_cli bk_common include fatfs
	)




