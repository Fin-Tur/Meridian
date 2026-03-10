#pragma once
#include <cstdint>
#include <string>
#include <algorithm>

#include "currency.h"

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

    void unify_asset_currencies(std::vector<asset>& assets, currency target_currency){
        std::map<currency, double> fx_cache; // Cache for exchange rates to avoid redundant API calls
        for(auto& asset : assets){
            if(asset.currency != target_currency){
                auto [it, inserted] = fx_cache.try_emplace(asset.currency, data_fetcher::fetch_fx_rate(asset.currency, target_currency));
                if(it.second == -1) throw std::runtime_error("Failed to fetch exchange rate for currency conversion.");
                for(size_t i = 0; i < asset.n_data_points; i++){
                    asset.data_points[i].low *= it.second;
                    asset.data_points[i].high *= it.second;
                    asset.data_points[i].adjclose *= it.second;
                }
                asset.currency = target_currency;
            }
        }
    }
}