#pragma once

#include <iostream>
#include <cstdint>
#include <string>
#include <vector>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include <sw/redis++/redis++.h>

// 플래그
enum class conn_flags : uint8_t {
	CONNECT_FLAG = 0X01,
	DISCONNECT_FLAG = 0X02,
	DATA_FLAG = 0X03,
	CHANG_CHUNK_FLAG = 0X04
};

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
	// 접속 클라이언트 컨테이너
	std::unordered_map<std::string, std::unordered_map<std::string, udp::endpoint>> chunk_clients;
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
	uint8_t flag = static_cast<uint8_t>(recv_buffer_[0]);

	// Flag 제거
	std::string data(recv_buffer_.begin() + 1, recv_buffer_.begin() + bytes_recvd);
	json jsonData = json::parse(data);
	std::string endpoint_key = remote_endpoint_.address().to_string() + ":" + std::to_string(remote_endpoint_.port());

	switch (static_cast<conn_flags>(flag)) {
	case conn_flags::CONNECT_FLAG: {

		std::string chunkInfo = jsonData["Chunk"];

		chunk_clients[chunkInfo][endpoint_key] = remote_endpoint_;

		std::cout << "Connection established." << std::endl;
	}
	break;

	case conn_flags::DATA_FLAG:
	{
		
		std::cout << "Location: " << jsonData["Location"] << std::endl;
		std::cout << "Rotation: " << jsonData["Rotation"] << std::endl;
		std::cout << "Velocity: " << jsonData["Velocity"] << std::endl;
	}
	break;

	case conn_flags::CHANG_CHUNK_FLAG:
	{

		std::string prevChunk = jsonData["PrevChunk"];
		std::string chunkInfo = jsonData["Chunk"];
	
		// 이전 청크에서 endpointkey 삭제
		chunk_clients[prevChunk].erase(endpoint_key);

		// 이전 청크에 클라이언트가 없으면 삭제
		if (chunk_clients[prevChunk].empty()) {
			chunk_clients.erase(prevChunk);
		}

		// 새로운 청크에 클라이언트 추가
		chunk_clients[chunkInfo][endpoint_key] = remote_endpoint_;

		std::cout << "Chunk changed." << std::endl;

	}
	break;

	case conn_flags::DISCONNECT_FLAG:
	{
		std::string chunkInfo = jsonData["Chunk"];

		// 청크에서 클라이언트 삭제
		chunk_clients[chunkInfo].erase(endpoint_key);
		// 청크에 클라이언트가 없으면 삭제
		if (chunk_clients[chunkInfo].empty()) {
			chunk_clients.erase(chunkInfo);
		}

		std::cout << "Connection closed." << std::endl;
	}
	break;

	default:
		std::cerr << "Unknown flag received." << std::endl;
		break;
	}
}