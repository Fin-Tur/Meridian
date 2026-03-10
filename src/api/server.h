#pragma once

#include <vector>
#include <string>
#include <iostream>
#include "../thirdparty/httplib.h"
#include "../thirdparty/json.hpp"
#include "../models/assets.h"
#include "../data/MarketDataFetcher.h"
#include "../computing/monte_carlo_engine.h"

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
            try{
                //Fetch body
                auto body = nlohmann::json::parse(req.body);
                double portfolio_value = body.at("portfolio_value").get<double>();
                size_t horizon_days = body.value("horizon_days", 252);
                size_t n_sims = body.value("n_simulations", 10000);

            std::vector<assets::asset> fetched_assets;
            std::vector<double> weights;

            for (const auto& a : body.at("assets")) {
                std::string ticker = a.at("ticker").get<std::string>();
                std::string type   = a.at("type").get<std::string>();
                double weight      = a.at("weight").get<double>();

                assets::asset fetched;
                if (type == "crypto") {
                    std::string coinId = ticker;
                    fetched = data_fetcher::fetch_crypto(coinId, ticker);
                } else {
                    fetched = data_fetcher::fetch_stock(ticker);
                }

                fetched_assets.push_back(fetched);
                weights.push_back(weight);
            }

            // Run Monte Carlo simulation
            auto preset = monte_carlo::generate_sim_preset(fetched_assets, weights, n_sims, horizon_days, portfolio_value);
            auto result = monte_carlo::run_simulation(preset);

            nlohmann::json response_json;   
            response_json["var_95"] = result.var_95;
            response_json["var_99"] = result.var_99;
            response_json["cvar_95"] = result.cvar_95;
            response_json["cvar_99"] = result.cvar_99;
            response_json["avg"] = result.avg;
            response_json["min"] = result.final_portfolio_values.front();
            response_json["max"] = result.final_portfolio_values.back();
            double bin_width = (result.final_portfolio_values.back() - result.final_portfolio_values.front()) / 50;
            response_json["bin_width"] = bin_width;    
            std::vector<int> histogramm_bins(50);
            for(auto& a : result.final_portfolio_values){
                int bin_index = std::min(static_cast<int>((a - result.final_portfolio_values.front()) / bin_width), 49);
                bins[bin_index]++;
            }
            res.set_content(response_json.dump(), "application/json");

            } catch(const std::exception& e) {
                res.status = 400;
                res.set_content(nlohmann::json{{"error", e.what()}}.dump(), "application/json");
                return;
            }
        });

        svr.Post("/correlation", [](const httplib::Request& req, httplib::Response& res){
            try{
                auto body = nlohmann::json::parse(req.body);
                std::vector<assets::asset> fetched_assets;

                for (const auto& a : body.at("assets")) {
                    std::string ticker = a.at("ticker").get<std::string>();
                    std::string type   = a.at("type").get<std::string>();

                    assets::asset fetched;
                    if (type == "crypto") {
                        std::string coinId = ticker;
                        fetched = data_fetcher::fetch_crypto(coinId, ticker);
                    } else {
                        fetched = data_fetcher::fetch_stock(ticker);
                    }

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

        svr.Post("/asset" [](const httplib::Request& req, httplib::Response& res){
            try{
                auto body = nlohmann::json::parse(req.body);
                std::string ticker = body.at("ticker").get<std::string>();
                std::string type   = body.at("type").get<std::string>();

                assets::asset fetched;
                if (type == "crypto") {
                    std::string coinId = ticker;
                    fetched = data_fetcher::fetch_crypto(coinId, ticker);
                } else {
                    fetched = data_fetcher::fetch_stock(ticker);
                }

                std::vector<double> adj_closes(fetched.data_points, fetched.data_points + fetched.n_data_points);

                nlohmann::json response_json;
                response_json["symbol"] = fetched.symbol;
                response_json["currency"] = fetched.currency;
                response_json["adj_closes"] = adj_closes;
                response_json["volatility"] = asset_compute::volatility(fetched) * sqrt(252); //Annualized volatility
                response_json["avg_return"] = asset_compute::average_log_return(fetched) * 252; //Annualized average log return
                response_json["sharpe_ratio"] = asset_compute::sharpe_ratio(fetched);
                response_json["max_drawdown"] = asset_compute::max_drawdown(fetched);
                response_json["ytd_return"] = asset_compute::ytd_return(fetched);
                response_json["skewness"] = asset_compute::skewness(fetched);
                response_json["kurtosis"] = asset_compute::excess_kurtosis(fetched) + 3; //Add 3 to get regular kurtosis

                res.set_content(response_json.dump(), "application/json");

            } catch(const std::exception& e) {
                res.status = 400;
                res.set_content(nlohmann::json{{"error", e.what()}}.dump(), "application/json");
                return;
            }
        });

        std::cout << "Server is running on " << host << ":" << port << std::endl;
        svr.listen(host, port);
    }
};