#pragma once

#include "../models/currency.h"
#include <cstddef>
#include <eigen3/Eigen/Dense>
#include <random>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <thread>
#include "../models/assets.h"
#include "asset_computing.h"

#define Mat Eigen::MatrixXd
#define Vec Eigen::VectorXd

inline thread_local std::mt19937 rng = [](){
    std::seed_seq seq {
        static_cast<uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count()),
        static_cast<uint64_t>(std::hash<std::thread::id>{}(std::this_thread::get_id())),
        static_cast<uint64_t>(reinterpret_cast<uintptr_t>(&seq))//Stack adress for extra entropy
    };
    return std::mt19937(seq);
}();

namespace monte_carlo {

    enum class drift_scenario {
        ZERO,
        SHRINKAGE_25,
        RISK_FREE,
        HISTORICAL
    };

    enum class volatility_model {
        HISTORICAL,
        EWMA_100,
        EWMA_75,
        EWMA_50,
    };

    struct sim_preset {
        double portfolio_value;

        size_t n_assets;
        std::vector<double> sigma;
        std::vector<double> mu;
        std::vector<double> df;
        std::vector<double> sigma_sq_ewma;
        Vec weight;

        Mat cholesky_cov_mat;
        size_t n_sims;
        size_t horizon_days;
        drift_scenario drift_scenario;
        volatility_model vol_model;
    };

    struct sim_result {
        std::vector<double> final_portfolio_values;
        double var_95;
        double var_99;
        double cvar_95;
        double cvar_99;
        double avg;
        double median;
    };

    //Portfolio value in USD, weights should be in decimals (0.5 for 50%) 
    sim_preset generate_sim_preset(std::vector<assets::asset>& assets, const std::vector<double>& weights, size_t n_sims, size_t horizon_days, double portfolio_value = 1000, drift_scenario scenario = drift_scenario::SHRINKAGE_25){
        sim_preset preset;
        data_fetcher::unify_asset_currencies(assets, currency::USD);
        preset.n_sims = n_sims;
        preset.horizon_days = horizon_days;
        preset.portfolio_value = portfolio_value;

        preset.n_assets = assets.size();
        preset.sigma.reserve(assets.size());
        preset.mu.reserve(assets.size());
        preset.df.reserve(assets.size());
        preset.drift_scenario = scenario;
        preset.sigma_sq_ewma.reserve(assets.size());
        preset.weight = Vec::Map(weights.data(), weights.size());

        double lambda;
        double r = 0.0;
        double sigma;
        double sigma_sq;
        for (size_t i = 0; i < assets.size(); i++) {

            //EWMA to simulate dirft-phases : lambda = 0.94 (JP Morgan 1994) | 0.9 for crypto since faster market changes
            sigma = asset_compute::volatility(assets[i]);
            sigma_sq = sigma * sigma;
            for(size_t j = 1; j < assets[i].n_data_points; j++){
                double r = std::log(assets[i].data_points[j].adjclose / 
                                    assets[i].data_points[j-1].adjclose);
                sigma_sq = lambda * sigma_sq + (1 - lambda) * r * r;
            }
            
            preset.sigma.push_back(sigma);
            preset.sigma_sq_ewma.push_back(sigma_sq);
            
            preset.mu.push_back(asset_compute::avg_log_return(assets[i])); 
            preset.df.push_back(std::max(asset_compute::dof_excess_kurtosis(assets[i]), 5.0));
            //preset.df.back() = std::min(5.0, 30.0);
            //Avoid extremely low dof which can cause instability in the t-distribution. Fat Tails only importanr for low horizons
            preset.df.back() = preset.df[i] + (30.0 - preset.df[i]) * std::min(1.0, horizon_days / 63.0);
            
        }
        Mat cov_matrix = asset_compute::compute_covariance_matrix(assets);
        asset_compute::normalize_covariance_matrix(cov_matrix);
        preset.cholesky_cov_mat = asset_compute::cholesky_decomp(cov_matrix);
        return preset;
    }

