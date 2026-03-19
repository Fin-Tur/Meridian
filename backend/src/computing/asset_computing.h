#pragma once
#include "../models/assets.h"
#include <cmath>
#include <vector>
#include <eigen3/Eigen/Dense>
#include <unordered_set>
#include <unordered_map>

#define Mat Eigen::MatrixXd
#define Vec Eigen::VectorXd

namespace asset_compute {

    struct precomputed_voc_mat_data {
        std::unordered_map<uint32_t, size_t> neg_count_map;
        std::unordered_map<uint32_t, size_t> total_count_map;
        std::vector<std::unordered_map<uint32_t, double>> log_return_maps;
        std::unordered_set<uint32_t> common_timestamps;
        size_t n_assets;
    };

    void compute_log_return_for_asset(assets::asset& asset){
        asset.data_points[0].log_return = 1.0; //First log return is set to 0 by convention
        for(size_t i = 1; i < (size_t)asset.n_data_points; i++){
           asset.data_points[i].log_return = std::log(asset.data_points[i].adjclose / asset.data_points[i - 1].adjclose);
        }
    }

    // ================ GENERAL ASSET ANALYSIS ================

    double avg_log_return(const assets::asset& asset){
        double sum_log_return = 0.0;
        for (size_t i = 1; i < asset.n_data_points; i++) {
            sum_log_return += asset.data_points[i].log_return;
        }
        return sum_log_return / (asset.n_data_points - 1);
    }

    double med_log_return_for_timestamp(const std::vector<assets::asset>& assets, uint32_t timestamp){
        std::vector<double> cross_asset_returns;
        cross_asset_returns.reserve(assets.size());
        for (const auto& asset : assets) {
            for (size_t i = 1; i < static_cast<size_t>(asset.n_data_points); i++) {
                if (asset.data_points[i].timestamp == timestamp) {
                    cross_asset_returns.push_back(asset.data_points[i].log_return);
                    break;
                }
            }
        }
        if (cross_asset_returns.empty()) return 0.0;
        size_t mid = cross_asset_returns.size() / 2;
        std::nth_element(cross_asset_returns.begin(), cross_asset_returns.begin() + mid, cross_asset_returns.end());
        return cross_asset_returns[mid];
    }


    //=============== COVARIANCE MATRIX & RELATED ================

    void fill_cov_mat_precompute(precomputed_voc_mat_data& data, const std::vector<assets::asset>& assets){
        for (const auto& asset : assets) {
            std::unordered_map<uint32_t, double> lr_map;
            for (int i = 1; i < asset.n_data_points; ++i) {
                uint32_t ts = asset.data_points[i].timestamp;
                lr_map[ts] = asset.data_points[i].log_return;
                data.total_count_map[ts]++;
                if(asset.data_points[i].log_return < 0.0){
                    data.neg_count_map[ts]++;
                } else if(data.neg_count_map.contains(ts) == false){
                    data.neg_count_map[ts] = 0;
                }
            }
            data.log_return_maps.push_back(std::move(lr_map));
        }
        for (const auto& [ts, _] : data.log_return_maps[0]) data.common_timestamps.insert(ts);
        for (size_t i = 1; i < data.log_return_maps.size(); ++i) {
            for (const auto& [ts, _] : data.log_return_maps[i]) {
                std::erase_if(data.common_timestamps, [&](uint32_t t){ return data.log_return_maps[i].count(ts) == 0; });
            }
            if (data.common_timestamps.empty()) return;
        }
    }

    

    std::unordered_map<uint32_t, double> avg_log_return_timestamp_map(const precomputed_voc_mat_data& data){
        std::unordered_map<uint32_t, std::pair<double, size_t>> ts_accum;
        for (const auto& asset_lr_map : data.log_return_maps) {
             for (const auto& [ts, lr] : asset_lr_map) {
                ts_accum[ts].first  += lr;
                ts_accum[ts].second += 1;
            }
        }
        std::unordered_map<uint32_t, double> result;
        result.reserve(ts_accum.size());
        for (const auto& [ts, acc] : ts_accum) {
            result[ts] = acc.first / static_cast<double>(acc.second);
        }
        return result;
    }

