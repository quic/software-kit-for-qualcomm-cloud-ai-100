// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

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

} // namespace QAicOpenRtContextUnitTest