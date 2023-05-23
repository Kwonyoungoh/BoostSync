#pragma once

#include <iostream>
#include <cstdint>
#include <string>
#include <vector>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <nlohmann/json.hpp>
#include <sw/redis++/redis++.h>

// 플래그
enum class conn_flags : uint8_t {
	CONNECT_FLAG = 0X01,
	DISCONNECT_FLAG = 0X02,
	DATA_FLAG = 0X03,
	CHANGE_CHUNK_FLAG = 0X04
};

using json = nlohmann::json;
using boost::asio::ip::udp;
using namespace sw::redis;

// UdpServer class
class UdpServer
{
public:
	UdpServer(boost::asio::io_context& io_context, unsigned short port);

	void start_receive();
	void handle_receive(std::size_t bytes_recvd);

	void sub_channel(const std::string& channel);
	void pub_msg(const std::string& channel, const std::string& msg);
	void unsub_channel(const std::string& channel);
	void handle_msg(const std::string& channel, const std::string& msg);

	std::string proc_send_json(const json& recv_json, const std::string& id);

	std::unique_ptr<Redis> redis_;
	std::unique_ptr<Subscriber> sub_;

private:
	udp::socket socket_;
	udp::endpoint remote_endpoint_;
	std::array<uint8_t, 1024> recv_buffer_;

	// 구독자 뮤텍스
	std::mutex sub_mutex_;

	// 접속 클라이언트 컨테이너
	std::unordered_map<std::string, std::unordered_map<std::string, udp::endpoint>> chunk_clients;
};

UdpServer::UdpServer(boost::asio::io_context& io_context, unsigned short port)
	: socket_(io_context, udp::endpoint(udp::v4(), port))
{
	try {

		// Connection Option
		ConnectionOptions opt_;
		opt_.host = "127.0.0.1";
		opt_.port = 6379;
		// 레디스 연결 초기화
		redis_ = std::unique_ptr<Redis>(new Redis(opt_));
		//test
		//redis_->set("hello","redis2");

		// sub 객체 생성
		sub_ = std::unique_ptr<Subscriber>(new Subscriber(redis_->subscriber()));

		// 메세지 콜백
		sub_->on_message([this](std::string channel, std::string msg) {
			this->handle_msg(channel, msg);
			std::cout << "Received message: " << msg << " from channel: " << channel << std::endl;
			});

		// 메타 메세지 콜백
		sub_->on_meta([](Subscriber::MsgType type, OptionalString channel, long long num) {
			if (type == Subscriber::MsgType::SUBSCRIBE) {
				std::cout << "Successfully subscribe channel: " << *channel << std::endl;
			}
			else if (type == Subscriber::MsgType::UNSUBSCRIBE ) {
				std::cout << "Successfully unsubscribe channel :" << *channel << std::endl;
			}
			});

		std::string channel = "test";

		sub_->subscribe(channel);

		std::cout << "Successfully connected to Redis." << std::endl;
	}
	catch (const Error& err) {
		std::cerr << "Failed to connect to Redis: " << err.what() << std::endl;
	}

	//boost::thread comsume_t([this] {
	//	while (true) {
	//		try {
	//			//std::lock_guard<std::mutex> lock(sub_mutex_);

	//			sub_->consume();
	//			std::cout << "consume() message." << std::endl;
	//		}
	//		catch (const TimeoutError& timeout_err) {
	//		}
	//		catch (const Error& err) {
	//			std::cerr << "comsume()  Error: " << err.what() << std::endl;
	//		}
	//	}
	//	});
	//comsume_t.detach();

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
	std::string chunkInfo = jsonData["Chunk"];

	switch (static_cast<conn_flags>(flag)) {
	case conn_flags::CONNECT_FLAG: {

		sub_channel(chunkInfo);

		chunk_clients[chunkInfo][endpoint_key] = remote_endpoint_;

		std::string send_str = proc_send_json(jsonData, endpoint_key);
		pub_msg(chunkInfo, send_str);

		std::cout << "Connection established." << std::endl;
	}
	break;

	case conn_flags::DATA_FLAG:
	{
		std::string send_str = proc_send_json(jsonData, endpoint_key);
		pub_msg(chunkInfo, send_str);
	}
	break;

	case conn_flags::CHANGE_CHUNK_FLAG:
	{
		std::string prevChunk = jsonData["PrevChunk"];

		// 이전 청크에서 endpoint key 삭제
		chunk_clients[prevChunk].erase(endpoint_key);
		// 이전 청크에 클라이언트가 없으면 삭제
		if (chunk_clients[prevChunk].empty()) {
			chunk_clients.erase(prevChunk);
			// 구독 취소
			unsub_channel(prevChunk);
		}

		sub_channel(chunkInfo);
		// 새로운 청크에 클라이언트 추가
		chunk_clients[chunkInfo][endpoint_key] = remote_endpoint_;

		std::string send_str = proc_send_json(jsonData, endpoint_key);
		pub_msg(chunkInfo, send_str);

	}
	break;

	case conn_flags::DISCONNECT_FLAG:
	{
		// 청크에서 클라이언트 삭제
		chunk_clients[chunkInfo].erase(endpoint_key);
		// 청크에 클라이언트가 없으면 삭제
		if (chunk_clients[chunkInfo].empty()) {
			chunk_clients.erase(chunkInfo);
			// 구독 취소
			unsub_channel(chunkInfo);
		}

		std::cout << "Connection closed." << std::endl;

		try {
			std::lock_guard<std::mutex> lock(sub_mutex_);
			sub_->consume();
		}
		catch (const Error& err) {
			std::cerr << "consume() Error : " << err.what() << std::endl;
		}
	}
	break;

	default:
		std::cerr << "Unknown flag received." << std::endl;
		break;
	}
}

