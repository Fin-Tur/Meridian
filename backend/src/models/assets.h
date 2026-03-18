#pragma once
#include <cstdint>
#include <string>
#include <algorithm>

#include "currency.h"
#include "../data/MarketDataFetcher.h"

namespace assets {
    
    struct data_point {
        double adjclose;
        double log_return;
        uint32_t timestamp;
    };


    struct asset {
        std::string symbol;
        currency currency;
        int32_t n_data_points = 0;
        data_point* data_points = nullptr;
        double weight = 1.0;

        asset() = default;

        asset(const asset& other)
            : symbol(other.symbol), currency(other.currency),
              n_data_points(other.n_data_points), weight(other.weight) {
            data_points = new data_point[n_data_points];
            std::copy(other.data_points, other.data_points + n_data_points, data_points);
        }

        // Move constructor — transfers ownership, leaves source in valid empty state
        asset(asset&& other) noexcept
            : symbol(std::move(other.symbol)), currency(std::move(other.currency)),
              n_data_points(other.n_data_points), data_points(other.data_points),
              weight(other.weight) {
            other.data_points = nullptr;
            other.n_data_points = 0;
        }

        asset& operator=(const asset& other) {
            if (this != &other) {
                delete[] data_points;
                symbol = other.symbol;
                currency = other.currency;
                n_data_points = other.n_data_points;
                weight = other.weight;
                data_points = new data_point[n_data_points];
                std::copy(other.data_points, other.data_points + n_data_points, data_points);
            }
            return *this;
        }

        asset& operator=(asset&& other) noexcept {
            if (this != &other) {
                delete[] data_points;
                symbol = std::move(other.symbol);
                currency = std::move(other.currency);
                n_data_points = other.n_data_points;
                data_points = other.data_points;
                weight = other.weight;
                other.data_points = nullptr;
                other.n_data_points = 0;
            }
            return *this;
        }

        ~asset() {
            delete[] data_points;
        }
    };


}