#include "backtesting.h"
#include "asset_computing.h"
#include "../models/coins.h"
#include <cmath>

#define u32 uint32_t

namespace testing {

    void _compute_mu(precompute_sim_params& pc, const std::vector<assets::asset>& assets){
        pc.mu.assign(pc.n_testings, std::vector<double>(assets.size()));
        for(size_t a = 0; a < assets.size(); a++){
            const auto& asset = assets[a];
            int32_t included_dps = asset.n_data_points - pc.testing_period;

            double sum = 0.0;
            for(int32_t i = 1; i < included_dps; i++)
                sum += asset.data_points[i].log_return;

            pc.mu[0][a] = sum / (included_dps - 1);

            for(int i = 1; i < pc.n_testings; i++){
                int32_t new_inc_dp = included_dps - pc.testing_period;
                for(int32_t j = new_inc_dp; j < included_dps; j++)
                    sum -= asset.data_points[j].log_return;
                included_dps = new_inc_dp;
                pc.mu[i][a] = sum / (new_inc_dp - 1);
            }
        }
    }

    void _compute_sigma(precompute_sim_params& pc, const std::vector<assets::asset>& assets){
        pc.sigma.assign(pc.n_testings, std::vector<double>(assets.size()));

        for(size_t a = 0; a < assets.size(); a++){
            const auto& asset = assets[a];
            int32_t included_dps = asset.n_data_points - pc.testing_period;

            std::vector<double> lr_sum(included_dps, 0.0);
            std::vector<double> lr_sum_sq(included_dps, 0.0);
            for(int32_t i = 1; i < included_dps; i++){
                double lr = asset.data_points[i].log_return;
                lr_sum[i]    = lr_sum[i-1]    + lr;
                lr_sum_sq[i] = lr_sum_sq[i-1] + lr * lr;
            }

            for(int t = 0; t < pc.n_testings; t++){
                int32_t inc_dps_new = included_dps - t * pc.testing_period; 
                int32_t n_lr = inc_dps_new - 1;                                
                double mean     = lr_sum[inc_dps_new - 1] / n_lr;
                double variance = lr_sum_sq[inc_dps_new - 1] / n_lr - mean * mean;
                pc.sigma[t][a]  = std::sqrt(std::max(variance, 0.0));
            }
        }
    }

    void _compute_sigma_ewma(precompute_sim_params& pc, const std::vector<assets::asset>& assets){
        pc.sigma_ewma.assign(pc.n_testings, std::vector<double>(assets.size()));

        for(size_t a = 0; a < assets.size(); a++){
            const auto& asset = assets[a];
            int32_t included_dps = asset.n_data_points - pc.testing_period;
            double lambda = coin::is_crypto(asset.symbol) ? 0.90 : 0.94;
            double sigma_sq = pc.sigma[0][a] * pc.sigma[0][a];

            std::vector<double> ewma_cache(included_dps);
            ewma_cache[0] = sigma_sq;
            for(int32_t i = 1; i < included_dps; i++){
                double lr = asset.data_points[i].log_return;
                sigma_sq = lambda * sigma_sq + (1.0 - lambda) * lr * lr;
                ewma_cache[i] = sigma_sq;
            }

            for(int t = 0; t < pc.n_testings; t++){
                int32_t new_incl_dp = included_dps - t * pc.testing_period;
                pc.sigma_ewma[t][a] = std::sqrt(ewma_cache[new_incl_dp - 1]);
            }
        }
    }

    void _compute_dof_each(precompute_sim_params& pc, const std::vector<assets::asset>& assets){
        pc.df.assign(pc.n_testings, std::vector<double>(assets.size()));
        double correction = std::min(1.0, static_cast<double>(pc.testing_period) / 63.0);

        for(size_t a = 0; a < assets.size(); a++){
            const auto& asset = assets[a];
            int32_t included_dps = asset.n_data_points - pc.testing_period;

            std::vector<double> s1(included_dps, 0.0);
            std::vector<double> s2(included_dps, 0.0);
            std::vector<double> s3(included_dps, 0.0);
            std::vector<double> s4(included_dps, 0.0);
            for(int32_t i = 1; i < included_dps; i++){
                double r = asset.data_points[i].log_return;
                double r2 = r * r;
                s1[i] = s1[i-1] + r;
                s2[i] = s2[i-1] + r2;
                s3[i] = s3[i-1] + r2 * r;
                s4[i] = s4[i-1] + r2 * r2;
            }

            for(int t = 0; t < pc.n_testings; t++){
                int32_t new_incl_dps  = included_dps - t * pc.testing_period;
                double   n_lr  = static_cast<double>(new_incl_dps - 1);
                double   mu = pc.mu[t][a];
                double   si = pc.sigma[t][a];

                double raw_df;
                if(n_lr < 4 || si <= 0.0) {
                    raw_df = std::numeric_limits<double>::infinity();
                } else {
                    double mu2 = mu * mu;
                    double sum4th = s4[new_incl_dps-1] - 4.0*mu*s3[new_incl_dps-1] + 6.0*mu2*s2[new_incl_dps-1] - 4.0*mu2*mu*s1[new_incl_dps-1] + n_lr*mu2*mu2;
                    double kurtosis = (sum4th / n_lr) / (si*si*si*si) - 3.0;
                    raw_df = (4.0 + 6.0 / kurtosis) > 0.0 ? kurtosis : std::numeric_limits<double>::infinity();
                }

                double df = std::max(raw_df, 5.0);
                df = df + (30.0 - df) * correction;
                pc.df[t][a] = df;
            }
        }
    }

