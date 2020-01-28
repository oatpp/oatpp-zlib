# oatpp-zlib [![Build Status](https://dev.azure.com/lganzzzo/lganzzzo/_apis/build/status/oatpp.oatpp-zlib?branchName=master)](https://dev.azure.com/lganzzzo/lganzzzo/_build/latest?definitionId=23&branchName=master)

Module `oatpp-zlib` provides functionality for compressing/decompressing content with `deflate` and `gzip`.  
Supports both "Simple" and "Async" oatpp APIs.

See more:
- [Oat++ Website](https://oatpp.io/)
- [Oat++ Github Repository](https://github.com/oatpp/oatpp)

## How To Build

### Requires

- ZLib installed.

#### Install ZLib

sudo apt-get install zlib1g-dev

### Install oatpp-zlib

Clone this repository. In the root of the repository run:

```bash
mkdir build && cd build
cmake ..
make install
```

## APIs

### Automatically Compress Served Content

Configure `server::ConnectionHandler` in `AppComponent.hpp`.

```cpp
#include "oatpp-zlib/EncoderProvider.hpp"

...


OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::server::ConnectionHandler>, serverConnectionHandler)([] {

  OATPP_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, router); // get Router component

  /* Create HttpProcessor::Components */
  auto components = std::make_shared<oatpp::web::server::HttpProcessor::Components>(router);

  /* Add content encoders */
  auto encoders = std::make_shared<oatpp::web::protocol::http::encoding::ProviderCollection>();

  encoders->add(std::make_shared<oatpp::zlib::DeflateEncoderProvider>());
  encoders->add(std::make_shared<oatpp::zlib::GzipEncoderProvider>());

  /* Set content encoders */
  components->contentEncodingProviders = encoders;

  /* return HttpConnectionHandler */
  return std::make_shared<oatpp::web::server::HttpConnectionHandler>(components);
  
}());

...
```

Now served content will be automatically compressed and streamed to the client if the client sets `Accept-Encoding` header appropriately.

### Automatically Decompress Uploaded Content

Configure `server::ConnectionHandler` in `AppComponent.hpp`.

```cpp
#include "oatpp-zlib/EncoderProvider.hpp"
#include "oatpp/web/protocol/http/incoming/SimpleBodyDecoder.hpp"

...

OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::server::ConnectionHandler>, serverConnectionHandler)([] {

  OATPP_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, router); // get Router component

  /* Create HttpProcessor::Components */
  auto components = std::make_shared<oatpp::web::server::HttpProcessor::Components>(router);

  /* Add content decoders */
  auto decoders = std::make_shared<oatpp::web::protocol::http::encoding::ProviderCollection>();

  decoders->add(std::make_shared<oatpp::zlib::DeflateDecoderProvider>());
  decoders->add(std::make_shared<oatpp::zlib::GzipDecoderProvider>());

  /* Set Body Decoder */
  components->bodyDecoder = std::make_shared<oatpp::web::protocol::http::incoming::SimpleBodyDecoder>(decoders);

  /* return HttpConnectionHandler */
  return std::make_shared<oatpp::web::server::HttpConnectionHandler>(components);
  
}());

...
```

Now uploaded content will be automatically decompressed if the client sets `Content-Encoding` header properly.

