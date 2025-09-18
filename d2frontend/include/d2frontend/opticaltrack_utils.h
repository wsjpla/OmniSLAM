#pragma once
#include <opencv2/opencv.hpp>

#include <chrono>
#include <d2common/d2basetypes.h>
#include <d2common/d2landmarks.h>
#include <d2frontend/utils.h>
#include <spdlog/spdlog.h>

#define PYR_LEVEL 2

namespace D2FrontEnd {
using D2Common::LandmarkIdType;
using LandmarkType = D2Common::LandmarkType;

template <typename T> struct LKImageInfo {
    std::vector<cv::Point2f> lk_pts;
    std::vector<Eigen::Vector3d> lk_pts_3d_norm;
    std::vector<LandmarkIdType> lk_ids;
    std::vector<int> lk_local_index;
    std::vector<LandmarkType> lk_types;
    std::vector<T> pyr;
};

using LKImageInfoCPU = LKImageInfo<cv::Mat>;
using LKImageInfoGPU = LKImageInfo<cv::cuda::GpuMat>;

void detectPoints(const cv::Mat &img, std::vector<cv::Point2f> &n_pts,
                  const std::vector<cv::Point2f> &cur_pts, int require_pts,
                  bool enable_cuda = true, bool use_fast = false,
                  int fast_rows = 3, int fast_cols = 4);

std::vector<cv::cuda::GpuMat> buildImagePyramid(const cv::cuda::GpuMat &prevImg,
                                                int maxLevel_ = 3);

std::vector<cv::Mat> buildImagePyramid(const cv::Mat &prevImg,
                                                int maxLevel_ = 3);

std::vector<cv::Point2f> opticalflowTrack(const cv::Mat &cur_img,
                                          const cv::Mat &prev_img,
                                          std::vector<cv::Point2f> &prev_pts,
                                          std::vector<LandmarkIdType> &ids,
                                          TrackLRType type = WHOLE_IMG_MATCH,
                                          bool enable_cuda = true);

LKImageInfoGPU opticalflowTrackPyr(
    const cv::Mat &cur_img, const LKImageInfoGPU& prev_lk,
    TrackLRType type = WHOLE_IMG_MATCH);

LKImageInfoCPU opticalflowTrackPyr(
    const cv::Mat &cur_img, const LKImageInfoCPU& prev_lk,
    TrackLRType type = WHOLE_IMG_MATCH);

std::vector<cv::DMatch>
matchKNN(const cv::Mat &desc_a, const cv::Mat &desc_b,
         double knn_match_ratio = 0.8,
         const std::vector<cv::Point2f> pts_a = std::vector<cv::Point2f>(),
         const std::vector<cv::Point2f> pts_b = std::vector<cv::Point2f>(),
         double search_local_dist = -1);

template <typename LKImageInfo>
void removeNearPoints(LKImageInfo& info, float near_lk_thread_rate) {
    std::vector<cv::Point2f> new_pts;
    std::vector<bool> status;
    int remove_count = 0;
    for (size_t i = 0; i < info.lk_pts.size(); i++) {
        bool has_nearby = false;
        for (size_t j = 0; j < new_pts.size(); j++) {
            if (cv::norm(info.lk_pts[i] - new_pts[j]) < near_lk_thread_rate) {
                has_nearby = true;
                break;
            }
        }
        if (!has_nearby) {
            new_pts.push_back(info.lk_pts[i]);
            status.push_back(true);
        }
        else {
            status.push_back(false);
            remove_count++;
        }
    }
    reduceVector(info.lk_pts, status);
    reduceVector(info.lk_pts_3d_norm, status);
    reduceVector(info.lk_ids, status);
    reduceVector(info.lk_local_index, status);
    reduceVector(info.lk_types, status);
    SPDLOG_DEBUG("removeNearPoints {}->{} thre: {}", info.lk_pts.size() + remove_count, info.lk_pts.size(), near_lk_thread_rate);
}

} // namespace D2FrontEnd
