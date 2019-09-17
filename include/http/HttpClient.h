/*
 * This file is part of ePipe
 * Copyright (C) 2019, Logical Clocks AB. All rights reserved
 *
 * ePipe is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ePipe is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef EPIPE_HTTPCLIENT_H
#define EPIPE_HTTPCLIENT_H

#include "common.h"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

struct HttpResponse{
  bool mSuccess;
  unsigned int mCode;
  std::string mResponse;
};

class HttpClient{
public:
  HttpClient(std::string addr){
    try {
      auto const i = addr.find(":");
      auto const ip = addr.substr(0, i);
      auto const port = static_cast<unsigned short>(std::atoi(
          addr.substr(i + 1, addr.length()).c_str()));
      mEndpoint = {net::ip::make_address(ip), port};
    } catch(const boost::system::system_error & ex){
      LOG_FATAL("error in http address format [" << addr << "]"
          << " only ips allowed : " <<  ex.code() << " " << ex.what());
    }
  }

  HttpResponse get(std::string target){
    return request(http::verb::get, target);
  }

  HttpResponse post(std::string target, std::string data){
    return request(http::verb::post, target, data);
  }

  HttpResponse delete_(std::string target){
    return request(http::verb::delete_, target);
  }
private:
  tcp::endpoint mEndpoint;

  HttpResponse request(http::verb verb, std::string
  target){
    return request(verb, target, "");
  }

  HttpResponse request(http::verb verb, std::string
  target, std::string data){

    std::string responseBody;
    bool succeed = true;
    unsigned int code = 0;

    try
    {
      net::io_context ioc;
      beast::tcp_stream stream(ioc);

      stream.connect(mEndpoint);

      http::request<http::string_body> req{verb, target, 11};
      req.set(http::field::host, mEndpoint.address().to_string());
      req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
      req.set(http::field::content_type, "application/json");

      if(data.length() != 0){
        req.body() = data;
        req.set(http::field::content_length, data.length());
        req.prepare_payload();
      }

      http::write(stream, req);

      beast::flat_buffer buffer;

      http::response<http::dynamic_body> response;

      http::read(stream, buffer, response);

      responseBody = beast::buffers_to_string(response.body().data());
      code = response.result_int();

      beast::error_code ec;
      stream.socket().shutdown(tcp::socket::shutdown_both, ec);

      if(ec && ec != beast::errc::not_connected){
        LOG_ERROR("Error in http connection with error code "<< ec);
        succeed = false;
      }
    }catch(std::exception const& e)
    {
      LOG_ERROR("Error in http connection : " << e.what());
      succeed = false;
    }

    return {succeed, code, responseBody};
  }

};
#endif //EPIPE_HTTPCLIENT_H
