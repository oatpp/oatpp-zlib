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

#ifndef oatpp_zlib_deflate_hpp
#define oatpp_zlib_deflate_hpp

#include "oatpp/core/data/stream/Stream.hpp"
#include "zlib.h"

namespace oatpp { namespace zlib {

/**
 * Proxy reader to read and deflate source.
 */
class DeflateStreamReader : public oatpp::data::stream::ReadCallback {
private:
  static constexpr v_int32 STATE_WAITING = 0;
  static constexpr v_int32 STATE_READING = 1;
  static constexpr v_int32 STATE_FINISHING = 2;
  static constexpr v_int32 STATE_DONE = 3;
private:
  data::stream::ReadCallback* m_sourceCallback;
  v_int32 m_compressionLevel;
  v_buff_size m_chunkBufferSize;
  bool m_useGzip;
  v_int32 m_state;
  z_stream m_zStream;
private:
  bool finish();
public:

  /**
   * Constructor.
   * @param sourceCallback - &id:oatpp::data::stream::ReadCallback;.
   */
  DeflateStreamReader(data::stream::ReadCallback* sourceCallback,
                      v_int32 compressionLevel = Z_DEFAULT_COMPRESSION,
                      v_buff_size chunkBufferSize = 1024,
                      bool useGzip = false);

  /**
   * Read deflated buffer from source callback.
   * @param buffer - buffer to store read bytes.
   * @param count - size buffer.
   * @return - &id:oatpp::data::v_io_size;.
   */
  data::v_io_size read(void *buffer, v_buff_size count) override;

};

/**
 * Proxy reader to read and inflate source.
 */
class InflateStreamReader : public oatpp::data::stream::ReadCallback {
private:
  static constexpr v_int32 STATE_WAITING = 0;
  static constexpr v_int32 STATE_READING = 1;
  static constexpr v_int32 STATE_FINISHING = 2;
  static constexpr v_int32 STATE_DONE = 3;
private:
  data::stream::ReadCallback* m_sourceCallback;
  v_buff_size m_chunkBufferSize;
  bool m_useGzip;
  v_int32 m_state;
  z_stream m_zStream;
private:
  bool finish();
public:

  /**
   * Constructor.
   * @param sourceCallback - &id:oatpp::data::stream::ReadCallback;.
   */
  InflateStreamReader(data::stream::ReadCallback* sourceCallback,
                      v_buff_size chunkBufferSize = 1024,
                      bool useGzip = false);

  /**
   * Read deflated buffer from source callback.
   * @param buffer - buffer to store read bytes.
   * @param count - size buffer.
   * @return - &id:oatpp::data::v_io_size;.
   */
  data::v_io_size read(void *buffer, v_buff_size count) override;

};

oatpp::String deflate(const oatpp::String& text, v_buff_size chunkSize, v_buff_size bufferSize, bool useGzip);
oatpp::String inflate(const oatpp::String& text, v_buff_size chunkSize, v_buff_size bufferSize, bool useGzip);

}}

#endif // oatpp_zlib_deflate_hpp