    std::unordered_map<uint32_t, double> rolling_20d_equal_weighted_avg_log_return(const precomputed_voc_mat_data& data){
        std::unordered_map<uint32_t, double> ts_returns = avg_log_return_timestamp_map(data);
        std::vector<uint32_t> timestamps(ts_returns.size());
        std::transform(ts_returns.begin(), ts_returns.end(), timestamps.begin(), [](const auto& pair){ return pair.first; });
        std::sort(timestamps.begin(), timestamps.end());
        std::unordered_map<uint32_t, double> result;
        
        for(size_t i = 20; i < timestamps.size(); i++){
            double sum = 0.0;
            for(size_t j = i - 20; j < i; j++){
                sum += ts_returns[timestamps[j]];
            }
            result[timestamps[i]] = sum / 20.0;
        }
        return result;
    }

    std::vector<std::vector<double>> aligned_log_returns(const precomputed_voc_mat_data& data, const std::unordered_set<uint32_t>* const timestamps = nullptr){ 
        std::vector<std::vector<double>> result;

        std::unordered_set<uint32_t> matching_timestamps;
        for(const auto& ts : data.common_timestamps){
            if(timestamps && !timestamps->contains(ts)) continue;
            matching_timestamps.insert(ts);
        }
        if(matching_timestamps.empty()) return {};

        for (uint32_t ts : matching_timestamps) {
            std::vector<double> day_returns;
            for (const auto& lr_map : data.log_return_maps) {
                day_returns.push_back(lr_map.at(ts));
            }
            result.push_back(std::move(day_returns));
        }
        return result;
    }

    double get_bottom_x_percentile(std::vector<double>& data, double x){
        if(data.empty()) return 0.0;
        size_t idx = static_cast<size_t>(std::floor(x * data.size()));
        std::nth_element(data.begin(), data.begin() + idx, data.end());
        return data[idx];
    }

