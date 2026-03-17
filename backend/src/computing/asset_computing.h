#pragma once
#include "../models/assets.h"
#include <cmath>
#include <vector>
#include <eigen3/Eigen/Dense>
#include <unordered_set>

#define Mat Eigen::MatrixXd
#define Vec Eigen::VectorXd

namespace asset_compute {

    double avg_log_return(const assets::asset& asset){
        double sum_log_return = 0.0;
        for (size_t i = 1; i < asset.n_data_points; i++) {
            double log_return = std::log(asset.data_points[i].adjclose / asset.data_points[i - 1].adjclose);
            sum_log_return += log_return;
        }
        return sum_log_return / (asset.n_data_points - 1);
    }

    double med_log_return_for_timestamp(const std::vector<assets::asset>& assets, uint32_t timestamp){
        std::vector<double> cross_asset_returns;
        cross_asset_returns.reserve(assets.size());
        for (const auto& asset : assets) {
            for (size_t i = 1; i < static_cast<size_t>(asset.n_data_points); i++) {
                if (asset.data_points[i].timestamp == timestamp) {
                    double prev = asset.data_points[i - 1].adjclose;
                    double curr = asset.data_points[i].adjclose;
                    if (prev <= 0.0 || curr <= 0.0) continue;
                    double lr = std::log(curr / prev);
                    if (!std::isfinite(lr)) continue;
                    cross_asset_returns.push_back(lr);
                    break;
                }
            }
        }
        if (cross_asset_returns.empty()) return 0.0;
        size_t mid = cross_asset_returns.size() / 2;
        std::nth_element(cross_asset_returns.begin(), cross_asset_returns.begin() + mid, cross_asset_returns.end());
        return cross_asset_returns[mid];
    }

    size_t count_negative_log_returns_for_timestamp(const std::vector<assets::asset>& assets, uint32_t timestamp){
        size_t count = 0;
        for (const auto& asset : assets) {
            for (size_t i = 1; i < static_cast<size_t>(asset.n_data_points); i++) {
                if (asset.data_points[i].timestamp == timestamp) {
                    double prev = asset.data_points[i - 1].adjclose;
                    double curr = asset.data_points[i].adjclose;
                    if (prev <= 0.0 || curr <= 0.0) continue;
                    double lr = std::log(curr / prev);
                    if (!std::isfinite(lr)) continue;
                    if (lr < 0.0) count++;
                    break;
                }
            }
        }
        return count;
    }

    std::vector<double> log_returns(const assets::asset& asset){
        std::vector<double> log_returns;
        log_returns.reserve(asset.n_data_points - 1);
        for (size_t i = 1; i < asset.n_data_points; i++) {
            double log_return = std::log(asset.data_points[i].adjclose / asset.data_points[i - 1].adjclose);
            log_returns.push_back(log_return);
        }
        return log_returns;
    }

    [[deprecated]] std::vector<std::pair<double,double>> log_return_aligned_assets(const assets::asset& asset1, const assets::asset& asset2) {
        std::vector<std::pair<double,double>> aligned_returns_vec(asset1.n_data_points, {0.0, 0.0});
        size_t j = 0;
        size_t k = 0;
        for (size_t i = 1; i < asset1.n_data_points; i++) {
           while (j < asset2.n_data_points && asset2.data_points[j].timestamp < asset1.data_points[i].timestamp) {
                j++;
            }
            if (j < asset2.n_data_points && asset2.data_points[j].timestamp == asset1.data_points[i].timestamp) {
                if(j==0) continue;
                aligned_returns_vec[k].first = std::log(asset1.data_points[i].adjclose / asset1.data_points[i - 1].adjclose);
                aligned_returns_vec[k].second = std::log(asset2.data_points[j].adjclose / asset2.data_points[j - 1].adjclose);
                k++;
            }
            
        }
        aligned_returns_vec.resize(k);
        return aligned_returns_vec;
    }

    int32_t count_assets_for_timestamp(const std::vector<assets::asset>& assets, uint32_t timestamp){
        int32_t counts = 0;
        for (const auto& asset : assets) {
            for (size_t i = 1; i <asset.n_data_points; i++) {
                if (asset.data_points[i].timestamp == timestamp) {
                    counts++;
                    break;
                }
            }
        }
        return counts;
    }

