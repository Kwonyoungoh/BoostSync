#pragma once

#include <iostream>
#include <boost/asio.hpp>
#include <hiredis/hiredis.h>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;
using boost::asio::ip::udp;
