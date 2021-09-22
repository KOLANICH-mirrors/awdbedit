cmake_minimum_required(VERSION 3.7.2)

set(CMAKE_USE_RELATIVE_PATHS TRUE)
project("awdbedit")

set("PROJECT_DESCRIPTION" "A GUI editor for legacy Award/Phoenix BIOSes for Windows.")
set("PROJECT_HOMEPAGE_URL" "https://github.com/KOLANICH/{PROJECT_NAME}")

set(CPACK_PACKAGE_VENDOR "Michael Tedder")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "1")
set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

set(OUR_CMAKE_MODULES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(OUR_CMAKE_3PARTY_MODULES_DIR "${OUR_CMAKE_MODULES_DIR}/thirdParty")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${OUR_CMAKE_MODULES_DIR}" "${OUR_CMAKE_MODULES_DIR}/Boilerplate" "${OUR_CMAKE_MODULES_DIR}/DoxygenUtils" "${OUR_CMAKE_3PARTY_MODULES_DIR}")


include(Boilerplate)
include(DoxygenUtils)

include(CheckIncludeFileCXX)
include(CheckStructHasMember)
include(CheckSymbolExists)
include(CheckCXXSymbolExists)

function(invalidateWinAPIHeadersPresence)
	unset(HAVE_IO_H CACHE)
	unset(HAVE_DOS_H CACHE)
	unset(HAVE_PROCESS_H CACHE)
	unset(HAVE_DIRECT_H CACHE)

	unset(HAVE_UNISTD_H CACHE)
	unset(HAVE_LIBGEN_H CACHE)
	unset(HAVE_UNISTD_H CACHE)
	unset(HAVE_DIRENT_H CACHE)
	unset(DIRENT_HAS_D_NAME CACHE)
	unset(DIRENT_DT_DIR_EXISTS CACHE)
	unset(HAVE_GLOB_H CACHE)
	unset(HAVE_SYS_STAT_H CACHE)
	unset(TIME_T_HAS_TV_SEC CACHE)
endfunction()

function(detectWinAPIHeadersPresence)
	CHECK_INCLUDE_FILE_CXX("io.h" HAVE_IO_H)  # DOS/Windows native header
	CHECK_INCLUDE_FILE_CXX("dos.h" HAVE_DOS_H)  # DOS/Windows native header
	CHECK_INCLUDE_FILE_CXX("process.h" HAVE_PROCESS_H)  # Windows native header
	CHECK_INCLUDE_FILE_CXX("direct.h" HAVE_DIRECT_H)  # Windows native header
	if(HAVE_DIRECT_H)
		check_cxx_symbol_exists("chdir" "direct.h" DIRECT_H_DEFINES_CHDIR_NO_UNDERSCORE)
		check_cxx_symbol_exists("getcwd" "direct.h" DIRECT_H_DEFINES_GETCWD_NO_UNDERSCORE)
		check_cxx_symbol_exists("mkdir" "direct.h" DIRECT_H_DEFINES_MKDIR_NO_UNDERSCORE)
		check_cxx_symbol_exists("rmdir" "direct.h" DIRECT_H_DEFINES_RMDIR_NO_UNDERSCORE)
	endif()

	CHECK_INCLUDE_FILE_CXX("unistd.h" HAVE_UNISTD_H) # POSIX header
	CHECK_INCLUDE_FILE_CXX("libgen.h" HAVE_LIBGEN_H) # POSIX header
	CHECK_INCLUDE_FILE_CXX("dirent.h" HAVE_DIRENT_H) # POSIX header
	CHECK_INCLUDE_FILE_CXX("glob.h" HAVE_GLOB_H) # POSIX header
	CHECK_INCLUDE_FILE_CXX("sys/stat.h" HAVE_SYS_STAT_H) # POSIX header

	CHECK_STRUCT_HAS_MEMBER("time_t" "tv_sec" "time.h" TIME_T_HAS_TV_SEC LANGUAGE CXX) # both POSIX and windows header. Though different even between POSIX MinGW and POSIX linux

	if(HAVE_DIRENT_H)
		CHECK_STRUCT_HAS_MEMBER("dirent" "d_name" "dirent.h" DIRENT_HAS_D_NAME LANGUAGE CXX)
		check_symbol_exists("DT_DIR" "dirent.h" "DIRENT_DT_DIR_EXISTS")
		if(DIRENT_HAS_D_NAME AND DIRENT_DT_DIR_EXISTS)
			set(DIRENT_IS_OK_FOR_OUR_PURPOSES ON)
		endif()
	endif()
endfunction()

function(detectIfWinelibCompiler)
	message(STATUS "Detecting if WineGCC/WineClang")
	if(WINE)
	else()
		check_cxx_symbol_exists("__WINE__" "" WINE)
	endif()

	if(WINE)
		set(WINE ON PARENT_SCOPE)
	endif()
endfunction()

function(detectCompilerFeaturesRelevantForTargetsUsingWinAPI)
	detectIfWinelibCompiler()
	if(WINE)
		set(WINE ON PARENT_SCOPE)
		option(WINE_USE_NO_CYGWIN "Appends -mno-cygwin to wine flags. Allows to use some windows libs instead: " OFF)  # requires us to have mechanisms invalidating cache of HAVE_* in the case of WINE_USE_NO_CYGWIN has changed

		if(WINE_USE_NO_CYGWIN STREQUAL WINE_USE_NO_CYGWIN_BACKUP)
		else()
			message(STATUS "WINE_USE_NO_CYGWIN has changed, invalidating some header-checking variables")

			if(WINE_USE_NO_CYGWIN)
				set(CMAKE_REQUIRED_FLAGS_BACKUP "${CMAKE_REQUIRED_FLAGS}")
				list(APPEND CMAKE_REQUIRED_FLAGS "-mno-cygwin")
			endif()
			invalidateWinAPIHeadersPresence()
			detectWinAPIHeadersPresence()

			if(WINE_USE_NO_CYGWIN)
				set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS_BACKUP}")
			endif()

			set(WINE_USE_NO_CYGWIN_BACKUP "${WINE_USE_NO_CYGWIN}" CACHE INTERNAL "A cache of WINE_USE_NO_CYGWIN for the purpose of checking if it has changed" FORCE)
		endif()
	else()
		detectWinAPIHeadersPresence()
	endif()
