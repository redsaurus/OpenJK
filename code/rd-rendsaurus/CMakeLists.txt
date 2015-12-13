#============================================================================
# Copyright (C) 2013 - 2015, OpenJK contributors
#
# This file is part of the OpenJK source code.
#
# OpenJK is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>.
#============================================================================

# Make sure the user is not executing this script directly
if(NOT InOpenJK)
	message(FATAL_ERROR "Use the top-level cmake script!")
endif(NOT InOpenJK)

if(BuildSPRdRendsaurus)

	set(SPRDRendsaurusRendererIncludeDirectories ${SPDir})
	set(SPRDRendsaurusRendererIncludeDirectories
		${SharedDir}
		${SPRDRendsaurusRendererIncludeDirectories}
		"${SPDir}/rd-rendsaurus"
		)

	# Modular Renderer crap (let's hope this works..)

	# Defines
	set(SPRDRendsaurusDefines ${SharedDefines} "RENDERER" "_JK2EXE")

	# Files

	# GHOUL 2
	set(SPRDRendsaurusG2Files
		"${SPDir}/ghoul2/G2.h"
		"${SPDir}/ghoul2/ghoul2_gore.h"
		"${SPDir}/rd-rendsaurus/G2_API.cpp"
		"${SPDir}/rd-rendsaurus/G2_bolts.cpp"
		"${SPDir}/rd-rendsaurus/G2_bones.cpp"
		"${SPDir}/rd-rendsaurus/G2_misc.cpp"
		"${SPDir}/rd-rendsaurus/G2_surfaces.cpp"
		)
	source_group("ghoul2" FILES ${SPRDRendsaurusG2Files})
	set(SPRDRendsaurusFiles ${SPRDRendsaurusFiles} ${SPRDRendsaurusG2Files})

    # Transparently use either bundled or system jpeg.
    list(APPEND SPRDRendsaurusRendererIncludeDirectories ${JPEG_INCLUDE_DIR})
    list(APPEND SPRDRendsaurusRendererLibraries          ${JPEG_LIBRARIES})

    # Transparently use either bundled or system libpng.  Order is important --
    # libpng used zlib, so it must come before it on the linker command line.
    list(APPEND SPRDRendsaurusRendererIncludeDirectories ${PNG_INCLUDE_DIRS})
    list(APPEND SPRDRendsaurusRendererLibraries          ${PNG_LIBRARIES})

    # Transparently use either bundled or system zlib.
    list(APPEND SPRDRendsaurusRendererIncludeDirectories ${ZLIB_INCLUDE_DIR})
    list(APPEND SPRDRendsaurusRendererLibraries          ${ZLIB_LIBRARIES})

    # Transparently use our bundled minizip.
    list(APPEND SPRDRendsaurusRendererIncludeDirectories ${MINIZIP_INCLUDE_DIRS})
    list(APPEND SPRDVRendsaurusRendererLibraries          ${MINIZIP_LIBRARIES})

    # Transparantly make use of all renderer directories
    list(APPEND SPRDRendsaurusRendererIncludeDirectories "${SPDir}/rd-common")
    list(APPEND SPRDRendsaurusRendererIncludeDirectories "${SPDir}/rd-rendsaurus")

	# Misc files
	set(SPRDRendsaurusCommonFiles
		"${SPDir}/qcommon/matcomp.cpp"
		"${SPDir}/qcommon/q_math.cpp"
		"${SPDir}/qcommon/q_shared.cpp"
		"${SPDir}/qcommon/q_shared.h"
		"${SPDir}/qcommon/q_platform.h"
		)
	source_group("common" FILES ${SPRDRendsaurusCommonFiles})
	set(SPRDRendsaurusFiles ${SPRDRendsaurusFiles} ${SPRDRendsaurusCommonFiles})

	# main files
	set(SPRDRendsaurusMainFiles
		"${SPDir}/rd-rendsaurus/glext.h"
		"${SPDir}/rd-rendsaurus/qgl.h"
		"${SPDir}/rd-rendsaurus/tr_arb.cpp"
		"${SPDir}/rd-rendsaurus/tr_backend.cpp"
		"${SPDir}/rd-rendsaurus/tr_bsp.cpp"
		"${SPDir}/rd-rendsaurus/tr_cmds.cpp"
		"${SPDir}/rd-rendsaurus/tr_curve.cpp"
		"${SPDir}/rd-rendsaurus/tr_draw.cpp"
		"${SPDir}/rd-rendsaurus/tr_ghoul2.cpp"
		"${SPDir}/rd-rendsaurus/tr_image.cpp"
		"${SPDir}/rd-rendsaurus/tr_init.cpp"
		"${SPDir}/rd-rendsaurus/tr_light.cpp"
		"${SPDir}/rd-rendsaurus/tr_local.h"
		"${SPDir}/rd-rendsaurus/tr_main.cpp"
		"${SPDir}/rd-rendsaurus/tr_marks.cpp"
		"${SPDir}/rd-rendsaurus/tr_mesh.cpp"
		"${SPDir}/rd-rendsaurus/tr_model.cpp"
		"${SPDir}/rd-rendsaurus/tr_quicksprite.cpp"
		"${SPDir}/rd-rendsaurus/tr_quicksprite.h"
		"${SPDir}/rd-rendsaurus/tr_scene.cpp"
		"${SPDir}/rd-rendsaurus/tr_shade.cpp"
		"${SPDir}/rd-rendsaurus/tr_shade_calc.cpp"
		"${SPDir}/rd-rendsaurus/tr_shader.cpp"
		"${SPDir}/rd-rendsaurus/tr_shadows.cpp"
		"${SPDir}/rd-rendsaurus/tr_skin.cpp"
		"${SPDir}/rd-rendsaurus/tr_sky.cpp"
		"${SPDir}/rd-rendsaurus/tr_stl.cpp"
		"${SPDir}/rd-rendsaurus/tr_stl.h"
		"${SPDir}/rd-rendsaurus/tr_subs.cpp"
		"${SPDir}/rd-rendsaurus/tr_surface.cpp"
		"${SPDir}/rd-rendsaurus/tr_surfacesprites.cpp"
		"${SPDir}/rd-rendsaurus/tr_world.cpp"
		"${SPDir}/rd-rendsaurus/tr_WorldEffects.cpp"
		"${SPDir}/rd-rendsaurus/tr_WorldEffects.h"
		)
	source_group("renderer" FILES ${SPRDRendsaurusMainFiles})
	set(SPRDRendsaurusFiles ${SPRDRendsaurusFiles} ${SPRDRendsaurusMainFiles})

	set(SPRDRendsaurusRdCommonFiles
		"${SPDir}/rd-common/mdx_format.h"
		"${SPDir}/rd-common/tr_common.h"
		"${SPDir}/rd-common/tr_font.cpp"
		"${SPDir}/rd-common/tr_font.h"
		"${SPDir}/rd-common/tr_image_load.cpp"
		"${SPDir}/rd-common/tr_image_jpg.cpp"
		"${SPDir}/rd-common/tr_image_tga.cpp"
		"${SPDir}/rd-common/tr_image_png.cpp"
		"${SPDir}/rd-common/tr_noise.cpp"
		"${SPDir}/rd-common/tr_public.h"
		"${SPDir}/rd-common/tr_types.h")
	source_group("rd-common" FILES ${SPRDRendsaurusRdCommonFiles})
	set(SPRDRendsaurusFiles ${SPRDRendsaurusFiles} ${SPRDRendsaurusRdCommonFiles})

	find_package(OpenGL REQUIRED)
	set(SPRDRendsaurusRendererIncludeDirectories ${SPRDRendsaurusRendererIncludeDirectories} ${OPENGL_INCLUDE_DIR})
	set(SPRDRendsaurusRendererLibraries ${SPRDRendsaurusRendererLibraries} ${OPENGL_LIBRARIES})

	set(SPRDRendsaurusRendererIncludeDirectories ${SPRDRendsaurusRendererIncludeDirectories} ${OpenJKLibDir})

	function(add_sp_renderer_project ProjectName Label EngineName InstallDir Component)
		add_library(${ProjectName} SHARED ${SPRDRendsaurusFiles})
		if(NOT MSVC)
			# remove "lib" prefix for .so/.dylib files
			set_target_properties(${ProjectName} PROPERTIES PREFIX "")
		endif()

		if(WIN32)
			install(TARGETS ${ProjectName}
				RUNTIME
				DESTINATION ${InstallDir}
				COMPONENT ${Component})
		else(WIN32)
			if(MakeApplicationBundles AND (BuildSPEngine OR BuildJK2SPEngine))
				install(TARGETS ${ProjectName}
					LIBRARY
					DESTINATION "${InstallDir}/${EngineName}.app/Contents/MacOS/"
					COMPONENT ${Component})
			else()
				install(TARGETS ${ProjectName}
					LIBRARY
					DESTINATION ${InstallDir}
					COMPONENT ${Component})
			endif()
		endif()

		set_target_properties(${ProjectName} PROPERTIES COMPILE_DEFINITIONS "${SPRDRendsaurusDefines}")

		# Hide symbols not explicitly marked public.
		set_property(TARGET ${ProjectName} APPEND PROPERTY COMPILE_OPTIONS ${OPENJK_VISIBILITY_FLAGS})

		set_target_properties(${ProjectName} PROPERTIES INCLUDE_DIRECTORIES "${SPRDRendsaurusRendererIncludeDirectories}")
		set_target_properties(${ProjectName} PROPERTIES PROJECT_LABEL ${Label})

		target_link_libraries(${ProjectName} ${SPRDRendsaurusRendererLibraries})
	endfunction(add_sp_renderer_project)

	add_sp_renderer_project(${SPRDRendsaurusRenderer} "SP Rendsaurus Renderer" ${SPEngine} ${JKAInstallDir} ${JKASPClientComponent})

endif(BuildSPRdRendsaurus)
