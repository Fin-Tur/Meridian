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

thread_local std::mt19937 rng = [](){
    std::seed_seq seq {
        static_cast<uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count()),
        static_cast<uint64_t>(std::hash<std::thread::id>{}(std::this_thread::get_id())),
        static_cast<uint64_t>(reinterpret_cast<uintptr_t>(&seq))//Stack adress for extra entropy
    };
    return std::mt19937(seq);
}();

namespace monte_carlo {

    struct sim_preset {
        size_t n_assets;
        std::vector<double> sigma;
        std::vector<double> mu;
        std::vector<double> df;
        Vec weight;

        size_t n_sims;
        size_t horizon_days;
        Mat cholesky_cov_mat;

        double portfolio_value;
    };

    struct sim_result {
        std::vector<double> final_portfolio_values;
        double var_95;
        double var_99;
        double cvar_95;
        double cvar_99;
        double avg;
    };

    //Portfolio value in USD, weights should be in decimals (0.5 for 50%)
    sim_preset generate_sim_preset(std::vector<assets::asset>& assets, const std::vector<double>& weights, size_t n_sims, size_t horizon_days, double portfolio_value = 1000){
        sim_preset preset;
        data_fetcher::unify_asset_currencies(assets, currency::USD);
        preset.n_sims = n_sims;
        preset.horizon_days = horizon_days;
        preset.portfolio_value = portfolio_value;

        preset.n_assets = assets.size();
        preset.sigma.reserve(assets.size());
        preset.mu.reserve(assets.size());
        preset.df.reserve(assets.size());

        preset.weight = Vec::Map(weights.data(), weights.size());

        for (size_t i = 0; i < assets.size(); i++) {
            preset.sigma.push_back(asset_compute::volatility(assets[i]));
            preset.mu.push_back(asset_compute::avg_log_return(assets[i])); 
            preset.df.push_back(asset_compute::dof_excess_kurtosis(assets[i]));
        }
        Mat cov_matrix = asset_compute::compute_covariance_matrix(assets);
        asset_compute::normalize_covariance_matrix(cov_matrix);
        preset.cholesky_cov_mat = asset_compute::cholesky_decomp(cov_matrix);
        return preset;
    }

    void simulation_job(const sim_preset& preset, int8_t id, int32_t load_size, double* res_out){
        std::vector<std::student_t_distribution<double>> dists;
        dists.reserve(preset.n_assets);
        std::vector<double> drifts(preset.n_assets);
        for (size_t k = 0; k < preset.n_assets; ++k) {
            dists.emplace_back(preset.df[k]);
            drifts[k] = preset.mu[k] - 0.5 * preset.sigma[k] * preset.sigma[k];
        }
        Vec S(preset.n_assets);
        Vec Z(preset.n_assets);
        Vec Eps(preset.n_assets);
        for(size_t i = 0; i < load_size && id * load_size + i < preset.n_sims; ++i){
            S.setOnes();
            for(size_t d = 0; d < preset.horizon_days; ++d){
                for(size_t k = 0; k < preset.n_assets; ++k){
                    Z[k] = dists[k](rng);
                }
                Eps.noalias() = preset.cholesky_cov_mat * Z;
                for(size_t j = 0; j < preset.n_assets; ++j){
                    S(j) *= std::exp(drifts[j] + preset.sigma[j] * Eps(j));
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


        return result;
    }
}