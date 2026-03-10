#include <iostream>
#include "data/MarketDataFetcher.h"
#include "computing/asset_computing.h"
#include "computing/monte_carlo_engine.h"

int main() {
    /*std::string symbol = "AAPL";
    assets::asset a = data_fetcher::fetch_stock(symbol);

    std::string coinId = "bitcoin";
    std::string btcSymbol = "BTC";
    assets::asset btc = data_fetcher::fetch_crypto(coinId, btcSymbol);

    std::vector<assets::asset> assets;
    assets.push_back(std::move(a));
    assets.push_back(std::move(btc));
    std::vector<double> weights = {0.5, 0.5};
    auto preset = monte_carlo::generate_sim_preset(assets, weights, 10000, 252);
    auto res = monte_carlo::run_simulation(preset);*/

    auto res = data_fetcher::fetch_fx_rate(convert_curr_fstr("USD"), convert_curr_fstr("EUR"));
    std::cout << "USD to EUR exchange rate: " << res << std::endl;


    return 0;
}