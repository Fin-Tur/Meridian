#pragma once

#include <cstddef>
#include <vector>
#include <unordered_map>
#include <eigen3/Eigen/Dense>
#include "../models/assets.h"

#define Mat Eigen::MatrixXd
#define Vec Eigen::VectorXd

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
        CALM,    //Normal Correlation, Low Volatility
        ENERGIC, //High Correlation, Medium Volatility
        CRISIS,  //Extreme Correlation, High Volatility
    };

    extern std::unordered_map<regimes, double> regime_vol_multiplier;
    extern std::unordered_map<regimes, double> regimes_df_multiplier;

    regimes change_regime(regimes regime, double x);

    struct sim_config {
        volatility_model vol_model;
        drift_scenario drift_scenario;
        bool multivariate_t;
        bool regimes;
    };

    struct sim_preset {
        double portfolio_value;

        size_t n_assets;
        std::vector<double> sigma;      //volatility for each asset
        std::vector<double> mu;         //average log return for each asset
        std::vector<double> df;         //portfolio tail -> higher dof = less tails
        double avg_df;
        std::vector<double> sigma_ewma; //volatility depending on previous log returns
        Vec weight;

        std::unordered_map<monte_carlo::regimes, Mat> cholesky_cov_mats;

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
    sim_preset generate_sim_preset(std::vector<assets::asset>& assets, const std::vector<double>& weights, size_t n_sims, size_t horizon_days, const sim_config& config, double portfolio_value = 1000);

    void simulation_job(const sim_preset& preset, const sim_config& config, unsigned int id, int32_t load_size, double* res_out);

    sim_result run_simulation(const sim_preset& preset, const sim_config& config);

}