    // avg_fd[testing_idx]: gewichteter Durchschnitt der df-Werte aller Assets.
    // Identisch zu monte_carlo_engine.cpp: gewichtete Summe / n_assets.
    void _compute_dof_avg(precompute_sim_params& pc, const std::vector<double>& weights){
        const size_t n_assets = pc.df.empty() ? 0 : pc.df[0].size();
        pc.avg_fd.assign(pc.n_testings, 0.0);
        for(int t = 0; t < pc.n_testings; t++){
            double avg = 0.0;
            for(size_t a = 0; a < n_assets; a++)
                avg += pc.df[t][a] * weights[a];
            pc.avg_fd[t] = avg / n_assets;
        }
    }

    void _compute_cholesky_cov_mats(precompute_sim_params& pc, std::vector<assets::asset>& assets){
        pc.cholesky_cov_mats.assign(pc.n_testings, std::unordered_map<monte_carlo::regimes, Mat>());

        std::vector<int32_t> orig_dps(assets.size());
        for(size_t a = 0; a < assets.size(); a++)
            orig_dps[a] = assets[a].n_data_points;

        int32_t included_dps = orig_dps[0] - pc.testing_period;
        for(size_t a = 0; a < assets.size(); a++)
            assets[a].n_data_points = included_dps;

        auto [cov_matrix, cov_matrix_stressed] = asset_compute::compute_cov_matricies(assets);
        asset_compute::normalize_covariance_matrix(cov_matrix);
        asset_compute::normalize_covariance_matrix(cov_matrix_stressed);

        uint32_t counter_normal  = 0;
        uint32_t counter_stressed = 0;

        for(int t = 0; t < pc.n_testings; t++){
            if(t > 0){
                included_dps -= pc.testing_period;
                for(size_t a = 0; a < assets.size(); a++)
                    assets[a].n_data_points = included_dps;

                counter_normal  += pc.testing_period;
                counter_stressed += pc.testing_period;

                if(counter_normal >= 10){
                    counter_normal = 0;
                    cov_matrix = asset_compute::compute_covariance_matrix(assets, false);
                    asset_compute::normalize_covariance_matrix(cov_matrix);
                }
                if(counter_stressed >= 30){
                    counter_stressed = 0;
                    cov_matrix_stressed = asset_compute::compute_covariance_matrix(assets, true);
                    asset_compute::normalize_covariance_matrix(cov_matrix_stressed);
                }
            }

            pc.cholesky_cov_mats[t][monte_carlo::regimes::CALM]    = asset_compute::cholesky_decomp(cov_matrix);
            pc.cholesky_cov_mats[t][monte_carlo::regimes::ENERGIC] = asset_compute::cholesky_decomp(0.6 * cov_matrix + 0.4 * cov_matrix_stressed);
            pc.cholesky_cov_mats[t][monte_carlo::regimes::CRISIS]  = asset_compute::cholesky_decomp(cov_matrix_stressed);
        }

        for(size_t a = 0; a < assets.size(); a++)
            assets[a].n_data_points = orig_dps[a];
    }


    job prepare_backtest_job(std::vector<assets::asset>& assets, std::vector<double>& weights, size_t n_sims, int testing_period, int n_testings ,monte_carlo::sim_config config){
        job j;
        j.config = config;
        j.n_testings = n_testings;
        j.test_period = testing_period;
        j.portfolio_values.resize(n_testings, 0.0);
        j.presets.reserve(n_testings);

        //First preset
        for(auto& asset : assets){
            j.portfolio_values[0] += asset.data_points[asset.n_data_points].adjclose * weights[&asset - &assets[0]];
            asset.n_data_points-=testing_period; 
        }
        j.presets[0] = monte_carlo::generate_sim_preset(assets, weights, n_sims, testing_period, config, j.portfolio_values[0]);
        
        //Following
        for(int i = 1; i < n_testings; i++){
            for(auto& asset : assets){
                j.portfolio_values[i] += asset.data_points[asset.n_data_points].adjclose * weights[&asset - &assets[0]];
                asset.n_data_points-=testing_period; 
            }
            monte_carlo::sim_preset preset;
        }

        
        return j;
    }


