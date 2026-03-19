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
#include "../models/coins.h"

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

    enum class regimes {
        CALM, //Normal Correlation, Low Volatility
        ENERGIC, //High Correlation, Medium Volatility
        CRISIS, //Extreme Correlation, High Volatility

    };

    inline regimes change_regime(regimes regime, double x){
        switch(regime){
            case regimes::CALM:
                if(x < 0.002) return regimes::CRISIS;
                if(x < 0.03) return regimes::ENERGIC;
                return regimes::CALM;
                break;
            case regimes::ENERGIC:
                if(x < 0.05) return regimes::CRISIS;
                if(x < 0.2) return regimes::CALM;
                return regimes::ENERGIC;
                break;
            case regimes::CRISIS:
                if(x < 0.25) return regimes::CALM;
                if(x < 0.55) return regimes::CRISIS;
                return regimes::ENERGIC;
                break;
        }
        return regimes::CALM;
    }

    static std::unordered_map<regimes, double> regime_vol_multiplier = {
        {regimes::CALM, 0.8},
        {regimes::ENERGIC, 1.4},
        {regimes::CRISIS, 2.5}
    };

    static std::unordered_map<regimes, double> regimes_df_multiplier = {
        {regimes::CALM, 1.5},
        {regimes::ENERGIC, 1.0},
        {regimes::CRISIS, 0.5}
    };

    struct sim_config {
        volatility_model vol_model;
        drift_scenario drift_scenario;
        bool multivariate_t;
        bool regimes; 
    };

    struct sim_preset {
        double portfolio_value;

        size_t n_assets;
        std::vector<double> sigma; //volatility for each asset
        std::vector<double> mu; //average log return for each asset
        std::vector<double> df; //portfolio tail -> higher dof = less tails
        double avg_df;
        std::vector<double> sigma_ewma; //volatility depending on previos log returns
        Vec weight; 

        std::unordered_map<regimes, Mat> cholesky_cov_mats;

        size_t n_sims;
        size_t horizon_days;
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
    sim_preset generate_sim_preset(std::vector<assets::asset>& assets, const std::vector<double>& weights, size_t n_sims, size_t horizon_days,  const sim_config& config, double portfolio_value = 1000){
        sim_preset preset;
        data_fetcher::unify_asset_currencies(assets, currency::USD);
        preset.n_sims = n_sims;
        preset.horizon_days = horizon_days;
        preset.portfolio_value = portfolio_value;

        preset.n_assets = assets.size();
        preset.sigma.reserve(assets.size());
        preset.mu.reserve(assets.size());
        preset.df.reserve(assets.size());
        preset.sigma_ewma.reserve(assets.size());
        preset.weight = Vec::Map(weights.data(), weights.size());

        double lambda;
        double r = 0.0;
        double sigma;
        double sigma_sq;
        for (size_t i = 0; i < assets.size(); i++) {

            //EWMA to simulate dirft-phases : lambda = 0.94 (JP Morgan 1994) | 0.9 for crypto since faster market changes
            lambda = coin::is_crypto(assets[i].symbol) ? 0.90 : 0.94;
            sigma = asset_compute::volatility(assets[i]);
            sigma_sq = sigma * sigma;
            for(size_t j = 1; j < assets[i].n_data_points; j++){
                double r = std::log(assets[i].data_points[j].adjclose / 
                                    assets[i].data_points[j-1].adjclose);
                sigma_sq = lambda * sigma_sq + (1 - lambda) * r * r;
            }
            
            preset.sigma.push_back(sigma);
            preset.sigma_ewma.push_back(std::sqrt(sigma_sq));
            
            preset.mu.push_back(asset_compute::avg_log_return(assets[i])); 
            preset.df.push_back(std::max(asset_compute::dof_excess_kurtosis(assets[i]), 5.0));
            preset.df.back() = preset.df[i] + (30.0 - preset.df[i]) * std::min(1.0, horizon_days / 63.0);
            
        }
        double avg_df = 0.0;
        for(const auto& df : preset.df){
            avg_df += df*weights[&df - &preset.df[0]];
        }

        preset.avg_df = avg_df/assets.size();
        auto [cov_matrix, cov_matrix_stressed] = asset_compute::compute_cov_matricies(assets);
        asset_compute::normalize_covariance_matrix(cov_matrix);
        asset_compute::normalize_covariance_matrix(cov_matrix_stressed);
        //For energetic regime we can use a blend of the two covariance matrices, for example 60% calm + 40% stressed
        Mat blended_cov = 0.6 * cov_matrix + 0.4 * cov_matrix_stressed;
        preset.cholesky_cov_mats[regimes::ENERGIC] = asset_compute::cholesky_decomp(blended_cov);
        preset.cholesky_cov_mats[regimes::CALM] = asset_compute::cholesky_decomp(cov_matrix);
        preset.cholesky_cov_mats[regimes::CRISIS] = asset_compute::cholesky_decomp(cov_matrix_stressed);

        return preset;
    }

    void simulation_job(const sim_preset& preset, const sim_config& config, unsigned int id, int32_t load_size, double* res_out){
        std::vector<std::student_t_distribution<double>> student_t_dists;
        std::chi_squared_distribution<double> chi2_calm(preset.avg_df * 1.5);
        std::chi_squared_distribution<double> chi2_energic(preset.avg_df * 1.0);
        std::chi_squared_distribution<double> chi2_crisis(preset.avg_df * 0.5);
        std::normal_distribution<double> norm(0.0, 1.0);
        std::uniform_real_distribution<double> uniform_dist(0.0, 1.0);
        student_t_dists.reserve(preset.n_assets);
        std::vector<double> drifts(preset.n_assets);
        for (size_t k = 0; k < preset.n_assets; ++k) {
            if(!config.multivariate_t)student_t_dists.emplace_back(preset.df[k]);
            switch (config.vol_model) {
                case volatility_model::HISTORICAL:
                    drifts[k] = preset.mu[k] - 0.5 * preset.sigma[k] * preset.sigma[k];
                    break; //Use preset.sigma as is
                case volatility_model::EWMA_100:
                    drifts[k] = preset.mu[k] - 0.5 * preset.sigma_ewma[k]*preset.sigma_ewma[k];
                    break;
                case volatility_model::EWMA_75:
                    drifts[k] = preset.mu[k] - 0.5 * (preset.sigma_ewma[k]*preset.sigma_ewma[k] * 0.75 + (1 - 0.75) * preset.sigma[k] * preset.sigma[k]);
                    break;
                case volatility_model::EWMA_50:
                    drifts[k] = preset.mu[k] - 0.5 * (preset.sigma_ewma[k]*preset.sigma_ewma[k] * 0.5 + (1 - 0.5) * preset.sigma[k] * preset.sigma[k]);
                    break;
            }

            switch(config.drift_scenario){
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
        Vec Shocks(preset.n_assets);
        Vec Eps(preset.n_assets);
        // == Declarations to avoid overhead in the loop ==
            double chi2_sample;
            double scale_multivariate_t;
            double student_t_sample;
            double scale;
        // ===
        for(size_t i = 0; i < load_size && id * load_size + i < preset.n_sims; ++i){
            regimes current_regime = regimes::CALM;
            S.setOnes();
            for(size_t d = 0; d < preset.horizon_days; ++d){
                if(config.regimes) current_regime = change_regime(current_regime, uniform_dist(rng));
                if(config.multivariate_t){
                    double eff_df = preset.avg_df * regimes_df_multiplier.at(current_regime);
                    switch(current_regime){
                        case regimes::CALM:    chi2_sample = chi2_calm(rng);    break;
                        case regimes::ENERGIC: chi2_sample = chi2_energic(rng); break;
                        case regimes::CRISIS:  chi2_sample = chi2_crisis(rng);  break;
                    }
                    scale_multivariate_t = std::sqrt(eff_df / chi2_sample);
                }
                for(size_t k = 0; k < preset.n_assets; ++k){
                    if(config.multivariate_t){
                        Shocks(k) = norm(rng) / scale_multivariate_t;
                    } else {
                        student_t_sample = student_t_dists[k](rng);
                        scale = std::sqrt((preset.df[k]*regimes_df_multiplier.at(current_regime) - 2) / (preset.df[k]*regime_vol_multiplier.at(current_regime)));
                        Shocks(k) = student_t_sample * scale;
                    }
                }

                switch(current_regime){
                    case regimes::CALM:
                        Eps.noalias() = (preset.cholesky_cov_mats.at(regimes::CALM)) * Shocks;
                        break;
                    case regimes::ENERGIC:
                        Eps.noalias() = (preset.cholesky_cov_mats.at(regimes::ENERGIC)) * Shocks;
                        break;
                    case regimes::CRISIS:
                        Eps.noalias() = (preset.cholesky_cov_mats.at(regimes::CRISIS)) * Shocks;
                        break;
                }

                for(size_t j = 0; j < preset.n_assets; ++j){
                    switch(config.vol_model) {
                        case volatility_model::HISTORICAL:
                            S(j) *= std::exp(drifts[j] + preset.sigma[j] * regime_vol_multiplier.at(current_regime) * Eps(j));
                            break;
                        case volatility_model::EWMA_100:
                            S(j) *= std::exp(drifts[j] + preset.sigma_ewma[j] * regime_vol_multiplier.at(current_regime) * Eps(j));
                            break;
                        case volatility_model::EWMA_75:
                            S(j) *= std::exp(drifts[j] + (preset.sigma_ewma[j] * 0.75 + (1 - 0.75) * preset.sigma[j]) * regime_vol_multiplier.at(current_regime) * Eps(j));
                            break;
                        case volatility_model::EWMA_50:
                            S(j) *= std::exp(drifts[j] + (preset.sigma_ewma[j] * 0.5 + (1 - 0.5) * preset.sigma[j]) * regime_vol_multiplier.at(current_regime) * Eps(j));
                            break;
                    }
                }
            }
            res_out[id*load_size + i] = (S.dot(preset.weight)) * preset.portfolio_value;
        }
    }

    sim_result run_simulation(const sim_preset& preset, const sim_config& config){
        sim_result result;
        result.final_portfolio_values.resize(preset.n_sims);
        unsigned int n_threads = std::thread::hardware_concurrency() > 0 ? std::thread::hardware_concurrency() : 4;
        std::vector<std::thread> threads;
        int32_t load_size = static_cast<int32_t>(preset.n_sims / n_threads);
        for(unsigned int i = 0; i < n_threads; ++i){
            threads.emplace_back(simulation_job, std::cref(preset), std::cref(config), i, load_size, result.final_portfolio_values.data());
        }
        //work on remaining simulations in main thread if n_sims is not perfectly divisible by n_threads
        simulation_job(std::cref(preset), std::cref(config), n_threads, load_size, result.final_portfolio_values.data());

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