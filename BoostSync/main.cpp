#include "BoostSync.h"

using namespace std;

int main()
{
	try {
		boost::asio::io_context io_context;

		UdpServer server(io_context, 12345);

		io_context.run();
	}
	catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}
