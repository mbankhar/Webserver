#include "../include/Webserv.hpp"
#include "../include/Server.hpp"
#include "../include/Config.hpp"
#include <thread>

std::atomic<bool> keepRunning(true); // Global flag for server loop

void signalHandler(int signum) {
    std::cout << "\nInterrupt signal (" << signum << ") received.\n";
    keepRunning = false; // Stop the server loop
}

void setupSignalHandler() {
    struct sigaction action;
    action.sa_handler = signalHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(SIGINT, &action, nullptr) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}

void	InitServer(Config &config, int i)
{
	Server server(config.getServerBlocks(), i);
	setupSignalHandler();
	server.run();
	return;
}

int main(int argc, char** argv) {
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <config_file_path>\n";
		return 1;
	}

	// debug("Server running in DEBUG mode.");
	setupSignalHandler(); // Set up the signal handler

	try {
		Config config(argv[1]); // Load and validate configuration

		int ports = config.getServerBlocks().size();

		std::vector<std::thread> threads;

		for (int i = 0; i < ports; i++) {
			// Create a thread for each server
			threads.emplace_back(InitServer, std::ref(config), i);
		}

		// Wait for all threads to finish
		for (auto& thread : threads) {
			if (thread.joinable()) {
				thread.join();
			}
		}

		std::cout << "All servers shut down gracefully.\n";
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}

