include(ExternalProject)
include(FetchContent)

ExternalProject_Add(
	maestro
	GIT_REPOSITORY https://github.com/QoroQuantum/maestro.git
	GIT_TAG main
	UPDATE_COMMAND ""
	CONFIGURE_COMMAND ""
	BUILD_COMMAND bash "${CMAKE_BINARY_DIR}/maestro-prefix/src/maestro/build.sh"
	BUILD_IN_SOURCE true
	INSTALL_COMMAND cp "${CMAKE_BINARY_DIR}/maestro-prefix/src/maestro/build/libmaestro.so" "${CMAKE_BINARY_DIR}/maestro.so"
	INSTALL_DIR ${CMAKE_BINARY_DIR}
	)

