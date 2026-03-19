#pragma once

#include <vector>
#include <string>
#include <iostream>
#include "../thirdparty/httplib.h"
#include "../thirdparty/json.hpp"
#include "../models/assets.h"
#include "../data/MarketDataFetcher.h"
#include "../computing/monte_carlo_engine.h"
#include "../computing/asset_computing.h"
#include "../models/currency.h"
#include "../models/coins.h"


//========= DEV ONLY ==========
//Python fast API to come

class server {
    private:
    httplib::Server svr;
    
    public:
    server() {
        svr.set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res){
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        if (req.method == "OPTIONS") {
            res.status = 204;
            return httplib::Server::HandlerResponse::Handled;
        }
        return httplib::Server::HandlerResponse::Unhandled;
    });
    }

    void run(const char* host, int port) {

        svr.Get("/health", [](const httplib::Request&, httplib::Response& res){
            res.set_content(R"({"status":"ok"})", "application/json");
        });

        svr.Post("/simulate", [](const httplib::Request& req, httplib::Response& res){
            std::cout << "Received simulation request with body: " << req.body << std::endl;
            try{
                //Fetch body
                auto body = nlohmann::json::parse(req.body);
                double portfolio_value = body.at("portfolio_value").get<double>();
                size_t horizon_days = body.value("horizon_days", 252);
                size_t n_sims = body.value("n_simulations", 1000);
                monte_carlo::drift_scenario drift_scenario = body.value("drift_scenario", "SHRINKAGE_25") == "SHRINKAGE_25" ? monte_carlo::drift_scenario::SHRINKAGE_25 : 
                                                body.value("drift_scenario", "SHRINKAGE_25") == "ZERO" ? monte_carlo::drift_scenario::ZERO :
                                                body.value("drift_scenario", "SHRINKAGE_25") == "RISK_FREE" ? monte_carlo::drift_scenario::RISK_FREE :
                                                monte_carlo::drift_scenario::HISTORICAL;
                monte_carlo::volatility_model vol_model = body.value("volatility_model", "HISTORICAL") == "HISTORICAL" ? monte_carlo::volatility_model::HISTORICAL :
                                                body.value("volatility_model", "HISTORICAL") == "EWMA_100" ? monte_carlo::volatility_model::EWMA_100 :
                                                body.value("volatility_model", "HISTORICAL") == "EWMA_75" ? monte_carlo::volatility_model::EWMA_75 :
                                                body.value("volatility_model", "HISTORICAL") == "EWMA_50" ? monte_carlo::volatility_model::EWMA_50 :
                                                monte_carlo::volatility_model::HISTORICAL;

            std::vector<assets::asset> fetched_assets;
            std::vector<double> weights;

            for (const auto& a : body.at("assets")) {
                std::string ticker = a.at("ticker").get<std::string>();
                std::string type   = a.at("type").get<std::string>();
                double weight      = a.at("weight").get<double>();

                assets::asset fetched;
                if(coin::is_crypto(ticker)) type = "crypto"; 
                if (type == "crypto") {
                    std::string coinId = coin::to_coin_id(ticker);
                    fetched = data_fetcher::fetch_crypto(coinId, ticker);
                } else {
                    fetched = data_fetcher::fetch_stock(ticker);
                }
                asset_compute::compute_log_return_for_asset(fetched);
                fetched_assets.push_back(fetched);
                weights.push_back(weight);
            }

            // Run Monte Carlo simulation
            
            monte_carlo::sim_config config = {
                .vol_model = vol_model,
                .drift_scenario = drift_scenario,
                .multivariate_t = body.value("multivariate_t", false),
                .regimes = body.value("regimes", false)
            };
            auto preset = monte_carlo::generate_sim_preset(fetched_assets, weights, n_sims, horizon_days,  config, portfolio_value);
            auto result = monte_carlo::run_simulation(preset, config);

            nlohmann::json response_json;   
            response_json["var_95"] = result.var_95;
            response_json["var_99"] = result.var_99;
            response_json["cvar_95"] = result.cvar_95;
            response_json["cvar_99"] = result.cvar_99;
            response_json["avg"] = result.avg;
            response_json["min"] = result.final_portfolio_values.front();
            response_json["max"] = result.final_portfolio_values.back();
            response_json["median"] = result.median;

            size_t trim_upper = static_cast<size_t>(result.final_portfolio_values.size() * 0.9); // Cut off extreme outliers for histogram
            double bin_max = result.final_portfolio_values[trim_upper];
            double bin_min = result.final_portfolio_values[0];
            double bin_width = (bin_max - bin_min) / 50;
            response_json["bin_width"] = bin_width;    
            std::vector<int> histogramm_bins(50);
            for(auto& a : result.final_portfolio_values){
                int bin_index = static_cast<int>((a - bin_min) / bin_width);
                if(bin_index >= 50) bin_index = 49;
                histogramm_bins[bin_index]++;
            }
            response_json["histogram_bins"] = histogramm_bins;
            std::cout << "Simulation completed and response sent:\n" << response_json << std::endl;
            res.set_content(response_json.dump(), "application/json");
            
            } catch(const std::exception& e) {
                res.status = 400;
                res.set_content(nlohmann::json{{"error", e.what()}}.dump(), "application/json");
                std::cerr << "Error processing simulation request: " << e.what() << std::endl;
                return;
            }
        });

        svr.Post("/correlation", [](const httplib::Request& req, httplib::Response& res){
            std::cout << "Received correlation request with body: " << req.body << std::endl;
            try{
                auto body = nlohmann::json::parse(req.body);
                std::vector<assets::asset> fetched_assets;

                for (const auto& a : body.at("assets")) {
                    std::string ticker = a.at("ticker").get<std::string>();
                    std::string type   = a.at("type").get<std::string>();

                    assets::asset fetched;
                    if(coin::is_crypto(ticker)) type = "crypto";
                    if (type == "crypto") {
                        std::string coinId = coin::to_coin_id(ticker);
                        fetched = data_fetcher::fetch_crypto(coinId, ticker);
                    } else {
                        fetched = data_fetcher::fetch_stock(ticker);
                    }
                    asset_compute::compute_log_return_for_asset(fetched);
                    fetched_assets.push_back(fetched);
                }

                nlohmann::json response_json;
                auto cov_matrix = asset_compute::compute_covariance_matrix(fetched_assets);
                asset_compute::normalize_covariance_matrix(cov_matrix);
                for(size_t i = 0; i < fetched_assets.size(); i++){
                    for(size_t j = 0; j < fetched_assets.size(); j++){
                        response_json[fetched_assets[i].symbol][fetched_assets[j].symbol] = cov_matrix(i,j);
                    }
                }
                res.set_content(response_json.dump(), "application/json");

            } catch(const std::exception& e) {
                res.status = 400;
                res.set_content(nlohmann::json{{"error", e.what()}}.dump(), "application/json");
                return;
            }
        });

        svr.Post("/asset", [](const httplib::Request& req, httplib::Response& res){
            try{
                auto body = nlohmann::json::parse(req.body);
                std::string ticker = body.at("ticker").get<std::string>();
                std::string type   = body.at("type").get<std::string>();

                assets::asset fetched;
                if(coin::is_crypto(ticker)) type = "crypto";
                if (type == "crypto") {
                    std::string coinId = coin::to_coin_id(ticker);
                    fetched = data_fetcher::fetch_crypto(coinId, ticker);
                } else {
                    fetched = data_fetcher::fetch_stock(ticker);
                }
                if(fetched.n_data_points == 0){
                    throw std::runtime_error("Failed to fetch market data for: " + ticker);
                }
                asset_compute::compute_log_return_for_asset(fetched);
                std::vector<double> adj_closes;
                adj_closes.reserve(fetched.n_data_points);
                for (int i = 0; i < fetched.n_data_points; i++)
                    adj_closes.push_back(fetched.data_points[i].adjclose);

                nlohmann::json response_json;
                response_json["symbol"] = fetched.symbol;
                response_json["currency"] = convert_curr_tostr(fetched.currency);
                response_json["adj_closes"] = adj_closes;
                response_json["volatility"] = asset_compute::volatility(fetched) * sqrt(252); //Annualized volatility
                response_json["avg_log_return"] = asset_compute::avg_log_return(fetched) * 252; //Annualized average log return
                response_json["sharpe_ratio"] = asset_compute::sharpe_ratio(fetched);
                response_json["max_drawdown"] = asset_compute::max_drawdown(fetched);
                response_json["ytd_return"] = asset_compute::ytd_return(fetched);
                response_json["skewness"] = asset_compute::skewness(fetched);
                response_json["kurtosis"] = asset_compute::excess_kurtosis(fetched) + 3; //Add 3 to get regular kurtosis

                res.set_content(response_json.dump(), "application/json");

            } catch(const std::exception& e) {
                if(std::string(e.what()).find("Failed to fetch market data") != std::string::npos){
                    res.status = 4041;
                    res.set_content(nlohmann::json{{"error", e.what()}}.dump(), "application/json");
                } else {
                    res.status = 400;
                    res.set_content(nlohmann::json{{"error", e.what()}}.dump(), "application/json");
                }
                res.status = 400;
                res.set_content(nlohmann::json{{"error", e.what()}}.dump(), "application/json");
                return;
            }
        });

        svr.Post("/portfolio", [](const httplib::Request& req, httplib::Response& res){
            std::cout << "Received portfolio request with body: " << req.body << std::endl;
            try{
                auto body = nlohmann::json::parse(req.body);
                std::vector<assets::asset> fetched_assets;
                std::vector<double> amounts;

                for (const auto& a : body.at("assets")) {
                    std::string ticker = a.at("ticker").get<std::string>();
                    std::string type   = a.at("type").get<std::string>();
                    double amount      = a.at("amount").get<double>();

                    assets::asset fetched;
                    if(coin::is_crypto(ticker)) type = "crypto";
                    if (type == "crypto") {
                        std::string coinId = coin::to_coin_id(ticker);
                        fetched = data_fetcher::fetch_crypto(coinId, ticker);
                    } else {
                        fetched = data_fetcher::fetch_stock(ticker);
                    }

                    if (fetched.n_data_points == 0) {
                        throw std::runtime_error("Failed to fetch market data for: " + ticker);
                    }
                    asset_compute::compute_log_return_for_asset(fetched);
                    fetched_assets.push_back(fetched);
                    amounts.push_back(amount);
                }

                nlohmann::json response_json;
                double portfolio_value = 0.0;

                data_fetcher::unify_asset_currencies(fetched_assets, currency::USD);

                for(size_t i = 0; i < fetched_assets.size(); i++){
                    portfolio_value += amounts[i] * fetched_assets[i].data_points[fetched_assets[i].n_data_points - 1].adjclose;
                }
                std::vector<std::pair<std::string, double>> weights;
                for(size_t i = 0; i < fetched_assets.size(); i++){
                    weights.push_back({fetched_assets[i].symbol, (amounts[i] * fetched_assets[i].data_points[fetched_assets[i].n_data_points - 1].adjclose) / portfolio_value});
                }
                response_json["portfolio_value"] = portfolio_value;
                response_json["weights"] = weights;
                res.set_content(response_json.dump(), "application/json");

            } catch(const std::exception& e) {
                if(std::string(e.what()).find("Failed to fetch market data") != std::string::npos){
                    res.status = 4041;
                    res.set_content(nlohmann::json{{"error", e.what()}}.dump(), "application/json");
                } else {
                    res.status = 400;
                    res.set_content(nlohmann::json{{"error", e.what()}}.dump(), "application/json");
                }
                res.status = 400;
                res.set_content(nlohmann::json{{"error", e.what()}}.dump(), "application/json");
                return;
            }
        });

        std::cout << "Server is running on " << host << ":" << port << std::endl;
        svr.listen(host, port);
    }
};