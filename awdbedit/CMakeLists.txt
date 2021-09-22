set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake" "${CMAKE_SOURCE_DIR}/cmake/Hardening")

#include(CPackComponent)
#include(CMakePackageConfigHelpers)
#include(GNUInstallDirs)
#include(Hardening)

set(Include_dir "${CMAKE_CURRENT_SOURCE_DIR}")

set(LZH_ENGINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lzhEngine")
add_subdirectory("${LZH_ENGINE_DIR}")

file(GLOB HDRFILES "${Include_dir}/*.h" "${Include_dir}/*.hpp")

file(GLOB SRCFILES "${CMAKE_CURRENT_SOURCE_DIR}/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB RCFILES "${CMAKE_CURRENT_SOURCE_DIR}/*.rc")

add_executable(awdbedit WIN32 "${SRCFILES}" "${RCFILES}")
target_include_directories(awdbedit PUBLIC "${Include_dir}")

target_link_libraries(awdbedit PRIVATE comctl32 lzh_engine)

posix_and_wine_fixes(awdbedit)
target_compile_definitions(awdbedit PRIVATE "-DAWDBEDIT_EXPORTS=1")

set_target_properties(awdbedit
	PROPERTIES
	ENABLE_EXPORTS ON
	WINDOWS_EXPORT_ALL_SYMBOLS ON
)

install(TARGETS awdbedit
	RUNTIME
)
