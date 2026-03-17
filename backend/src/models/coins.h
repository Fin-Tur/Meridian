#pragma once
#include <string>
#include <unordered_map>

namespace coin {
    static const std::unordered_map<std::string, std::string> ticker_to_id_coingecko = {
        {"BTC",  "bitcoin"},
        {"ETH",  "ethereum"},
        {"SOL",  "solana"},
        {"BNB",  "binancecoin"},
        {"XRP",  "ripple"},
        {"ADA",  "cardano"},
        {"AVAX", "avalanche-2"},
        {"DOGE", "dogecoin"},
        {"DOT",  "polkadot"},
        {"MATIC","matic-network"},
        {"LINK", "chainlink"},
        {"UNI",  "uniswap"},
        {"LTC",  "litecoin"},
        {"ATOM", "cosmos"},
        {"XLM",  "stellar"},
        {"ALGO", "algorand"},
        {"VET",  "vechain"},
        {"FIL",  "filecoin"},
        {"TRX",  "tron"},
        {"ETC",  "ethereum-classic"}
    };

    static const std::unordered_map<std::string, std::string> ticker_to_id = {
        {"BTC",  "XBTUSD"},
        {"ETH",  "ETHUSD"},
        {"SOL",  "SOLUSD"},
        {"BNB",  "BNBUSD"},
        {"XRP",  "XRPUSD"},
        {"ADA",  "ADAUSD"},
        {"AVAX", "AVAXUSD"},
        {"DOGE", "XDGUSD"},
        {"DOT",  "DOTUSD"},
        {"MATIC","MATICUSD"},
        {"LINK", "LINKUSD"},
        {"UNI",  "UNIUSD"},
        {"LTC",  "LTCUSD"},
        {"ATOM", "ATOMUSD"},
        {"XLM",  "XLMUSD"},
        {"ALGO", "ALGOUSD"},
        {"VET",  "VETUSD"},
        {"FIL",  "FILUSD"},
        {"TRX",  "TRXUSD"},
        {"ETC",  "ETCUSD"}
    };

    bool is_crypto(const std::string& ticker) {
        return ticker_to_id.find(ticker) != ticker_to_id.end();
    }

    std::string to_coin_id(const std::string& ticker) {
        auto it = ticker_to_id.find(ticker);
        if (it != ticker_to_id.end()) {
            return it->second;
        }
        return "";
    }
}