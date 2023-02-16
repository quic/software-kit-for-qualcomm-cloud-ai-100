// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear
#ifndef QUTIL_H
#define QUTIL_H

#include "QAicRuntimeTypes.h"
#include "QAicHostApiInfo.h"
#include "spdlog/spdlog.h"
#include "QTypes.h"

#include <cstddef>
#include <memory>
#include <sstream>
#include <type_traits>
#include <utility>
#include <numeric>

namespace qaic {

namespace qutil {

extern const uint8_t MAX_NUM_NSP;

std::string hex(uint64_t n);

const char *statusStr(const QStatus &status);

/// Convert a uint32_t to string. The uint32_t is interpretted this way:
/// [0:15] build, [16:23] minor, [24:31] major.
std::string uint32VerToStr(uint32_t version);

const char *devStatusToStr(QDevStatus devStatus);

std::string uint32ToPCIeStr(uint32_t pcie);
std::string pcieLinkSpeed(uint32_t maxSpeed);
std::string qPciInfoToPCIeStr(QPciInfo pciInfo);

std::string str(const QDevInfo &info);
std::string str(const QResourceInfo &info);
std::string str(const QPerformanceInfo &info);
std::string str(const QBoardInfo &info);

void convertToHostFormat(const host_api_info_data_header_internal_t &source,
                         QHostApiInfoDevData &target);
void convertToHostFormat(const host_api_info_nsp_static_data_internal_t &source,
                         QHostApiInfoNspVersion &target);
void convertToHostFormat(const host_api_info_nsp_data_internal_t &source,
                         QHostApiInfoNspData &target);

// dev_data to ApiInfoDevData
void convertToHostFormat(const host_api_info_dev_data_internal_t &source,
                         const QDeviceFeatureBitset &deviceFeatures,
                         QHostApiInfoDevData &target);

// dev_data to BoardInfo
void convertToHostFormat(const host_api_info_dev_data_internal_t &source,
                         const QDeviceFeatureBitset &deviceFeatures,
                         QBoardInfo &target);

const std::int32_t BROADCAST_QID = -1;
const std::int32_t INVALID_QID = -2;
const std::uint32_t INVALID_VCID = std::numeric_limits<uint32_t>::max();
const std::uint64_t INVALID_BUF_FD = -1ULL;
constexpr uint8_t qidDerivedBase = 100;

template <class T> class stats {
public:
  stats()
      : sum_(std::numeric_limits<T>::min()),
        avg_(std::numeric_limits<T>::min()),
        min_(std::numeric_limits<T>::max()),
        max_(std::numeric_limits<T>::min()),
        median_(std::numeric_limits<T>::min()),
        tile90pct_(std::numeric_limits<T>::min()) {}
  const T min() const { return min_; }
  const T max() const { return max_; }
  const T avg() const { return avg_; }
  const T stddev() const { return std_dev_; }
  const T cv() const { return cv_; }
  const T median() const { return median_; }
  const T tile90pct() const { return tile90pct_; }
  const T tile10pct() const { return tile10pct_; }
  uint32_t numSamples() const { return samples_.size(); }
  void addSample(T value) {
    samples_.emplace_back(value);
    max_ = std::max(max_, value);
    min_ = std::min(min_, value);
  }
  // Note, STD Dev is not the most efficient method, use for testing
  void calc() {
    sum_ = std::accumulate(samples_.begin(), samples_.end(), 0.0);
    avg_ = sum_ / samples_.size();
    diffs_.clear();
    diffs_.resize(samples_.size());
    std::transform(samples_.begin(), samples_.end(), diffs_.begin(),
                   [&](T x) { return x - avg_; });
    sq_sum_ = std::inner_product(diffs_.begin(), diffs_.end(), diffs_.begin(),
                                 std::numeric_limits<T>::min());
    std_dev_ = std::sqrt(sq_sum_ / samples_.size());
    cv_ = (std_dev_ / avg_);

    std::sort(samples_.begin(), samples_.end());
    tile90pct_ = samples_[samples_.size() * 0.9];
    tile10pct_ = samples_[samples_.size() * 0.1];
    median_ = samples_[samples_.size() * 0.5];
  }

private:
  T sum_;
  T avg_;
  T min_;
  T max_;
  T median_;
  T tile10pct_;
  T tile90pct_;
  T sq_sum_;
  T std_dev_;
  T cv_;
  std::vector<T> diffs_;
  std::vector<T> samples_;
};

static inline void initQBuffer(QBuffer &qBuffer) {
  qBuffer = {0, nullptr, 0, 0, QBufferType::QBUFFER_TYPE_HEAP};
}

using DeviceAddrInfo = std::pair<QID, std::string>;

enum DeviceFeatures {
  FW_FUSA_CRC_ENABLED = 0,
};

} // namespace qutil

} // namespace qaic

#endif // QUTIL_H
