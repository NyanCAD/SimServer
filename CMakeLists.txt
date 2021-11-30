project("SimulationServer" CXX)
cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard to be used")

find_package(CapnProto CONFIG REQUIRED)

capnp_generate_cpp(SimulatorSources SimulatorHeaders Simulator.capnp)
add_executable(SimServer main.cpp ${SimulatorSources})
target_link_libraries(SimServer PRIVATE CapnProto::capnp-rpc ${CMAKE_DL_LIBS})
target_include_directories(SimServer PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
install(TARGETS SimServer)
