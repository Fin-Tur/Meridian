#pragma once

#include "../models/currency.h"
#include <cstddef>
#include <eigen3/Eigen/Dense>
#include <random>
#include <chrono>
#include <algorithm>
#include "../models/assets.h"
#include "asset_computing.h"

#define Mat Eigen::MatrixXd
#define Vec Eigen::VectorXd

thread_local std::mt19937 rng(std::chrono::system_clock::now().time_since_epoch().count());

namespace monte_carlo {

    struct sim_preset {
        std::vector<double> s_0;
        std::vector<double> sigma;
        std::vector<double> mu;
        std::vector<double> df;
        std::vector<double> weight;

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

        preset.s_0.reserve(assets.size());
        preset.sigma.reserve(assets.size());
        preset.mu.reserve(assets.size());
        preset.df.reserve(assets.size());

        preset.weight.reserve(assets.size());
        for (size_t i = 0; i < assets.size(); i++) {
            //preset.s_0.push_back(assets[i].data_points[assets[i].n_data_points - 1].adjclose);
            preset.s_0.push_back(1);
            preset.sigma.push_back(asset_compute::volatility(assets[i]));
            preset.mu.push_back(asset_compute::avg_log_return(assets[i])); 
            preset.df.push_back(asset_compute::dof_excess_kurtosis(assets[i]));
            preset.weight.push_back(weights[i]);
        }
        Mat cov_matrix = asset_compute::compute_covariance_matrix(assets);
        asset_compute::normalize_covariance_matrix(cov_matrix);
        preset.cholesky_cov_mat = asset_compute::cholesky_decomp(cov_matrix);
        return preset;
    }

    sim_result run_simulation(const sim_preset& preset){
        const size_t n_assets = preset.s_0.size();
        sim_result result;
        result.final_portfolio_values.reserve(preset.n_sims);

        for(size_t i = 0; i < preset.n_sims; ++i){
            Vec S = Vec::Map(preset.s_0.data(), n_assets);
            for(size_t d = 0; d < preset.horizon_days; ++d){
                Vec Z(n_assets);
                for(size_t k = 0; k < n_assets; ++k){
                    std::student_t_distribution<double> dist(preset.df[k]);
                    Z(k) = dist(rng);
                }
                auto Eps = preset.cholesky_cov_mat * Z;
                for(size_t j = 0; j < n_assets; ++j){
                    S(j) *= std::exp((preset.mu[j] - 0.5 * preset.sigma[j] * preset.sigma[j])+ preset.sigma[j] * Eps(j));
                }
            }
            result.final_portfolio_values.push_back((S.dot(Vec::Map(preset.weight.data(), preset.weight.size()))) * preset.portfolio_value);
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