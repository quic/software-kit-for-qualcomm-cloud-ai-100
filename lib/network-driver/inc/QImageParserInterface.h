// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QI_IMANGE_PARSER_H
#define QI_IMANGE_PARSER_H

#include "QAicRuntimeTypes.h"

#include <memory>

namespace qaic {

class QMetaDataInterface;

/// This interface provides a function parseImage() which parses a buffer
/// and returns a pointer to QMetaDataInterface object. QMetaDataInterface has a
/// template
/// to update a list of request elements with predefined data.
class QImageParserInterface {
public:
  QImageParserInterface() = default;
  virtual std::unique_ptr<QMetaDataInterface> parseImage(const QBuffer &buf,
                                                         QStatus &status) = 0;

  virtual std::unique_ptr<QMetaDataInterface> parseBuffer(const QBuffer &buf,
                                                          QStatus &status) = 0;

  virtual ~QImageParserInterface() = default;
};

} // namespace qaic

#endif // QIIMAGEPARSER_H
