#pragma once
#include "../models/assets.h"
#include <cmath>
#include <vector>
#include <eigen3/Eigen/Dense>

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

    std::vector<std::pair<double,double>> log_return_aligned_assets(assets::asset& asset1, assets::asset& asset2) {
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

    Mat compute_covariance_matrix(const std::vector<assets::asset>& assets){
        size_t n_assets = assets.size();
        Mat cov_matrix(n_assets, n_assets);
        for (size_t i = 0; i < n_assets; i++) {
            for (size_t j = i; j < n_assets; j++) {
                auto aligned_returns = log_return_aligned_assets(const_cast<assets::asset&>(assets[i]), const_cast<assets::asset&>(assets[j]));
                if (aligned_returns.size() < 2) {
                    cov_matrix(i, j) = 0.0;
                    cov_matrix(j, i) = 0.0;
                    continue;
                }
                //aligned subset only
                double mean_i = 0.0, mean_j = 0.0;
                for (const auto& p : aligned_returns) {
                    mean_i += p.first;
                    mean_j += p.second;
                }
                mean_i /= aligned_returns.size();
                mean_j /= aligned_returns.size();

                double cov_ij = 0.0;
                for (const auto& p : aligned_returns) {
                    cov_ij += (p.first - mean_i) * (p.second - mean_j);
                }
                cov_matrix(i, j) = cov_ij / static_cast<double>(aligned_returns.size() - 1);
                cov_matrix(j, i) = cov_matrix(i, j); //Symmetric matrix
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

    double excess_kurtosis(const assets::asset& asset){
        double avg_return = avg_log_return(asset);
        double vol = volatility(asset);
        double kurtosis_sum = 0.0;
        for(size_t i = 1; i < asset.n_data_points; i++){
            kurtosis_sum += std::pow((asset.data_points[i].adjclose - avg_return) / vol, 4);
        }
         return kurtosis_sum / (asset.n_data_points - 1) - 3.0; //Excess kurtosis is kurtosis - 3
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