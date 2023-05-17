#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include <sw/redis++/redis++.h>

using json = nlohmann::json;
using boost::asio::ip::udp;
using namespace sw::redis;

// UdpServer class
class UdpServer
{
public:
	UdpServer(boost::asio::io_context& io_context,unsigned short port);
private:
	void start_receive();
	void handle_receive(std::size_t bytes_recvd);
	void handle_send(boost::shared_ptr<std::string> , const boost::system::error_code& , std::size_t );
	
	udp::socket socket_;
	udp::endpoint remote_endpoint_;
	std::array<uint8_t, 1024> recv_buffer_;
	std::unique_ptr<Redis> redis_;
};

UdpServer::UdpServer(boost::asio::io_context& io_context, unsigned short port)
	: socket_(io_context, udp::endpoint(udp::v4(), port))
{
	try {
		// 레디스 연결 초기화
		redis_ = std::unique_ptr<Redis>(new Redis("tcp://127.0.0.1:6379"));
		//test
		//redis_->set("hello","redis");
		std::cout << "Successfully connected to Redis." << std::endl;
	}
	catch (const Error &err) {
		std::cerr << "Failed to connect to Redis: " << err.what() << std::endl;
	}

	start_receive();
}

void UdpServer::start_receive()
{
	std::cerr << "Start receive()." << std::endl;

	socket_.async_receive_from(
		boost::asio::buffer(recv_buffer_), remote_endpoint_,
		[this](boost::system::error_code ec, std::size_t bytes_recvd)
		{
			if (!ec && bytes_recvd > 0)
			{
				handle_receive(bytes_recvd);
				start_receive();
		}
			else
			{
				std::cerr << "Error receiving data: " << ec.message() << std::endl;
				start_receive();

		}
	});
}

void UdpServer::handle_receive(std::size_t bytes_recvd)
{
	// 데이터 문자열 변환
	std::string data(recv_buffer_.begin(), recv_buffer_.begin() + bytes_recvd);
	// json 파싱
	json jsonData = json::parse(data);

	std::cout << "Location: " << jsonData["Location"] << std::endl;
	std::cout << "Rotation: " << jsonData["Rotation"] << std::endl;
	std::cout << "Velocity: " << jsonData["Velocity"] << std::endl;
}

