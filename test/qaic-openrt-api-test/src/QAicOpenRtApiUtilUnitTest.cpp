// Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <limits>

#include "QAicOpenRtUnitTestBase.hpp"
#include "QAicOpenRtUtil.hpp"
#include "QUtil.h"

namespace QAicOpenRtUnitTest {

class QAicOpenRtApiUtilsUnitTest : public QAicOpenRtUnitTestBase {

public:
  QAicOpenRtApiUtilsUnitTest(){};
  virtual ~QAicOpenRtApiUtilsUnitTest() = default;

  QAicOpenRtApiUtilsUnitTest(const QAicOpenRtApiUtilsUnitTest &) =
      delete; // Disable Copy Constructor
  QAicOpenRtApiUtilsUnitTest &operator=(const QAicOpenRtApiUtilsUnitTest &) =
      delete; // Disable Assignment Operator

protected:
  void TestUtilsQueryDeviceList();
  void TestUtilsQueryDeviceResources();
  void TestUtilsQueryDevicePerformance();

}; // class QAicOpenRtApiUtilsUnitTest

void QAicOpenRtApiUtilsUnitTest::TestUtilsQueryDeviceList() {
  qaic::openrt::Util util;
  std::vector<QID> devIdList;
  QStatus status;
  status = util.getDeviceIds(devIdList);
  ASSERT_TRUE(status == QS_SUCCESS);
  LogInfo("Found num devices {}", devIdList.size());
}

void QAicOpenRtApiUtilsUnitTest::TestUtilsQueryDeviceResources() {
  qaic::openrt::Util util;
  std::vector<QID> devIdList;
  QStatus status;
  QResourceInfo devResourceInfo;
  status = util.getDeviceIds(devIdList);
  ASSERT_TRUE(status == QS_SUCCESS);
  LogInfo("Found num devices {}", devIdList.size());
  for (auto &dev : devIdList) {
    util.getResourceInfo(dev, devResourceInfo);
    LogInfo("Device {} resource info:", dev);
    LogInfo("{}", qaic::qutil::str(devResourceInfo));
  }
}

void QAicOpenRtApiUtilsUnitTest::TestUtilsQueryDevicePerformance() {
  qaic::openrt::Util util;
  std::vector<QID> devIdList;
  QStatus status;
  QPerformanceInfo devPerformanceInfo;
  status = util.getDeviceIds(devIdList);
  ASSERT_TRUE(status == QS_SUCCESS);
  LogInfo("Found num devices {}", devIdList.size());
  for (auto &dev : devIdList) {
    util.getPerformanceInfo(dev, devPerformanceInfo);
    LogInfo("Device {} Performance info:", dev);
    LogInfo("{}", qaic::qutil::str(devPerformanceInfo));
  }
}

TEST_F(QAicOpenRtApiUtilsUnitTest, DeviceListTest) {
  TestUtilsQueryDeviceList();
}

TEST_F(QAicOpenRtApiUtilsUnitTest, DeviceResourceTest) {
  TestUtilsQueryDeviceResources();
}

TEST_F(QAicOpenRtApiUtilsUnitTest, DevicePerformanceTest) {
  TestUtilsQueryDevicePerformance();
}

TEST_F(QAicOpenRtApiUtilsUnitTest, CheckSkuTypeToStringConversion) {
    ASSERT_STREQ("Invalid", qaic::qutil::getSkuTypeStr(0).c_str());
    ASSERT_STREQ("M.2", qaic::qutil::getSkuTypeStr(1).c_str());
    ASSERT_STREQ("PCIe", qaic::qutil::getSkuTypeStr(2).c_str());
    ASSERT_STREQ("PCIe Pro", qaic::qutil::getSkuTypeStr(3).c_str());
    ASSERT_STREQ("PCIe Lite", qaic::qutil::getSkuTypeStr(4).c_str());
    ASSERT_STREQ("PCIe Ultra", qaic::qutil::getSkuTypeStr(5).c_str());
    ASSERT_STREQ("Auto", qaic::qutil::getSkuTypeStr(6).c_str());
    ASSERT_STREQ("PCIe Ultra Plus", qaic::qutil::getSkuTypeStr(7).c_str());
    ASSERT_STREQ("PCIe 080", qaic::qutil::getSkuTypeStr(8).c_str());
    ASSERT_STREQ("PCIe Ultra 080", qaic::qutil::getSkuTypeStr(9).c_str());
    ASSERT_STREQ("Invalid", qaic::qutil::getSkuTypeStr(10).c_str());
    ASSERT_STREQ("Invalid", qaic::qutil::getSkuTypeStr(std::numeric_limits<std::uint8_t>::max()).c_str());
}

} // namespace QAicOpenRtContextUnitTest