// Redis Pub/Sub
void UdpServer::sub_channel(const std::string& channel)
{
	// 중복청크 체크
	if (chunk_clients.find(channel) != chunk_clients.end()) {
		return;
	}

	try {
		std::lock_guard<std::mutex> lock(sub_mutex_);
		sub_->subscribe(channel);
		sub_->consume();
	}
	catch (const Error& err) {
		std::cerr << "subscribe() Error : " << err.what() << std::endl;
	}
}

void UdpServer::pub_msg(const std::string& channel, const std::string& msg)
{
	try {
		std::lock_guard<std::mutex> lock(sub_mutex_);
		redis_->publish(channel, msg);
		sub_->consume();
	}
	catch (const Error& err) {
		std::cerr << "publish() Error : " << err.what() << std::endl;
	}
}

void UdpServer::unsub_channel(const std::string& channel)
{
	try {
		std::lock_guard<std::mutex> lock(sub_mutex_);
		sub_->unsubscribe(channel);
		sub_->consume();
	}
	catch (const Error& err) {
		std::cerr << "unsubscribe() Error : " << err.what() << std::endl;
	}
}

void UdpServer::handle_msg(const std::string& channel, const std::string& msg)
{
	// 플래그 파싱
	uint8_t flag = static_cast<uint8_t>(msg[0]);
	std::string dataWithoutFlag = msg.substr(1);

	json sendJson = json::parse(dataWithoutFlag);
	std::string senderId = sendJson["id"];

	// 메시지 전송
	for (auto& client : chunk_clients[channel]) {
		// 중복체크
		//if (client.first == senderId) {
		//	continue;
		//}

		socket_.async_send_to(boost::asio::buffer(msg), client.second,
			[this](boost::system::error_code ec, std::size_t bytes_sent)
			{
				if (!ec && bytes_sent > 0)
				{
					// 
					std::cout << "msg sent." << std::endl;
				}
				else
				{
					std::cerr << "Error sending data: " << ec.message() << std::endl;
				}
			});
	}
}

std::string UdpServer::proc_send_json(const json& recv_json, const std::string& id)
{
	json send_json;

	send_json["id"] = id;
	send_json["Location"] = recv_json["Location"];
	send_json["Rotation"] = recv_json["Rotation"];
	send_json["Velocity"] = recv_json["Velocity"];

	// Add the DATA_FLAG
	uint8_t data_flag = static_cast<uint8_t>(conn_flags::DATA_FLAG);
	std::string send_str = std::string(1, data_flag) + send_json.dump();

	return send_str;
}