    /*job prepare_backtest_job(std::vector<assets::asset>& assets, std::vector<double>& weights, size_t n_sims, int testing_period, int n_testings ,monte_carlo::sim_config config){
        job j;
        j.config = config;
        j.n_testings = n_testings;
        j.test_period = testing_period;
        j.portfolio_values.resize(n_testings, 0.0);

        for(int i = 0; i < n_testings; i++){
            for(auto& asset : assets){
                asset.n_data_points-=testing_period; 
                j.portfolio_values[i] += asset.data_points[asset.n_data_points].adjclose * weights[&asset - &assets[0]];
            }
            j.presets.push_back(monte_carlo::generate_sim_preset(assets, weights, n_sims, testing_period, config, j.portfolio_values[i]));
            std::cout << "Prepared preset " << i+1 << "/" << n_testings << "\r" << std::flush;
        }

        return j;
    }*/

    test_result run_backtest(const job& j) {
        test_result result;

        uint32_t exceed_95 = 0;
        uint32_t exceed_99 = 0;

        uint32_t n_00 = 0, n_01 = 0, n_10 = 0, n_11 = 0;
        bool exceeded_before = false;

        // portfolio_values ist in umgekehrter Zeitreihenfolge: [0] = neuester, [n-1] = ältester
        // preset[i] simuliert ab portfolio_values[i] vorwärts -> tatsächliches Ergebnis ist portfolio_values[i-1]
        for(int i = 1; i < j.n_testings; i++) {
            auto sim_res = monte_carlo::run_simulation(j.presets[i], j.config);

            double actual_return = (j.portfolio_values[i-1] - j.portfolio_values[i])
                                / j.portfolio_values[i];
            double actual_loss = -actual_return;

            double var_95_pct = sim_res.var_95 / j.portfolio_values[i];
            double var_99_pct = sim_res.var_99 / j.portfolio_values[i];

            bool exceeded = actual_loss > var_95_pct;

            // Christoffersen Übergangsmatrix befüllen
            if(exceeded_before && exceeded)  n_11++;
            if(exceeded_before && !exceeded) n_10++;
            if(!exceeded_before && exceeded) n_01++;
            if(!exceeded_before && !exceeded) n_00++;

            if(exceeded) {
                exceed_95++;
                if(actual_loss > var_99_pct) exceed_99++;
            }

            exceeded_before = exceeded;
        }

        // ── Exceedance Rates ──────────────────────────────────────────────────
        result.exceedance_rate_95 = static_cast<double>(exceed_95) / j.n_testings;
        result.exceedance_rate_99 = static_cast<double>(exceed_99) / j.n_testings;

        // ── Kupiec Test ──────────────────────────────────────────
        auto kupiec = [&](double p, uint32_t x, uint32_t T) -> double {
            double p_hat = static_cast<double>(x) / T;
            if(p_hat <= 0.0 || p_hat >= 1.0) return 0.0;
            return -2.0 * (
                (T - x) * std::log(1.0 - p)     + x * std::log(p) -
                (T - x) * std::log(1.0 - p_hat) - x * std::log(p_hat)
            );
        };

        result.kupiec_lr_95   = kupiec(0.05, exceed_95, j.n_testings);
        result.kupiec_lr_99   = kupiec(0.01, exceed_99, j.n_testings);
        result.kupiec_95_pass = result.kupiec_lr_95 < 3.841;
        result.kupiec_99_pass = result.kupiec_lr_99 < 6.635;

        // ── Christoffersen Test ───────────────────────────────────────────────
        double p_01 = (n_00 + n_01) > 0 ?
                    static_cast<double>(n_01) / (n_00 + n_01) : 0.0;
        double p_11 = (n_10 + n_11) > 0 ?
                    static_cast<double>(n_11) / (n_10 + n_11) : 0.0;
        double p    = static_cast<double>(n_01 + n_11) /
                    (n_00 + n_01 + n_10 + n_11);

        if(p > 0.0 && p < 1.0 && p_01 > 0.0 && p_11 > 0.0 &&
        p_01 < 1.0 && p_11 < 1.0) {
            result.christoffersen_lr = -2.0 * (
                (n_00 + n_10) * std::log(1.0 - p) +
                (n_01 + n_11) * std::log(p) -
                n_00 * std::log(1.0 - p_01) - n_01 * std::log(p_01) -
                n_10 * std::log(1.0 - p_11) - n_11 * std::log(p_11)
            );
        } else {
            result.christoffersen_lr = 0.0;
        }

        result.christoffersen_pass = result.christoffersen_lr < 3.841;

        return result;
    }
}