/***************************************************************************
 *
 * Project         _____    __   ____   _      _
 *                (  _  )  /__\ (_  _)_| |_  _| |_
 *                 )(_)(  /(__)\  )( (_   _)(_   _)
 *                (_____)(__)(__)(__)  |_|    |_|
 *
 *
 * Copyright 2018-present, Leonid Stryzhevskyi <lganzzzo@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ***************************************************************************/

#include "DeflateTest.hpp"

#include "oatpp-zlib/Processor.hpp"
#include "oatpp/core/utils/Random.hpp"
#include "oatpp/core/data/stream/BufferStream.hpp"

#include "oatpp-test/Checker.hpp"

namespace oatpp { namespace test { namespace zlib {

namespace {

void runCompressorPipeline (bool gzip) {

  for (v_int32 e = 1; e <= 64; e++) {
    for (v_int32 d = 1; d <= 64; d++) {

      oatpp::String original(1024);
      oatpp::utils::random::Random::randomBytes(original->getData(), original->getSize());

      oatpp::data::stream::BufferInputStream inStream(original);
      oatpp::data::stream::BufferOutputStream outStream;

      oatpp::zlib::DeflateEncoder encoder(e, gzip);
      oatpp::zlib::DeflateDecoder decoder(d, gzip);

      oatpp::data::buffer::ProcessingPipeline pipeline({
                                                         &encoder,
                                                         &decoder
                                                       });

      oatpp::data::buffer::IOBuffer buffer;
      auto res = oatpp::data::stream::transfer(&inStream, &outStream, 0, buffer.getData(), buffer.getSize(), &pipeline);

      auto check = outStream.toString();

      if (check != original) {
        OATPP_LOGD("TEST", "Error. e=%d, d=%d, res=%d", e, d, res);
      }

      OATPP_ASSERT(check == original);

    }
  }

}

void runCompressor (bool gzip) {

  for (v_int32 e = 1; e <= 64; e++) {
    for (v_int32 d = 1; d <= 64; d++) {

      oatpp::String original(1024);
      oatpp::utils::random::Random::randomBytes(original->getData(), original->getSize());

      oatpp::data::buffer::IOBuffer buffer;

      oatpp::data::stream::BufferInputStream inStream(original);
      oatpp::data::stream::BufferOutputStream outEncoded;

      oatpp::zlib::DeflateEncoder encoder(e, gzip);

      oatpp::data::stream::transfer(&inStream, &outEncoded, 0, buffer.getData(), buffer.getSize(), &encoder);

      oatpp::data::stream::BufferInputStream inEncoded(outEncoded.toString());
      oatpp::data::stream::BufferOutputStream outStream;

      oatpp::zlib::DeflateDecoder decoder(d, gzip);

      oatpp::data::stream::transfer(&inEncoded, &outStream, 0, buffer.getData(), buffer.getSize(), &decoder);

      auto check = outStream.toString();

      if (check != original) {
        OATPP_LOGD("TEST", "Error. e=%d, d=%d", e, d);
      }

      OATPP_ASSERT(check == original);

    }
  }

}

}

void DeflateTest::onRun() {

  {
    oatpp::test::PerformanceChecker timer("Deflate");
    runCompressor(false);
  }

  {
    oatpp::test::PerformanceChecker timer("Gzip");
    runCompressor(true);
  }

  {
    oatpp::test::PerformanceChecker timer("Deflate - pipeline");
    runCompressorPipeline(false);
  }

  {
    oatpp::test::PerformanceChecker timer("Gzip - pipeline");
    runCompressorPipeline(true);
  }

}

}}}
