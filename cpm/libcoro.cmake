include(CPM)
CPMAddPackage(
  NAME libcoro
  VERSION 0.6
  OPTIONS
    "LIBCORO_BUILD_TESTS OFF"
    "LIBCORO_BUILD_EXAMPLES OFF"
  GIT_REPOSITORY https://github.com/jbaldwin/libcoro.git
  GIT_TAG v0.6
)
