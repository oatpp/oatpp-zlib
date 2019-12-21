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

#include "oatpp-zlib/Deflate.hpp"
#include "oatpp/core/utils/Random.hpp"

namespace oatpp { namespace test { namespace zlib {

void DeflateTest::onRun() {

  for (v_int32 i = 1; i <= 128; i++) {
    for (v_int32 b = 1; b <= 64; b++) {

      oatpp::String original(2048);
      oatpp::utils::random::Random::randomBytes(original->getData(), original->getSize());

      auto result = oatpp::zlib::deflate(original, i, b);
      auto check = oatpp::zlib::inflate(result, i, b);

      if (check != original) {
        OATPP_LOGD(TAG, "Error. i=%d, b=%d", i, b);
      }

      OATPP_ASSERT(check == original);

    }
  }

}

}}}
