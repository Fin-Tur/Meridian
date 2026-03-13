cmake_minimum_required(VERSION 3.10.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(Meridian VERSION 0.1.0 LANGUAGES C CXX)

add_executable(Meridian 
    src/main.cpp
    src/models/assets.h
    src/thirdparty/httplib.h
    src/thirdparty/json.hpp
    src/data/MarketDataFetcher.h
    src/api/server.h
    src/computing/asset_computing.h
    src/computing/monte_Carlo_engine.h
    src/models/currency.h

    )

find_package(OpenSSL REQUIRED)
find_package(Eigen3 REQUIRED)
target_link_libraries(Meridian PRIVATE OpenSSL::SSL OpenSSL::Crypto Eigen3::Eigen)

if (WIN32)
    target_link_libraries(Meridian PRIVATE ws2_32 crypt32)
    target_compile_definitions(Meridian PRIVATE
        WINVER=0x0A00
        _WIN32_WINNT=0x0A00
    )
endif()
