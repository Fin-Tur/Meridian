#pragma once
#include "monte_carlo_engine.h"

#define u32 uint32_t

namespace testing {

    struct job {
        std::vector<monte_carlo::sim_preset> presets;
        std::vector<double> portfolio_values; 
        monte_carlo::sim_config config;
        int test_period;
        int n_testings;
    };

    struct test_result {
        double exceedance_rate_95;
        double exceedance_rate_99;
        double christoffersen_lr;
        bool christoffersen_pass;
        double median_return_diff;
        double avg_return_diff;
    };

    struct precompute_sim_params {
        //Outer vec = testing iteration, inner vec = asset
        std::vector<std::vector<double>> mu;
        std::vector<std::vector<double>> sigma;
        std::vector<std::vector<double>> sigma_ewma;
        std::vector<std::vector<double>> df;
        std::vector<double> avg_fd;
        std::vector<std::unordered_map<monte_carlo::regimes, Mat>> cholesky_cov_mats; 
        int testing_period;
        int n_testings;
    };

    void _compute_mu(precompute_sim_params& pc, const std::vector<assets::asset>& assets);

    void _compute_sigma(precompute_sim_params& pc, const std::vector<assets::asset>& assets);

    void _compute_sigma_ewma(precompute_sim_params& pc, const std::vector<assets::asset>& assets);

    void _compute_dof_each(precompute_sim_params& pc, const std::vector<assets::asset>& assets);

    void _compute_dof_avg(precompute_sim_params& pc, const std::vector<double>& weights);

    inline void _compute_cholesky_cov_mats(precompute_sim_params& pc){}




    job prepare_backtest_job(std::vector<assets::asset>& assets, std::vector<double>& weights, size_t n_sims, int testing_period, int n_testings ,monte_carlo::sim_config config);
    test_result run_backtest(const job& j);

}