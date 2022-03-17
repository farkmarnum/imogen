# ======================================================================================
#
#  ██╗     ███████╗███╗   ███╗ ██████╗ ███╗   ██╗███████╗
#  ██║     ██╔════╝████╗ ████║██╔═══██╗████╗  ██║██╔════╝
#  ██║     █████╗  ██╔████╔██║██║   ██║██╔██╗ ██║███████╗
#  ██║     ██╔══╝  ██║╚██╔╝██║██║   ██║██║╚██╗██║╚════██║
#  ███████╗███████╗██║ ╚═╝ ██║╚██████╔╝██║ ╚████║███████║
#  ╚══════╝╚══════╝╚═╝     ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚══════╝
#
#  This file is part of the Lemons open source library and is licensed under the terms of the GNU Public License.
#
# ======================================================================================

include_guard (GLOBAL)

if(NOT COMMAND CPMAddPackage)
	if(NOT DEFINED ENV{CPM_SOURCE_CACHE})
		if(CPM_Lemons_SOURCE)
			set (ENV{CPM_SOURCE_CACHE} "${CPM_Lemons_SOURCE}/Cache")
		else()
			set (ENV{CPM_SOURCE_CACHE} "${CMAKE_SOURCE_DIR}/Cache")
		endif()
	endif()

	list (APPEND CMAKE_PREFIX_PATH $ENV{CPM_SOURCE_CACHE})
	set (CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" CACHE INTERNAL "")

	set (LEMONS_CPM_PATH "$ENV{CPM_SOURCE_CACHE}/CPM.cmake" CACHE PATH
																  "Path to the CPM.cmake script")
	mark_as_advanced (FORCE LEMONS_CPM_PATH)

	if(NOT EXISTS ${LEMONS_CPM_PATH})
		message (VERBOSE "Downloading CPM.cmake to ${LEMONS_CPM_PATH}")

		file (DOWNLOAD https://raw.githubusercontent.com/cpm-cmake/CPM.cmake/master/cmake/CPM.cmake
			  ${LEMONS_CPM_PATH})
	endif()

	include ("${LEMONS_CPM_PATH}")
endif()

set(LEMONS_GIT_TAG $ENV{LEMONS_GIT_TAG})

if(NOT LEMONS_GIT_TAG)
    set(LEMONS_GIT_TAG origin/main)
endif()

cpmaddpackage (NAME Lemons GITHUB_REPOSITORY benthevining/Lemons GIT_TAG ${LEMONS_GIT_TAG})

list (APPEND CMAKE_MODULE_PATH ${LEMONS_CMAKE_MODULE_PATH})

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} CACHE INTERNAL "")
