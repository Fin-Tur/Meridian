#include <iostream>
#include "data/MarketDataFetcher.h"

int main() {
    std::string symbol = "AAPL";
    assets::asset a = data_fetcher::fetch_stock(symbol);

    std::cout << "Symbol:   " << a.symbol << "\n";
    std::cout << "Currency: " << a.currency << "\n";
    std::cout << "Points:   " << a.n_data_points << "\n\n";

    // Erste und letzte 3 Datenpunkte ausgeben
    for (size_t i = 0; i < 3; i++) {
        std::cout << "high: " << a.data_points[i].high
                  << "  low: "  << a.data_points[i].low
                  << "  close: "<< a.data_points[i].adjclose << "\n";
    }

    std::cout << "...\n";

    for (size_t i = a.n_data_points - 3; i < a.n_data_points; i++) {
        std::cout << "high: " << a.data_points[i].high
                  << "  low: "  << a.data_points[i].low
                  << "  close: "<< a.data_points[i].adjclose << "\n";
    }

    // Crypto
    std::string coinId = "bitcoin";
    std::string btcSymbol = "BTC";
    assets::asset btc = data_fetcher::fetch_crypto(coinId, btcSymbol);

    std::cout << "\nSymbol:   " << btc.symbol << "\n";
    std::cout << "Currency: " << btc.currency << "\n";
    std::cout << "Points:   " << btc.n_data_points << "\n";
    return 0;
}