    std::vector<std::vector<double>> aligned_log_returns(const std::vector<assets::asset>& assets, const std::unordered_set<uint32_t>* const timestamps = nullptr){ 
        std::vector<std::vector<double>> result;
        if (assets.empty()) return result;
        //map timestamp, logreturn
        std::vector<std::unordered_map<uint32_t, double>> asset_log_return_maps;
        asset_log_return_maps.reserve(assets.size());
        for (const auto& asset : assets) {
            std::unordered_map<uint32_t, double> lr_map;
            for (int i = 1; i < asset.n_data_points; ++i) {
                double prev = asset.data_points[i - 1].adjclose;
                double curr = asset.data_points[i].adjclose;
                if (prev <= 0.0 || curr <= 0.0) continue; // skip invalid/missing data
                double lr = std::log(curr / prev);
                if (!std::isfinite(lr)) continue; // skip NaN / inf
                uint32_t ts = asset.data_points[i].timestamp;
                lr_map[ts] = lr;
            }
            asset_log_return_maps.push_back(std::move(lr_map));
        }
        //extract timestamps in every asset
        std::unordered_set<uint32_t> common_timestamps;
        for (const auto& [ts, _] : asset_log_return_maps[0]) common_timestamps.insert(ts);
        for (size_t i = 1; i < asset_log_return_maps.size(); ++i) {
            std::unordered_set<uint32_t> next;
            for (const auto& [ts, _] : asset_log_return_maps[i]) {
                if (common_timestamps.count(ts)) next.insert(ts);
            }
            common_timestamps = std::move(next);
            if (common_timestamps.empty()) return {};
        }

        std::unordered_set<uint32_t> matching_timestamps;
        for(const auto& ts : common_timestamps){
            if(timestamps && !timestamps->contains(ts)) continue;
            matching_timestamps.insert(ts);
        }
        if(matching_timestamps.empty()) return {};

        for (uint32_t ts : matching_timestamps) {
            std::vector<double> day_returns;
            for (const auto& lr_map : asset_log_return_maps) {
                day_returns.push_back(lr_map.at(ts));
            }
            result.push_back(std::move(day_returns));
        }
        return result;
    }

    [[deprecated]] std::vector<std::pair<double,double>> log_return_aligned_assets_for_timestamps(const assets::asset& asset1, const assets::asset& asset2, const std::unordered_set<uint32_t>& timestamps) {
        std::vector<std::pair<double,double>> aligned_returns_vec(asset1.n_data_points, {0.0, 0.0});
        size_t j = 0;
        size_t k = 0;
        for (size_t i = 1; i < asset1.n_data_points; i++) {
           while (j < asset2.n_data_points && asset2.data_points[j].timestamp < asset1.data_points[i].timestamp) {
                j++;
            }
            if (j < asset2.n_data_points && asset2.data_points[j].timestamp == asset1.data_points[i].timestamp) {
                if(j==0) continue;
                if(!timestamps.contains(asset1.data_points[i].timestamp)) continue;
                aligned_returns_vec[k].first = std::log(asset1.data_points[i].adjclose / asset1.data_points[i - 1].adjclose);
                aligned_returns_vec[k].second = std::log(asset2.data_points[j].adjclose / asset2.data_points[j - 1].adjclose);
                k++;
            }
            
        }
        aligned_returns_vec.resize(k);
        return aligned_returns_vec;
    }


    std::unordered_set<uint32_t> compute_stressed_timestamps(const std::vector<assets::asset>& assets, double med_log_return_threshold = 0.01, double min_negative_log_return_pct = 0.8){
        std::unordered_set<uint32_t> stressed_timestamps;
        if (assets.empty()) return stressed_timestamps;
        uint32_t earliest = UINT32_MAX;
        uint32_t latest   = 0;
        for (const auto& asset : assets) {
            for (size_t i = 0; i < static_cast<size_t>(asset.n_data_points); i++) {
                uint32_t ts = asset.data_points[i].timestamp;
                if (ts < earliest) earliest = ts;
                if (ts > latest)   latest   = ts;
            }
        }
        if (earliest == UINT32_MAX || latest < earliest) return stressed_timestamps;

        for (uint32_t ts = earliest; ts <= latest; ts ++ ) {
            double med_lr = med_log_return_for_timestamp(assets, ts);
            if (med_lr <= -med_log_return_threshold) {
                size_t count_neg = count_negative_log_returns_for_timestamp(assets, ts);
                size_t count_total = count_assets_for_timestamp(assets, ts);
                double neg_pct = static_cast<double>(count_neg) / count_total;
                if (neg_pct >= min_negative_log_return_pct && count_total / static_cast<double>(assets.size()) > 0.6) {
                    stressed_timestamps.insert(ts);
                }
            }
        }
        return stressed_timestamps;
    }

    Mat compute_covariance_matrix(const std::vector<assets::asset>& assets, bool stressed = false){
        size_t n_assets = assets.size();
        Mat cov_matrix(n_assets, n_assets);

        std::unordered_set<uint32_t> stressed_timestamps;
        if (stressed) {
            stressed_timestamps = compute_stressed_timestamps(assets);
        }

        auto daily_returns = stressed
            ? aligned_log_returns(assets, &stressed_timestamps)
            : aligned_log_returns(assets);

        if (daily_returns.size() < 2) {
            cov_matrix.setZero();
            return cov_matrix;
        }

        std::vector<double> means(n_assets, 0.0);
        for (const auto& day : daily_returns) {
            for (size_t k = 0; k < n_assets; ++k)
                means[k] += day[k];
        }
        for (size_t k = 0; k < n_assets; ++k)
            means[k] /= static_cast<double>(daily_returns.size());

        for (size_t i = 0; i < n_assets; ++i) {
            for (size_t j = i; j < n_assets; ++j) {
                double cov = 0.0;
                for (const auto& day : daily_returns)
                    cov += (day[i] - means[i]) * (day[j] - means[j]);
                cov /= static_cast<double>(daily_returns.size() - 1);
                cov_matrix(i, j) = cov;
                cov_matrix(j, i) = cov; //Symmetric
            }
        }
        return cov_matrix;
    }

