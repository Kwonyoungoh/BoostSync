#include "BoostSync.h"

int main()
{
	try {
		boost::asio::io_context io_context;

		UdpServer server(io_context, 12345);

		boost::thread_group worker_threads;
		for (std::size_t i = 0; i < boost::thread::hardware_concurrency(); ++i) {
			worker_threads.create_thread(
				[&io_context]() {
					io_context.run();
				}
			);
		}

		//boost::thread input_thread([&server]() {
		//	std::string input;
		//	while (true) {
		//		std::getline(std::cin, input);
		//		if (input == "quit") {
		//			break;
		//		}
		//	}
		//	
		//});
		//input_thread.detach();

		worker_threads.join_all();
	}
	catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}
