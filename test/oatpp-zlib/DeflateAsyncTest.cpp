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

#include "oatpp-zlib/Processor.hpp"
#include "oatpp/utils/Random.hpp"
#include "oatpp/data/stream/BufferStream.hpp"

#include "oatpp/async/Executor.hpp"

namespace oatpp { namespace test { namespace zlib {

namespace {

class TestCoroutine : public oatpp::async::Coroutine<TestCoroutine> {
private:
  bool m_gzip;
private:
  oatpp::String m_original;
  oatpp::data::stream::BufferInputStream m_inStream;
  oatpp::data::stream::BufferOutputStream m_outStream;
private:
  v_int32 m_e;
  v_int32 m_d;
public:

  TestCoroutine(bool gzip)
    : m_gzip(gzip)
    , m_original(1024)
    , m_inStream(m_original)
    , m_e(1)
    , m_d(0)
  {}

  Action act() {

    m_d ++;

    if(m_d > 64) {
      m_d = 1;
      m_e ++;
    }

    if(m_e > 64) {
      return finish();
    }

    oatpp::utils::Random::randomBytes((p_char8)m_original->data(), m_original->size());
    m_inStream.reset(m_original.getPtr(), (p_char8)m_original->data(), m_original->size());
    m_outStream.setCurrentPosition(0);
    return yieldTo(&TestCoroutine::runPipeline);
  }

  Action runPipeline() {
    auto encoder = std::make_shared<oatpp::zlib::DeflateEncoder>(m_e, m_gzip);
    auto decoder = std::make_shared<oatpp::zlib::DeflateDecoder>(m_d, m_gzip);

    auto pipeline = std::shared_ptr<oatpp::data::buffer::Processor>(new oatpp::data::buffer::ProcessingPipeline({
      oatpp::base::ObjectHandle<oatpp::data::buffer::Processor>(encoder),
      oatpp::base::ObjectHandle<oatpp::data::buffer::Processor>(decoder)
    }));

    auto buffer = std::make_shared<oatpp::data::buffer::IOBuffer>();
    return oatpp::data::stream::transferAsync(&m_inStream, &m_outStream, 0, buffer, pipeline)
           .next(yieldTo(&TestCoroutine::check));
  }

  Action check() {

    auto check = m_outStream.toString();

    if (check != m_original) {
      OATPP_LOGd("TEST", "Error. e={}, d={}", m_e, m_d);
    }

    OATPP_ASSERT(check == m_original);

    return yieldTo(&TestCoroutine::act);

  }

};

}

void DeflateAsyncTest::onRun() {


  oatpp::async::Executor executor;

  executor.execute<TestCoroutine>(false);
  executor.execute<TestCoroutine>(true);

  executor.waitTasksFinished();
  executor.stop();
  executor.join();


}

}}}