endfunction()
detectCompilerFeaturesRelevantForTargetsUsingWinAPI()

function(posix_and_wine_fixes target)
	if(WINE)
		message(STATUS "The compiler is winegcc. We will have to explicitly link the needed libs")
		target_link_libraries(${target} PRIVATE comdlg32 gdi32 shell32)
		if(WINE_USE_NO_CYGWIN)
			target_compile_options(${target} PRIVATE "-mno-cygwin")
		endif()
	endif()

	if(HAVE_DIRENT_H)
		target_compile_definitions(${target} PRIVATE -DHAVE_DIRENT_H=1)
		if(DIRENT_IS_OK_FOR_OUR_PURPOSES)
			target_compile_definitions(${target} PRIVATE -DDIRENT_IS_OK_FOR_OUR_PURPOSES=1)
		endif()
	endif()

	if(HAVE_IO_H)
		target_compile_definitions(${target} PRIVATE -DHAVE_IO_H=1)
	endif()
	if(HAVE_PROCESS_H)
		target_compile_definitions(${target} PRIVATE -DHAVE_PROCESS_H=1)
	endif()
	if(HAVE_DOS_H)
		target_compile_definitions(${target} PRIVATE -DHAVE_DOS_H=1)
	endif()
	if(HAVE_DIRECT_H)
		target_compile_definitions(${target} PRIVATE -DHAVE_DIRECT_H=1)
		if(DIRECT_H_DEFINES_CHDIR_NO_UNDERSCORE)
			target_compile_definitions(${target} PRIVATE -DDIRECT_H_DEFINES_CHDIR_NO_UNDERSCORE=1)
		endif()
		if(DIRECT_H_DEFINES_GETCWD_NO_UNDERSCORE)
			target_compile_definitions(${target} PRIVATE -DDIRECT_H_DEFINES_GETCWD_NO_UNDERSCORE=1)
		endif()
		if(DIRECT_H_DEFINES_MKDIR_NO_UNDERSCORE)
			target_compile_definitions(${target} PRIVATE -DDIRECT_H_DEFINES_MKDIR_NO_UNDERSCORE=1)
		endif()
		if(DIRECT_H_DEFINES_RMDIR_NO_UNDERSCORE)
			target_compile_definitions(${target} PRIVATE -DDIRECT_H_DEFINES_RMDIR_NO_UNDERSCORE=1)
		endif()
	endif()

	if(HAVE_UNISTD_H)
		target_compile_definitions(${target} PRIVATE -DHAVE_UNISTD_H=1)
	endif()
	if(HAVE_GLOB_H)
		target_compile_definitions(${target} PRIVATE -DHAVE_GLOB_H=1)
	endif()
	if(HAVE_SYS_STAT_H)
		target_compile_definitions(${target} PRIVATE -DHAVE_SYS_STAT_H=1)
	endif()
	if(HAVE_LIBGEN_H)
		target_compile_definitions(${target} PRIVATE -DHAVE_LIBGEN_H=1)
	endif()

	if(TIME_T_HAS_TV_SEC)
		target_compile_definitions(${target} PRIVATE -DTIME_T_HAS_TV_SEC=1)
	endif()
endfunction()

set(CPACK_PACKAGE_DESCRIPTION "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_HOMEPAGE_URL "${PROJECT_HOMEPAGE_URL}")
set(CPACK_PACKAGE_MAINTAINER "${CPACK_PACKAGE_VENDOR}")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_MAINTAINER}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING.md")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/ReadMe.md")
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_RPM_COMPONENT_INSTALL ON)
set(CPACK_NSIS_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_ENABLE_COMPONENT_DEPENDS ON)
set(CPACK_RPM_ENABLE_COMPONENT_DEPENDS ON)
set(CPACK_DEBIAN_COMPRESSION_TYPE "xz")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS ON)
set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS_POLICY ">=")

add_subdirectory("awdbedit")
add_subdirectory("awdbe_builtins")
add_subdirectory("awdbe_wavedit")

include(CPack)
