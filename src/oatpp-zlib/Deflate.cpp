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

#include "Deflate.hpp"
#include "oatpp/core/data/stream/BufferStream.hpp"

namespace oatpp { namespace zlib {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DeflateStreamReader

DeflateStreamReader::DeflateStreamReader(data::stream::ReadCallback* sourceCallback, v_int32 compressionLevel, v_buff_size chunkBufferSize, bool useGzip)
  : m_sourceCallback(sourceCallback)
  , m_compressionLevel(compressionLevel)
  , m_chunkBufferSize(chunkBufferSize)
  , m_useGzip(useGzip)
  , m_state(STATE_WAITING)
{}

bool DeflateStreamReader::finish() {
  while(m_zStream.avail_out > 0) {
    auto res = deflate(&m_zStream, Z_FINISH);
    if(res == Z_STREAM_END) {
      return true;
    } else if(res == Z_OK) {
      continue;
    } else if(res == Z_BUF_ERROR) {
      break;
    } else {
      return true; // unknown error. just finish silently.
    }
  }
  return false;
}

data::v_io_size DeflateStreamReader::read(void *buffer, v_buff_size count) {

  if(m_state == STATE_WAITING) {

    m_zStream.zalloc = Z_NULL;
    m_zStream.zfree = Z_NULL;
    m_zStream.opaque = Z_NULL;

    m_zStream.next_in = nullptr;
    m_zStream.avail_in = 0;

    v_int32 res;

    if(m_useGzip) {
      res = deflateInit2(&m_zStream,
                         m_compressionLevel,
                         Z_DEFLATED,
                         15 | 16,
                         8 /* default memory */,
                         Z_DEFAULT_STRATEGY);
    } else {
      res = deflateInit(&m_zStream, m_compressionLevel);
    }

    if(res != Z_OK) {
      return 0;
    }

    m_state = STATE_READING;

  } else if(m_state == STATE_DONE) {
    return 0;
  }

  v_buff_size chunkSize = m_chunkBufferSize;
  std::unique_ptr<v_char8[]> chunk(new v_char8[chunkSize]);

  m_zStream.next_out = (Bytef *) buffer;
  m_zStream.avail_out = (uInt) count;

  if(m_state == STATE_FINISHING) {

    if(finish()) {
      m_state = STATE_DONE;
    }

  } else {

    while (m_zStream.avail_out > 0) {

      if(m_zStream.avail_in == 0) {

        chunkSize = m_sourceCallback->read(chunk.get(), chunkSize);

        m_zStream.next_in = (Bytef *) chunk.get();
        m_zStream.avail_in = (uInt) chunkSize;

      }

      if (chunkSize > 0) {

        auto res = deflate(&m_zStream, Z_NO_FLUSH);

        if(res != Z_OK) {
          break;
        }

      } else if (chunkSize == data::IOError::RETRY_READ || chunkSize == data::IOError::WAIT_RETRY_READ ||
                 chunkSize == data::IOError::RETRY_WRITE || chunkSize == data::IOError::WAIT_RETRY_WRITE) {
        chunkSize = m_chunkBufferSize;
      } else if (chunkSize == 0) {

        m_state = STATE_FINISHING;
        if(finish()) {
          m_state = STATE_DONE;
        }
        break;

      } else if (m_zStream.avail_out < count) {
        break;
      } else if (m_zStream.avail_out == count) {
        return chunkSize; // return error
      }

    }

  }

  return count - m_zStream.avail_out;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AsyncDeflateStreamReader

AsyncDeflateStreamReader::AsyncDeflateStreamReader(const std::shared_ptr<data::stream::AsyncReadCallback>& sourceCallback,
                                                   v_int32 compressionLevel,
                                                   v_buff_size chunkBufferSize,
                                                   bool useGzip)
  : m_sourceCallback(sourceCallback)
  , m_compressionLevel(compressionLevel)
  , m_chunkBufferSize(chunkBufferSize)
  , m_useGzip(useGzip)
  , m_state(STATE_WAITING)
{}


oatpp::async::Action AsyncDeflateStreamReader::readAsyncInline(AsyncInlineReadData& inlineData, oatpp::async::Action&& nextAction) {

  class ReadAsyncCoroutine : public async::Coroutine<ReadAsyncCoroutine> {
  private:
    AsyncDeflateStreamReader* m_this;
    AsyncInlineReadData* m_outData;
  private:
    std::unique_ptr<v_char8[]> m_chunk;
    AsyncInlineReadData m_inData;
  public:

    ReadAsyncCoroutine(AsyncDeflateStreamReader* _this, AsyncInlineReadData* outData)
      : m_this(_this)
      , m_outData(outData)
    {}

    bool processFinish() {
      while(m_this->m_zStream.avail_out > 0) {
        auto res = deflate(&m_this->m_zStream, Z_FINISH);
        if(res == Z_STREAM_END) {
          return true;
        } else if(res == Z_OK) {
          continue;
        } else if(res == Z_BUF_ERROR) {
          break;
        } else {
          return true; // unknown error. just finish silently.
        }
      }
      return false;
    }

    Action act() override {

      if(m_this->m_state == STATE_WAITING) {

        m_this->m_zStream.zalloc = Z_NULL;
        m_this->m_zStream.zfree = Z_NULL;
        m_this->m_zStream.opaque = Z_NULL;

        m_this->m_zStream.next_in = nullptr;
        m_this->m_zStream.avail_in = 0;

        v_int32 res;

        if(m_this->m_useGzip) {
          res = deflateInit2(&m_this->m_zStream,
                             m_this->m_compressionLevel,
                             Z_DEFLATED,
                             15 | 16,
                             8 /* default memory */,
                             Z_DEFAULT_STRATEGY);
        } else {
          res = deflateInit(&m_this->m_zStream, m_this->m_compressionLevel);
        }

        if(res != Z_OK) {
          return finish();
        }

        m_this->m_state = STATE_READING;

      } else if(m_this->m_state == STATE_DONE) {
        return finish();
      }

      m_this->m_zStream.next_out = (Bytef *) m_outData->currBufferPtr;
      m_this->m_zStream.avail_out = (uInt) m_outData->bytesLeft;

      if(m_this->m_state == STATE_FINISHING) {

        if(processFinish()) {
          m_this->m_state = STATE_DONE;
        }

        m_outData->inc(m_outData->bytesLeft - m_this->m_zStream.avail_out);
        return finish();

      }

      m_chunk.reset(new v_char8[m_this->m_chunkBufferSize]);
      return yieldTo(&ReadAsyncCoroutine::processNext);

    }

    Action processNext() {

      if(m_this->m_zStream.avail_out == 0) {
        m_outData->inc(m_outData->bytesLeft - m_this->m_zStream.avail_out);
        return finish();
      }

      if(m_this->m_zStream.avail_in == 0) {
        m_inData.set(m_chunk.get(), m_this->m_chunkBufferSize);
        return yieldTo(&ReadAsyncCoroutine::readChunkData);
      }

      auto res = deflate(&m_this->m_zStream, Z_NO_FLUSH);

      if(res != Z_OK) {
        m_outData->inc(m_outData->bytesLeft - m_this->m_zStream.avail_out);
        return finish();
      }

      return repeat();

    }

    Action readChunkData() {
      return m_this->m_sourceCallback->readAsyncInline(m_inData, yieldTo(&ReadAsyncCoroutine::setNewInData));
    }

    Action setNewInData() {

      m_this->m_zStream.next_in = (Bytef *) m_chunk.get();
      m_this->m_zStream.avail_in = (uInt) m_this->m_chunkBufferSize - m_inData.bytesLeft;

      if(m_this->m_zStream.avail_in > 0) {
        return yieldTo(&ReadAsyncCoroutine::processNext);
      }

      m_this->m_state = STATE_FINISHING;

      if(processFinish()) {
        m_this->m_state = STATE_DONE;
      }

      m_outData->inc(m_outData->bytesLeft - m_this->m_zStream.avail_out);
      return finish();

    }

  };

  if(m_state != STATE_DONE) {
    return ReadAsyncCoroutine::start(this, &inlineData).next(std::forward<oatpp::async::Action>(nextAction));
  }

  return std::forward<oatpp::async::Action>(nextAction);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// InflateStreamReader

InflateStreamReader::InflateStreamReader(data::stream::ReadCallback* sourceCallback, v_buff_size chunkBufferSize, bool useGzip)
  : m_sourceCallback(sourceCallback)
  , m_chunkBufferSize(chunkBufferSize)
  , m_useGzip(useGzip)
  , m_state(STATE_WAITING)
{}

bool InflateStreamReader::finish() {
  while(m_zStream.avail_out > 0) {
    auto res = inflate(&m_zStream, Z_FINISH);
    if(res == Z_STREAM_END) {
      return true;
    } else if(res == Z_OK) {
      continue;
    } else if(res == Z_BUF_ERROR) {
      break;
    } else {
      return true; // unknown error. just finish silently.
    }
  }
  return false;
}

data::v_io_size InflateStreamReader::read(void *buffer, v_buff_size count) {

  if(m_state == STATE_WAITING) {

    m_zStream.zalloc = Z_NULL;
    m_zStream.zfree = Z_NULL;
    m_zStream.opaque = Z_NULL;

    m_zStream.next_in = nullptr;
    m_zStream.avail_in = 0;

    v_int32 res;

    if(m_useGzip) {
      res = inflateInit2(&m_zStream, 15 | 16);
    } else {
      res = inflateInit(&m_zStream);
    }

    if(res != Z_OK) {
      return 0;
    }

    m_state = STATE_READING;

  } else if(m_state == STATE_DONE) {
    return 0;
  }

  v_buff_size chunkSize = m_chunkBufferSize;
  std::unique_ptr<v_char8[]> chunk(new v_char8[chunkSize]);

  m_zStream.next_out = (Bytef *) buffer;
  m_zStream.avail_out = (uInt) count;


  if(m_state == STATE_FINISHING) {

    if(finish()) {
      m_state = STATE_DONE;
    }

  } else {

    while (m_zStream.avail_out > 0) {

      if(m_zStream.avail_in == 0) {

        chunkSize = m_sourceCallback->read(chunk.get(), chunkSize);

        m_zStream.next_in = (Bytef *) chunk.get();
        m_zStream.avail_in = (uInt) chunkSize;

      }

      if (chunkSize > 0) {

        auto res = inflate(&m_zStream, Z_NO_FLUSH);

        if(res != Z_OK) {
          if(res == Z_DATA_ERROR) {
            return 0;
          }
          break;
        }

      } else if (chunkSize == data::IOError::RETRY_READ || chunkSize == data::IOError::WAIT_RETRY_READ ||
                 chunkSize == data::IOError::RETRY_WRITE || chunkSize == data::IOError::WAIT_RETRY_WRITE) {
        chunkSize = m_chunkBufferSize;
      } else if (chunkSize == 0) {

        m_state = STATE_FINISHING;
        if(finish()) {
          m_state = STATE_DONE;
        }
        break;

      } else if (m_zStream.avail_out < count) {
        break;
      } else if (m_zStream.avail_out == count) {
        return chunkSize;
      }

    }

  }

  return count - m_zStream.avail_out;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AsyncInflateStreamReader

AsyncInflateStreamReader::AsyncInflateStreamReader(const std::shared_ptr<data::stream::AsyncReadCallback>& sourceCallback,
                                                   v_buff_size chunkBufferSize,
                                                   bool useGzip)
  : m_sourceCallback(sourceCallback)
  , m_chunkBufferSize(chunkBufferSize)
  , m_useGzip(useGzip)
  , m_state(STATE_WAITING)
{}


oatpp::async::Action AsyncInflateStreamReader::readAsyncInline(AsyncInlineReadData& inlineData, oatpp::async::Action&& nextAction) {

  class ReadAsyncCoroutine : public async::Coroutine<ReadAsyncCoroutine> {
  private:
    AsyncInflateStreamReader* m_this;
    AsyncInlineReadData* m_outData;
  private:
    std::unique_ptr<v_char8[]> m_chunk;
    AsyncInlineReadData m_inData;
  public:

    ReadAsyncCoroutine(AsyncInflateStreamReader* _this, AsyncInlineReadData* outData)
      : m_this(_this)
      , m_outData(outData)
    {}

    bool processFinish() {
      while(m_this->m_zStream.avail_out > 0) {
        auto res = inflate(&m_this->m_zStream, Z_FINISH);
        if(res == Z_STREAM_END) {
          return true;
        } else if(res == Z_OK) {
          continue;
        } else if(res == Z_BUF_ERROR) {
          break;
        } else {
          return true; // unknown error. just finish silently.
        }
      }
      return false;
    }

    Action act() override {

      if(m_this->m_state == STATE_WAITING) {

        m_this->m_zStream.zalloc = Z_NULL;
        m_this->m_zStream.zfree = Z_NULL;
        m_this->m_zStream.opaque = Z_NULL;

        m_this->m_zStream.next_in = nullptr;
        m_this->m_zStream.avail_in = 0;

        v_int32 res;

        if(m_this->m_useGzip) {
          res = inflateInit2(&m_this->m_zStream, 15 | 16);
        } else {
          res = inflateInit(&m_this->m_zStream);
        }

        if(res != Z_OK) {
          return finish();
        }

        m_this->m_state = STATE_READING;

      } else if(m_this->m_state == STATE_DONE) {
        return finish();
      }

      m_this->m_zStream.next_out = (Bytef *) m_outData->currBufferPtr;
      m_this->m_zStream.avail_out = (uInt) m_outData->bytesLeft;

      if(m_this->m_state == STATE_FINISHING) {

        if(processFinish()) {
          m_this->m_state = STATE_DONE;
        }

        m_outData->inc(m_outData->bytesLeft - m_this->m_zStream.avail_out);
        return finish();

      }

      m_chunk.reset(new v_char8[m_this->m_chunkBufferSize]);
      return yieldTo(&ReadAsyncCoroutine::processNext);

    }

    Action processNext() {

      if(m_this->m_zStream.avail_out == 0) {
        m_outData->inc(m_outData->bytesLeft - m_this->m_zStream.avail_out);
        return finish();
      }

      if(m_this->m_zStream.avail_in == 0) {
        m_inData.set(m_chunk.get(), m_this->m_chunkBufferSize);
        return yieldTo(&ReadAsyncCoroutine::readChunkData);
      }

      auto res = inflate(&m_this->m_zStream, Z_NO_FLUSH);

      if(res != Z_OK) {
        m_outData->inc(m_outData->bytesLeft - m_this->m_zStream.avail_out);
        return finish();
      }

      return repeat();

    }

    Action readChunkData() {
      return m_this->m_sourceCallback->readAsyncInline(m_inData, yieldTo(&ReadAsyncCoroutine::setNewInData));
    }

    Action setNewInData() {

      m_this->m_zStream.next_in = (Bytef *) m_chunk.get();
      m_this->m_zStream.avail_in = (uInt) m_this->m_chunkBufferSize - m_inData.bytesLeft;

      if(m_this->m_zStream.avail_in > 0) {
        return yieldTo(&ReadAsyncCoroutine::processNext);
      }

      m_this->m_state = STATE_FINISHING;

      if(processFinish()) {
        m_this->m_state = STATE_DONE;
      }

      m_outData->inc(m_outData->bytesLeft - m_this->m_zStream.avail_out);
      return finish();

    }

  };

  if(m_state != STATE_DONE) {
    return ReadAsyncCoroutine::start(this, &inlineData).next(std::forward<oatpp::async::Action>(nextAction));
  }

  return std::forward<oatpp::async::Action>(nextAction);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Convenience Functions

oatpp::String deflate(const oatpp::String& text, v_buff_size chunkSize, v_buff_size bufferSize, bool useGzip) {

  oatpp::data::stream::BufferOutputStream resultBuffer;

  oatpp::data::stream::BufferInputStream textStream(text);
  oatpp::data::stream::DefaultReadCallback streamReader(&textStream);
  oatpp::zlib::DeflateStreamReader reader(&streamReader, Z_DEFAULT_COMPRESSION, chunkSize, useGzip);

  oatpp::String buffer(bufferSize);
  oatpp::data::v_io_size res = 1;
  while (res > 0) {
    res = reader.read(buffer->getData(), buffer->getSize());
    if (res > 0) {
      resultBuffer.write(buffer->getData(), res);
    }
  }

  return resultBuffer.toString();

}

oatpp::String inflate(const oatpp::String& text, v_buff_size chunkSize, v_buff_size bufferSize, bool useGzip) {

  oatpp::data::stream::BufferOutputStream resultBuffer;

  oatpp::data::stream::BufferInputStream textStream(text);
  oatpp::data::stream::DefaultReadCallback streamReader(&textStream);
  oatpp::zlib::InflateStreamReader reader(&streamReader, chunkSize, useGzip);

  oatpp::String buffer(bufferSize);
  oatpp::data::v_io_size res = 1;
  while (res > 0) {
    res = reader.read(buffer->getData(), buffer->getSize());
    if (res > 0) {
      resultBuffer.write(buffer->getData(), res);
    }
  }

  return resultBuffer.toString();

}

}}
