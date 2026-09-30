cmake_minimum_required(VERSION 3.20)
project(wxengine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(WXE_WITH_SIMCONNECT "Build the MSFS SimConnect client (Windows only)" ON)
option(WXE_BUILD_TESTS "Build unit tests" ON)
set(MSFS_SDK "C:/MSFS 2024 SDK" CACHE PATH "MSFS 2024 SDK root (contains 'SimConnect SDK')")

include(FetchContent)
FetchContent_Declare(json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_Declare(httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.18.3
  GIT_SHALLOW TRUE)
set(HTTPLIB_REQUIRE_OPENSSL OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_ZLIB_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_BROTLI_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(json httplib)

add_library(wxe_core STATIC
  src/config.cpp
  src/http_util.cpp
  src/geo.cpp
  src/simbrief.cpp
  src/open_meteo.cpp
  src/stratawx_provider.cpp
  src/winds.cpp
  src/airports.cpp
  src/metar.cpp
  src/uplink.cpp
  src/server.cpp)
target_include_directories(wxe_core PUBLIC src)
target_link_libraries(wxe_core PUBLIC nlohmann_json::nlohmann_json httplib::httplib)

if(WIN32)
  target_sources(wxe_core PRIVATE src/http_client_winhttp.cpp)
  target_link_libraries(wxe_core PUBLIC winhttp ws2_32)
  target_compile_definitions(wxe_core PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX _WIN32_WINNT=0x0A00)
else()
  # Non-Windows builds exist for development and tests; HTTPS needs OpenSSL.
  find_package(OpenSSL REQUIRED)
  target_sources(wxe_core PRIVATE src/http_client_httplib.cpp)
  target_link_libraries(wxe_core PUBLIC OpenSSL::SSL OpenSSL::Crypto)
endif()

if(WIN32 AND WXE_WITH_SIMCONNECT)
  set(SIMCONNECT_DIR "${MSFS_SDK}/SimConnect SDK")
  if(NOT EXISTS "${SIMCONNECT_DIR}/include/SimConnect.h")
    message(FATAL_ERROR "SimConnect.h not found under ${SIMCONNECT_DIR}. Set -DMSFS_SDK=... or -DWXE_WITH_SIMCONNECT=OFF")
  endif()
  target_sources(wxe_core PRIVATE src/sim_simconnect.cpp)
  target_include_directories(wxe_core PUBLIC "${SIMCONNECT_DIR}/include")
  target_link_libraries(wxe_core PUBLIC "${SIMCONNECT_DIR}/lib/SimConnect.lib")
else()
  target_sources(wxe_core PRIVATE src/sim_null.cpp)
endif()

add_executable(wxengine src/main.cpp)
target_link_libraries(wxengine PRIVATE wxe_core)
if(WIN32 AND WXE_WITH_SIMCONNECT)
  add_custom_command(TARGET wxengine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${SIMCONNECT_DIR}/lib/SimConnect.dll" $<TARGET_FILE_DIR:wxengine>)
endif()

if(WXE_BUILD_TESTS)
  FetchContent_Declare(doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.11
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(doctest)
  enable_testing()
  add_executable(wxe_tests tests/test_main.cpp)
  target_link_libraries(wxe_tests PRIVATE wxe_core doctest::doctest)
  target_compile_definitions(wxe_tests PRIVATE WXE_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")
  add_test(NAME wxe_tests COMMAND wxe_tests)
endif()
