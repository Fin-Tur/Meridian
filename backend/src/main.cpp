#include <iostream>
#include "data/MarketDataFetcher.h"
#include "computing/asset_computing.h"
#include "computing/monte_carlo_engine.h"
#include "api/server.h"

int main() {

    server s = server();
    s.run("localhost", 8080);

    /*std::vector<assets::asset> assets;
    std::string symbols[] = {"AAPL", "MSFT", "GOOGL"};
    for(auto& sym : symbols){
        assets::asset a = data_fetcher::fetch_stock(sym);
        asset_compute::compute_log_return_for_asset(a);
        assets.push_back(a);
    }

    monte_carlo::sim_config config;
    config.drift_scenario = monte_carlo::drift_scenario::SHRINKAGE_25;
    config.vol_model = monte_carlo::volatility_model::EWMA_75;
    config.multivariate_t = true;

    monte_carlo::sim_preset preset = monte_carlo::generate_sim_preset(assets, {0.4, 0.4, 0.2}, 10000, 21, config);
    auto result = monte_carlo::run_simulation(preset, config);

    std::cout << "VaR 95%: " << result.var_95 << std::endl;
    std::cout << "VaR 99%: " << result.var_99 << std::endl;
    std::cout << "CVaR 95%: " << result.cvar_95 << std::endl;
    std::cout << "CVaR 99%: " << result.cvar_99 << std::endl;
    std::cout << "Median: " << result.median << std::endl;*/

    /*auto m = asset_compute::compute_covariance_matrix(assets, false);
    asset_compute::normalize_covariance_matrix(m);
    std::cout << "Covariance Matrix:" << std::endl;
    std::cout << m << std::endl;

    auto ms = asset_compute::compute_covariance_matrix(assets, true);
    asset_compute::normalize_covariance_matrix(ms);
    std::cout << "Stressed Covariance Matrix:" << std::endl;
    std::cout << ms << std::endl;*/
    return 0;
}