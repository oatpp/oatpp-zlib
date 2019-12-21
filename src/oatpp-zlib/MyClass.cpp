//
// Created by Leonid on 2019-01-26.
//

#include "MyClass.hpp"
#include "Deflate.hpp"

#include "oatpp/core/data/stream/BufferStream.hpp"

#include "zlib.h"

namespace {

void* allocMemoryFunc(void *q, unsigned int n, unsigned int m) {
(void)q;
return ::new v_char8[n * m];
}

void freeMemoryFunc(void *q, void *p) {
  (void)q;
  delete [] (p_char8) p;
}

oatpp::String oatppDeflate(const oatpp::String& data) {

  oatpp::String outData(data->getSize());

  v_int32 buffSize = 10240;
  v_char8 buff[buffSize];

  z_stream zStream;


  zStream.zalloc = Z_NULL;
  zStream.zfree = Z_NULL;
  zStream.opaque = Z_NULL;

  auto res = deflateInit(&zStream, Z_DEFAULT_COMPRESSION);
  if(res != Z_OK) {
    OATPP_LOGE("Z", "deflateInit");
  }

  zStream.next_in = data->getData();
  zStream.avail_in = data->getSize();

  zStream.next_out = buff;
  zStream.avail_out = buffSize;

//  while(res != Z_STREAM_END) {
    res = deflate(&zStream, Z_FINISH);
    if (res != Z_STREAM_END) {
      OATPP_LOGE("Z", "deflate");
    }
//  }

  res = deflateEnd(&zStream);
  if(res != Z_OK) {
    OATPP_LOGE("Z", "deflateEnd");
  }


  /* Finish the stream, still forcing small buffers: */
//  for (;;) {
//    c_stream.avail_out = 1;
//    err = deflate(&c_stream, Z_FINISH);
//    if (err == Z_STREAM_END) break;
//    if(err != Z_OK) {
//      OATPP_LOGE("Z", "deflate2 error");
//    }
//  }

  return oatpp::String((const char*)buff, zStream.total_out, true);

}

oatpp::String deflate(const oatpp::String& text) {

  oatpp::data::stream::BufferOutputStream resultBuffer;

  oatpp::data::stream::BufferInputStream textStream(text);
  oatpp::data::stream::DefaultReadCallback streamReader(&textStream);
  oatpp::zlib::DeflateStreamReader reader(&streamReader);

  oatpp::data::v_io_size res = 1;
  while (res > 0) {
    v_buff_size buffSize = 16;
    v_char8 buff[buffSize];
    res = reader.read(buff, buffSize);
    if (res > 0) {
      resultBuffer.write(buff, res);
    }
  }

  return resultBuffer.toString();

}

oatpp::String inflate(const oatpp::String& text) {

  oatpp::data::stream::BufferOutputStream resultBuffer;

  oatpp::data::stream::BufferInputStream textStream(text);
  oatpp::data::stream::DefaultReadCallback streamReader(&textStream);
  oatpp::zlib::InflateStreamReader reader(&streamReader);

  oatpp::data::v_io_size res = 1;
  while (res > 0) {
    v_buff_size buffSize = 16;
    v_char8 buff[buffSize];
    res = reader.read(buff, buffSize);
    if (res > 0) {
      resultBuffer.write(buff, res);
    } else {
      OATPP_LOGD("aa", "?");
    }
  }

  return resultBuffer.toString();

}

}

void MyClass::doSomething() {

  oatpp::String text = "Kiev or Kyiv[a] (Ukrainian: Київ, romanized: Kyiv; Russian: Киев, romanized: Kiyev) is the capital and most populous city of Ukraine. It is in north-central Ukraine along the Dnieper River. Its population in July 2015 was 2,887,974[1] (though higher estimated numbers have been cited in the press),[12] making Kiev the 7th most populous city in Europe.[13]\n"
                       "\n"
                       "Kiev is an important industrial, scientific, educational and cultural center of Eastern Europe. It is home to many high-tech industries, higher education institutions, and historical landmarks. The city has an extensive system of public transport and infrastructure, including the Kiev Metro.\n"
                       "\n"
                       "The city's name is said to derive from the name of Kyi, one of its four legendary founders. During its history, Kiev, one of the oldest cities in Eastern Europe, passed through several stages of great prominence and relative obscurity. The city probably existed as a commercial centre as early as the 5th century. A Slavic settlement on the great trade route between Scandinavia and Constantinople, Kiev was a tributary of the Khazars,[14] until its capture by the Varangians (Vikings) in the mid-9th century. Under Varangian rule, the city became a capital of the Kievan Rus', the first East Slavic state. Completely destroyed during the Mongol invasions in 1240, the city lost most of its influence for the centuries to come. It was a provincial capital of marginal importance in the outskirts of the territories controlled by its powerful neighbours, first Lithuania, then Poland and Russia.[15]\n"
                       "\n"
                       "The city prospered again during the Russian Empire's Industrial Revolution in the late 19th century. In 1918, after the Ukrainian National Republic declared independence from Soviet Russia, Kiev became its capital. From 1921 onward Kiev was a city of the Ukrainian Soviet Socialist Republic, which was proclaimed by the Red Army, and, from 1934, Kiev was its capital. The city was almost completely ruined during World War II but quickly recovered in the postwar years, remaining the Soviet Union's third-largest city.\n"
                       "\n"
                       "Following the Soviet Union's collapse and Ukrainian independence in 1991, Kiev remained Ukraine's capital and experienced a steady influx of ethnic Ukrainian migrants from other regions of the country.[16] During the country's transformation to a market economy and electoral democracy, Kiev has continued to be Ukraine's largest and wealthiest city. Its armament-dependent industrial output fell after the Soviet collapse, adversely affecting science and technology, but new sectors of the economy such as services and finance facilitated Kiev's growth in salaries and investment, as well as providing continuous funding for the development of housing and urban infrastructure. Kiev emerged as the most pro-Western region of Ukraine; parties advocating tighter integration with the European Union dominate during elections.[17][18][19][20]";

  //auto result1 = oatppDeflate(text);

  auto result2 = deflate(text);
  auto result3 = inflate(result2);

  OATPP_LOGD("0", "size=%d, '%s'", text->getSize(), text->getData());
  OATPP_LOGD("---", "");
  OATPP_LOGD("2", "size=%d, '%s'", result2->getSize(), result2->getData());
  OATPP_LOGD("---", "");
  OATPP_LOGD("3", "size=%d, '%s'", result3->getSize(), result3->getData());

}
