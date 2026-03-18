#include <iostream>
#include "data/MarketDataFetcher.h"
#include "computing/asset_computing.h"
#include "computing/monte_carlo_engine.h"
#include "api/server.h"

int main() {

    //server s = server();
    //s.run("localhost", 8080);

    std::vector<assets::asset> assets;
    std::string symbols[] = {"AAPL", "MSFT", "GOOGL"};
    for(auto& sym : symbols){
        assets::asset a = data_fetcher::fetch_stock(sym);
        asset_compute::compute_log_return_for_asset(a);
        assets.push_back(a);
    }

    auto m = asset_compute::compute_covariance_matrix(assets, false);
    asset_compute::normalize_covariance_matrix(m);
    std::cout << "Covariance Matrix:" << std::endl;
    std::cout << m << std::endl;

    auto ms = asset_compute::compute_covariance_matrix(assets, true);
    asset_compute::normalize_covariance_matrix(ms);
    std::cout << "Stressed Covariance Matrix:" << std::endl;
    std::cout << ms << std::endl;
    return 0;
}