    void normalize_covariance_matrix(Mat& cov_matrix){
        auto n_rows = cov_matrix.rows();
        auto n_cols = cov_matrix.cols();
        auto diag = cov_matrix.diagonal().eval();
        for(size_t i = 0; i < n_rows; ++i){
            for(size_t j = i; j < n_cols; ++j){
                cov_matrix(i,j) /= std::sqrt(diag(i) * diag(j));
                cov_matrix(j,i) = cov_matrix(i,j); 
            }
        }
    }

    double volatility(const assets::asset& asset){
        double avg_return = avg_log_return(asset);
        double volatility_sum = 0.0;
        for(size_t i = 1; i < (size_t)asset.n_data_points; i++){
            double log_ret = std::log(asset.data_points[i].adjclose / asset.data_points[i - 1].adjclose);
            volatility_sum += (log_ret - avg_return) * (log_ret - avg_return);
        }
        return std::sqrt(volatility_sum / (asset.n_data_points - 1));
    }

    double excess_kurtosis(const assets::asset& asset) {
        if (asset.n_data_points < 2) {
            return 0.0;
        }
        double mu = avg_log_return(asset);
        double sigma = volatility(asset);

        if (sigma <= 0.0) {
            return 0.0;
        }

        double sum = 0.0;
        size_t n_returns = asset.n_data_points - 1;

        for (size_t i = 1; i < asset.n_data_points; ++i) {
            double r = std::log(
                asset.data_points[i].adjclose / asset.data_points[i - 1].adjclose
            );

            double z = (r - mu) / sigma;
            sum += std::pow(z, 4);
        }

        return sum / static_cast<double>(n_returns) - 3.0;
    }

    double dof_excess_kurtosis(const assets::asset& asset){
        double kurtosis_sum = excess_kurtosis(asset);
         return (4 + 6/kurtosis_sum) > 0 ? kurtosis_sum : std::numeric_limits<double>::infinity(); 
         //Formula to get dof for student-t distribution based on excess kurtosis, if the result is negative 
         //we return infinity which corresponds to a normal distribution
    }

    Mat cholesky_decomp(const Mat& cov_mat){
        Eigen::LLT<Mat> llt(cov_mat);
        if(llt.info() == Eigen::NumericalIssue){
            throw std::runtime_error("Covariance matrix is not positive definite");
        }
        return llt.matrixL();
    }

    double max_drawdown(const assets::asset& asset){
        double max_drawdown = 0.0;
        double peak = asset.data_points[0].adjclose;
        for(size_t i = 1; i < asset.n_data_points; i++){
            if(asset.data_points[i].adjclose > peak){
                peak = asset.data_points[i].adjclose;
            }
            double drawdown = (peak - asset.data_points[i].adjclose) / peak;
            if(drawdown > max_drawdown){
                max_drawdown = drawdown;
            }
        }
        return max_drawdown;
    }

    double sharpe_ratio(const assets::asset& asset, double risk_free_rate = 0.04){
        double avg_return = avg_log_return(asset)*252; //Annualize
        double vol = volatility(asset)*std::sqrt(252); //Annualize
        return (avg_return - risk_free_rate) / vol;
    }

    double ytd_return(const assets::asset& asset){
        std::time_t now = std::time(nullptr);
        std::tm* now_tm = std::localtime(&now);
        int current_year = now_tm->tm_year + 1900;

        size_t start_idx = 0;
        for(size_t i = 0; i < asset.n_data_points; i++){
            std::time_t t = asset.data_points[i].timestamp;
            std::tm* tm = std::localtime(&t);
            if(tm->tm_year + 1900 == current_year){
                start_idx = i;
                break;
            }
        }

        double start_price = asset.data_points[start_idx].adjclose;
        double end_price   = asset.data_points[asset.n_data_points - 1].adjclose;
        return (end_price - start_price) / start_price;
    }

    double skewness(const assets::asset& asset){
        double avg = avg_log_return(asset);
        double vol = volatility(asset);
        double sum = 0.0;
        for(size_t i = 1; i < asset.n_data_points; i++){
            double log_ret = std::log(asset.data_points[i].adjclose / asset.data_points[i-1].adjclose);
            sum += std::pow((log_ret - avg) / vol, 3.0);
        }
        return sum / (asset.n_data_points - 1);
    }

}