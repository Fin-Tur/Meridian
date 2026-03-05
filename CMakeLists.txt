cmake_minimum_required(VERSION 3.10.0)
project(Meridian VERSION 0.1.0 LANGUAGES C CXX)

add_executable(Meridian 
    src/main.cpp
    src/models/assets.h
    src/thirdparty/httplib.h
    src/thirdparty/json.hpp
    src/data/MarketDataFetcher.h
    )

find_package(OpenSSL REQUIRED)
target_link_libraries(Meridian PRIVATE OpenSSL::SSL OpenSSL::Crypto)
