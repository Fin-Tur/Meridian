#pragma once
#include <cstdint>
#include <string>

namespace assets {
    
    struct data_point {
        double low;
        double high;
        double adjclose;
        uint32_t timestamp;
    };

    struct data_point_red{
        double adj_close;
    };


    struct asset {
        std::string symbol;
        std::string currency;
        int32_t n_data_points;
        data_point* data_points;
        double weight = 1.0;
        ~asset() {
            delete[] data_points;
        }
    };
}