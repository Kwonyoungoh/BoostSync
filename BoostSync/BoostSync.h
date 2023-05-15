#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;
using boost::asio::ip::tcp;