   // 20d_avg_return <= 10% quantil
   // > 70% of assets have negative return
    std::unordered_set<uint32_t> compute_stressed_timestamps(const precomputed_voc_mat_data& data, double btm_percentile_treshhold = 0.1, double min_negative_log_return_pct = 0.7){

        std::unordered_map<uint32_t, double> rolling_log_r_avg = rolling_20d_equal_weighted_avg_log_return(data);
        std::vector<double> avg_returns(rolling_log_r_avg.size());
        std::transform(rolling_log_r_avg.begin(), rolling_log_r_avg.end(), avg_returns.begin(), [](const auto& pair){ return pair.second; });
        double btm_10th_percentile = get_bottom_x_percentile(avg_returns, btm_percentile_treshhold);

        std::unordered_set<uint32_t> stressed_timestamps;

        for (const auto& [ts, avg] : rolling_log_r_avg) {
            if (avg <= btm_10th_percentile) {
                double neg_pct = static_cast<double>(data.neg_count_map.at(ts)) / data.total_count_map.at(ts);
                if (neg_pct >= min_negative_log_return_pct && data.total_count_map.at(ts) / data.n_assets > min_negative_log_return_pct) {
                    stressed_timestamps.insert(ts);
                }
            }
        }
        std::cout << "Identified " << stressed_timestamps.size() << " stressed timestamps out of " << rolling_log_r_avg.size() << " total timestamps." << std::endl;
        return stressed_timestamps;
    }
    /*
        ENTRY POINT FOR COVARIANCE MATRIX CALCULATION
        If stressed is false, we compute the covariance matrix using all common timestamps
    */
    Mat compute_covariance_matrix(const std::vector<assets::asset>& assets, bool stressed = false){
        size_t n_assets = assets.size();
        Mat cov_matrix(n_assets, n_assets);

        precomputed_voc_mat_data precompute_data;
        precompute_data.n_assets = n_assets;
        fill_cov_mat_precompute(precompute_data, assets);

        std::unordered_set<uint32_t> stressed_timestamps;
        if (stressed) {
            stressed_timestamps = compute_stressed_timestamps(precompute_data);
        }

        auto daily_returns = stressed
            ? aligned_log_returns(precompute_data, &stressed_timestamps)
            : aligned_log_returns(precompute_data);

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

    std::pair<Mat, Mat> compute_cov_matricies(const std::vector<assets::asset>& assets){
        size_t n_assets = assets.size();

        precomputed_voc_mat_data precompute_data;
        precompute_data.n_assets = n_assets;
        fill_cov_mat_precompute(precompute_data, assets);

        std::unordered_set<uint32_t> stressed_timestamps = compute_stressed_timestamps(precompute_data);

        std::vector<uint32_t> sorted_ts(precompute_data.common_timestamps.begin(), precompute_data.common_timestamps.end());
        std::sort(sorted_ts.begin(), sorted_ts.end());
        const size_t T = sorted_ts.size();

        std::vector<Eigen::Index> stressed_rows;
        stressed_rows.reserve(stressed_timestamps.size());
        for (size_t t = 0; t < T; ++t) {
            if (stressed_timestamps.count(sorted_ts[t]))
                stressed_rows.push_back(static_cast<Eigen::Index>(t));
        }

        if (T < 2 || stressed_rows.size() < 2) {
            Mat zero = Mat::Zero(n_assets, n_assets);
            return {zero, zero};
        }

        Mat Return_Mat(T, n_assets);
        for (size_t t = 0; t < T; ++t)
            for (size_t k = 0; k < n_assets; ++k)
                Return_Mat(t, static_cast<Eigen::Index>(k)) = precompute_data.log_return_maps[k].at(sorted_ts[t]);

        // Normal covariance via Eigen BLAS matrix multiply
        Vec mean = Return_Mat.colwise().mean();
        Mat Return_sub_mean = Return_Mat.rowwise() - mean.transpose();
        Mat cov_matrix = (Return_sub_mean.transpose() * Return_sub_mean) / static_cast<double>(T - 1);

        const size_t T_s = stressed_rows.size();
        Mat Return_sub_mean_stressed(T_s, n_assets);
        for (size_t si = 0; si < T_s; ++si)
            Return_sub_mean_stressed.row(static_cast<Eigen::Index>(si)) = Return_Mat.row(stressed_rows[si]);

        Vec mean_s = Return_sub_mean_stressed.colwise().mean();
        Mat X_s_c = Return_sub_mean_stressed.rowwise() - mean_s.transpose();
        Mat cov_matrix_stressed = (X_s_c.transpose() * X_s_c) / static_cast<double>(T_s - 1);

        return {cov_matrix, cov_matrix_stressed};
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

    Mat cholesky_decomp(const Mat& cov_mat){
        Eigen::LLT<Mat> llt(cov_mat);
        if(llt.info() == Eigen::NumericalIssue){
            throw std::runtime_error("Covariance matrix is not positive definite");
        }
        return llt.matrixL();
    }

    // =============== OTHER ASSET METRICS ================

    double volatility(const assets::asset& asset){
        double avg_return = avg_log_return(asset);
        double volatility_sum = 0.0;
        for(size_t i = 1; i < (size_t)asset.n_data_points; i++){
            double log_ret = asset.data_points[i].log_return;
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
            double z = (asset.data_points[i].log_return - mu) / sigma;
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
            sum += std::pow((asset.data_points[i].log_return - avg) / vol, 3.0);
        }
        return sum / (asset.n_data_points - 1);
    }


    //======== DEPRECATED ==============
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

    [[deprecated]] int32_t count_assets_for_timestamp(const std::vector<assets::asset>& assets, uint32_t timestamp){
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

    [[deprecated]] size_t count_negative_log_returns_for_timestamp(const std::vector<assets::asset>& assets, uint32_t timestamp){
        size_t count = 0;
        for (const auto& asset : assets) {
            for (size_t i = 1; i < static_cast<size_t>(asset.n_data_points); i++) {
                if (asset.data_points[i].timestamp == timestamp) {
                     if(asset.data_points[i].log_return < 0.0) count++;
                    break;
                }
            }
        }
        return count;
    }

}