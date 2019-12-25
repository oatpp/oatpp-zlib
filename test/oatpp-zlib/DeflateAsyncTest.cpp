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

#include "DeflateAsyncTest.hpp"

#include "oatpp-zlib/Deflate.hpp"

#include "oatpp/core/data/stream/BufferStream.hpp"
#include "oatpp/core/utils/Random.hpp"

#include "oatpp-test/Checker.hpp"

namespace oatpp { namespace test { namespace zlib {

namespace {

class AsyncStreamReader : public oatpp::data::stream::AsyncReadCallback {
private:
  std::shared_ptr<oatpp::data::stream::InputStream> m_stream;
public:

  AsyncStreamReader(const std::shared_ptr<oatpp::data::stream::InputStream>& stream)
    : m_stream(stream)
  {}

  oatpp::async::Action readAsyncInline(oatpp::data::stream::AsyncInlineReadData& inlineData, oatpp::async::Action&& nextAction) override {

    auto res = m_stream->read(inlineData.currBufferPtr, inlineData.bytesLeft);

    if(res > 0) {
      inlineData.inc(res);
      return std::forward<oatpp::async::Action>(nextAction);
    }

    if(res == oatpp::data::IOError::RETRY_READ || res == oatpp::data::IOError::WAIT_RETRY_READ ||
       res == oatpp::data::IOError::RETRY_WRITE || res == oatpp::data::IOError::WAIT_RETRY_WRITE) {
      return m_stream->suggestInputStreamAction(res);
    }

    return std::forward<oatpp::async::Action>(nextAction);

  }

};

class TestIterationCoroutine : public oatpp::async::Coroutine<TestIterationCoroutine> {
private:
  oatpp::String m_text;
  v_buff_size m_chunkSize;
  v_buff_size m_bufferSize;
  bool m_useGzip;
public:

  TestIterationCoroutine(const oatpp::String& text, v_buff_size chunkSize, v_buff_size bufferSize, bool useGzip)
    : m_text(text)
    , m_chunkSize(chunkSize)
    , m_bufferSize(bufferSize)
    , m_useGzip(useGzip)
  {}



};

void runCompressor (bool useGzip) {

  for (v_int32 i = 1; i <= 128; i++) {
    for (v_int32 b = 1; b <= 64; b++) {

      oatpp::String original(2048);
      oatpp::utils::random::Random::randomBytes(original->getData(), original->getSize());

      auto result = oatpp::zlib::deflate(original, i, b, useGzip);
      auto check = oatpp::zlib::inflate(result, i, b, useGzip);

      if (check != original) {
        OATPP_LOGD("TEST", "Error. i=%d, b=%d", i, b);
      }

      OATPP_ASSERT(check == original);

    }
  }

}

}

void DeflateAsyncTest::onRun() {

  {
    oatpp::test::PerformanceChecker timer("Deflate");
    runCompressor(false);
  }

  {
    oatpp::test::PerformanceChecker timer("Gzip");
    runCompressor(true);
  }

}

}}}
