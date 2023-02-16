// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QIMAGE_PARSER_H
#define QIMAGE_PARSER_H

#include "QImageParserInterface.h"
#include "QLogger.h"

namespace qaic {

class QImageParser : public QImageParserInterface, public QLogger {
public:
  QImageParser() : QLogger("QImageParser"){};
  ~QImageParser() = default;

  /// Parse the network image and return an QIMedtaData object which
  /// contains a template of partically constructed request elements.
  /// The template can be used to update request elements by filling
  /// in the missing fields like request ID and src/dst address.
  std::unique_ptr<QMetaDataInterface> parseImage(const QBuffer &buf,
                                                 QStatus &status) override;
  /// The buffer contains unparsed metadata
  std::unique_ptr<QMetaDataInterface> parseBuffer(const QBuffer &buf,
                                                  QStatus &status) override;

private:
  std::unique_ptr<QMetaDataInterface> parse(uint8_t *metaBuf, uint32_t metaSize,
                                            QStatus &status);
};

} // namespace qaic

#endif // QIMAGE_PARSER_H
