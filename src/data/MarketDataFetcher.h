#pragma once
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <map>
#include <vector>
#include <chrono>
#include <stdexcept>
#include "../thirdparty/httplib.h"
#include "../thirdparty/json.hpp"
#include "../models/assets.h"

namespace data_fetcher {

    //TODO limited size + FIFO
    inline std::unordered_map<std::string, std::pair<assets::asset, size_t>> stock_cache;

    assets::asset fetch_stock(std::string& symbol, std::string range = "1y") {
        size_t day = std::chrono::duration_cast<std::chrono::hours>(std::chrono::system_clock::now().time_since_epoch()).count()/24;
        if(stock_cache.contains(symbol)){
            auto& [cached_asset, last_fetched_day] = stock_cache[symbol];
            if(day - last_fetched_day < 1){ //Cache is valid for 1 day
                return cached_asset;
            }
        }

        httplib::SSLClient client("query1.finance.yahoo.com");
        client.set_default_headers({{"User-Agent", "Mozilla/5.0"}});

        std::string path = "/v8/finance/chart/";
        path += symbol;
        path += "?range=" + range + "&interval=1d&includeAdjustedClose=true";

        auto res = client.Get(path.c_str());

        if (!res || res->status != 200) {
            return assets::asset{};
        }

        auto j   = nlohmann::json::parse(res->body);

        auto& quote  = j["chart"]["result"][0]["indicators"]["quote"][0];
        auto& highs  = quote["high"];
        auto& lows   = quote["low"];
        auto& closes = j["chart"]["result"][0]["indicators"]["adjclose"][0]["adjclose"];
        auto& timestamps = j["chart"]["result"][0]["timestamp"];

        assets::asset a;
        a.symbol = symbol;
        a.n_data_points = closes.size();
        a.data_points = new assets::data_point[a.n_data_points];
        a.currency = convert_curr_fstr(j["chart"]["result"][0]["meta"]["currency"].get<std::string>());

        for (size_t i = 0; i < closes.size(); i++) {
            if (closes[i].is_null() || highs[i].is_null() || lows[i].is_null()) continue;
            a.data_points[i] = assets::data_point{
                .low = lows[i].get<double>(),
                .high = highs[i].get<double>(),
                .adjclose = closes[i].get<double>(),
                .timestamp = timestamps[i].get<uint32_t>() / 86400 //Convert to Days
            };
        }
        stock_cache[symbol] = {a, day};
        return a;
}

    // Uses CoinGecko market_chart endpoint with daily interval to ensure timestamp
    // alignment with stock data from Yahoo Finance (both return one data point per calendar day).
    assets::asset fetch_crypto(std::string& coinId, std::string& symbol, uint16_t days = 365) {
        httplib::SSLClient client("api.coingecko.com");

        std::string path = "/api/v3/coins/";
        path += coinId;
        path += "/market_chart?vs_currency=usd&days=" + std::to_string(days) + "&interval=daily";

        auto res = client.Get(path.c_str());

        if (!res || res->status != 200) {
            return assets::asset{};
        }

        auto j = nlohmann::json::parse(res->body);
        auto& prices = j["prices"]; // [[timestamp_ms, price], ...]

        assets::asset a;
        a.symbol = symbol;
        a.n_data_points = prices.size();
        a.data_points = new assets::data_point[a.n_data_points];
        a.currency = convert_curr_fstr("USD"); //Cryptos are typically priced in USD

        for (size_t i = 0; i < prices.size(); i++) {
            if (prices[i][1].is_null()) continue;
            double close_price = prices[i][1].get<double>();
            a.data_points[i] = assets::data_point{
                .low = close_price,
                .high = close_price,
                .adjclose = close_price,
                .timestamp = static_cast<uint32_t>(prices[i][0].get<long long>() / (1000LL * 86400)) //Convert ms to Days
            };
        }
        return a;
    }

    double fetch_fx_rate(const currency& from_currency, currency to_currency = currency::USD) {
        httplib::SSLClient client("query1.finance.yahoo.com");
        client.set_default_headers({{"User-Agent", "Mozilla/5.0"}});

        std::string ticker = convert_curr_tostr(from_currency) + convert_curr_tostr(to_currency) + "=X";
        std::string path = "/v8/finance/chart/"+ticker+"?range=1d&interval=1d";

        auto res = client.Get(path.c_str());

        if (!res || res->status != 200) {
            return -1.0;
        }

        auto j = nlohmann::json::parse(res->body);
        double rate = j["chart"]["result"][0]["meta"]["regularMarketPrice"].get<double>();
        return rate;
    }

    void unify_asset_currencies(std::vector<assets::asset>& asset_list, currency target_currency){
        std::map<currency, double> fx_cache; // Cache for exchange rates to avoid redundant API calls
        for(auto& asset : asset_list){
            if(asset.currency != target_currency){
                auto [it, inserted] = fx_cache.try_emplace(asset.currency, fetch_fx_rate(asset.currency, target_currency));
                if(it->second == -1) throw std::runtime_error("Failed to fetch exchange rate for currency conversion.");
                for(size_t i = 0; i < asset.n_data_points; i++){
                    asset.data_points[i].low *= it->second;
                    asset.data_points[i].high *= it->second;
                    asset.data_points[i].adjclose *= it->second;
                }
                asset.currency = target_currency;
            }
        }
    }

}