    void simulation_job(const sim_preset& preset, unsigned int id, int32_t load_size, double* res_out){
        std::vector<std::student_t_distribution<double>> dists;
        dists.reserve(preset.n_assets);
        std::vector<double> drifts(preset.n_assets);
        for (size_t k = 0; k < preset.n_assets; ++k) {
            dists.emplace_back(preset.df[k]);
            switch (preset.vol_model) {
                case volatility_model::HISTORICAL:
                    drifts[k] = preset.mu[k] - 0.5 * preset.sigma[k] * preset.sigma[k];
                    break; //Use preset.sigma as is
                case volatility_model::EWMA_100:
                    drifts[k] = preset.mu[k] - 0.5 * preset.sigma_sq_ewma[k];
                    break;
                case volatility_model::EWMA_75:
                    drifts[k] = preset.mu[k] - 0.5 * (preset.sigma_sq_ewma[k] * 0.75 + (1 - 0.75) * preset.sigma[k] * preset.sigma[k]);
                    break;
                case volatility_model::EWMA_50:
                    drifts[k] = preset.mu[k] - 0.5 * (preset.sigma_sq_ewma[k] * 0.5 + (1 - 0.5) * preset.sigma[k] * preset.sigma[k]);
                    break;
            }

            switch(preset.drift_scenario){
                case drift_scenario::ZERO:
                    drifts[k] = 0.0;
                    break;
                case drift_scenario::SHRINKAGE_25:
                    drifts[k] *= 0.25;
                    break;
                case drift_scenario::RISK_FREE:
                    drifts[k] = 0.01 / 252; //Assuming 1% annual risk free rate, converted to daily
                    break;
                case drift_scenario::HISTORICAL:
                default:
                    break;
            }
        }
        Vec S(preset.n_assets);
        Vec Z(preset.n_assets);
        Vec Eps(preset.n_assets);
        for(size_t i = 0; i < load_size && id * load_size + i < preset.n_sims; ++i){
            S.setOnes();
            for(size_t d = 0; d < preset.horizon_days; ++d){
                for(size_t k = 0; k < preset.n_assets; ++k){
                    double t = dists[k](rng);
                    double scale = std::sqrt((preset.df[k] - 2) / preset.df[k]);
                    Z(k) = t * scale;
                }
                Eps.noalias() = preset.cholesky_cov_mat * Z;
                for(size_t j = 0; j < preset.n_assets; ++j){
                    switch(preset.vol_model) {
                        case volatility_model::HISTORICAL:
                            S(j) *= std::exp(drifts[j] + preset.sigma[j] * Eps(j));
                            break;
                        case volatility_model::EWMA_100:
                            S(j) *= std::exp(drifts[j] + preset.sigma_sq_ewma[j] * Eps(j));
                            break;
                        case volatility_model::EWMA_75:
                            S(j) *= std::exp(drifts[j] + (preset.sigma_sq_ewma[j] * 0.75 + (1 - 0.75) * preset.sigma[j] * preset.sigma[j]) * Eps(j));
                            break;
                        case volatility_model::EWMA_50:
                            S(j) *= std::exp(drifts[j] + (preset.sigma_sq_ewma[j] * 0.5 + (1 - 0.5) * preset.sigma[j] * preset.sigma[j]) * Eps(j));
                            break;
                    }
                    double daily_return = drifts[j] + preset.sigma[j] * Eps(j);
                    daily_return = std::clamp(daily_return, -0.20, 0.20); //max +- 20% daily return to avoid instability from extreme outliers in t-distribution
                    S[j] *= std::exp(daily_return);
                }
            }
            res_out[id*load_size + i] = (S.dot(preset.weight)) * preset.portfolio_value;
        }
    }

    sim_result run_simulation(const sim_preset& preset){
        sim_result result;
        result.final_portfolio_values.resize(preset.n_sims);
        unsigned int n_threads = std::thread::hardware_concurrency() > 0 ? std::thread::hardware_concurrency() : 4;
        std::vector<std::thread> threads;
        int32_t load_size = static_cast<int32_t>(preset.n_sims / n_threads);
        for(unsigned int i = 0; i < n_threads; ++i){
            threads.emplace_back(simulation_job, std::cref(preset), i, load_size, result.final_portfolio_values.data());
        }
        //work on remaining simulations in main thread if n_sims is not perfectly divisible by n_threads
        simulation_job(std::cref(preset), n_threads, load_size, result.final_portfolio_values.data());

        for(auto& t : threads){
            t.join();
        }

        std::sort(result.final_portfolio_values.begin(), result.final_portfolio_values.end());

        result.var_95 = preset.portfolio_value - result.final_portfolio_values[static_cast<size_t>(preset.n_sims * 0.05)];
        result.var_99 = preset.portfolio_value - result.final_portfolio_values[static_cast<size_t>(preset.n_sims * 0.01)];
        result.cvar_95 = preset.portfolio_value - std::accumulate(result.final_portfolio_values.begin(), result.final_portfolio_values.begin() + static_cast<size_t>(preset.n_sims * 0.05), 0.0) / (preset.n_sims * 0.05);
        result.cvar_99 = preset.portfolio_value - std::accumulate(result.final_portfolio_values.begin(), result.final_portfolio_values.begin() + static_cast<size_t>(preset.n_sims * 0.01), 0.0) / (preset.n_sims * 0.01);
        result.avg = std::accumulate(result.final_portfolio_values.begin(), result.final_portfolio_values.end(), 0.0) / preset.n_sims;
        result.median = result.final_portfolio_values[preset.n_sims / 2];


        return result;
    }
}