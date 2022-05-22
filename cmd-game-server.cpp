#include "../util/util.hpp"
#include <iostream>
#include <fstream>
#include <conio.h>
#include <sstream>
#include <filesystem>

#pragma comment(lib,"ws2_32.lib")

int main(int argc, char** argv)
{
		util::InternetServerTCP inetS{ 100, std::cout };
		std::string world_name = "My World";
		if (argc > 1)
			world_name = argv[1];
		if(!std::filesystem::exists(std::filesystem::current_path().string() + "\\" + world_name))
		{
			std::cout << "[MAIN THREAD] " << std::filesystem::current_path().string() + "\\" + world_name << " doesn't exist! Error message : " << util::ErrorCodeToString(ERROR_PATH_NOT_FOUND);
			return ERROR_PATH_NOT_FOUND;
		}
		do
		{
			std::cout << "Checking for connection requests!\n";
			SOCKET client = inetS.AcceptConnectionFromClientASYNC();
			std::thread newConnection
			{
				[&]()
				{
					// Actual message
					char rec[5000] = { "" };
					// Return Value of recv
					int ec{};
					// The client socket
					SOCKET this_Client = std::move(client);
					// Message to send back
					const char* send_msg = "";
					std::cout << "Accepted connection request\n";
					do {
						std::cout << "[THREAD " << std::this_thread::get_id() << "] Checking for messages from client!\n";
						ec = recv(this_Client, rec, 5000, NULL);
						if (ec == SOCKET_ERROR)
						{
							std::cout << "An error has occured while calling recv() Error : " << ::util::ErrorCodeToString(WSAGetLastError());
						}
						std::cout << "[THREAD " << std::this_thread::get_id() << "] Recived message!\n"
								  << "[THREAD " << std::this_thread::get_id() << "] Extracting message from message buffer\n";
						std::cout
							<< "[THREAD " << std::this_thread::get_id() << "] Extracted message!\n" << "[THREAD " << std::this_thread::get_id() << "] Message is \n"
							<< rec;
						if (_stricmp(rec, "GAME_PROTOCOL_GET 0x00\r\n\r\n") == 0 || _stricmp(rec, "GAME_PROTOCOL_GET 0x00") == 0)
						{
							send_msg = world_name.c_str();
							send(this_Client, send_msg, world_name.length(), NULL);
						}
						else if (_stricmp(rec, "GAME_PROTOCOL_GET 0x01\r\n\r\n") == 0 || _stricmp(rec, "GAME_PROTOCOL_GET 0x01") == 0)
						{
							std::string ln = "";
							std::ifstream ifstr;
							std::ifstream ifstr2;
							if (std::filesystem::exists(std::filesystem::current_path().string() + "\\" + world_name))
							{
								ifstr.open(std::filesystem::current_path().string() + "\\" + world_name + "\\savegame.cmdgamesave");
								ifstr2.open(std::filesystem::current_path().string() + "\\" + world_name + "\\savegame.cmdgamesave");
							}
							else
							{
								std::cout << "World " << (std::filesystem::current_path().string() + "\\" + world_name) << " doesn't exist!";
								break;
							}

							ifstr2.unsetf(std::ios_base::skipws);

							int64_t line_count = std::count(
								std::istream_iterator<char>(ifstr2),
								std::istream_iterator<char>(),
								'\n');
							std::string ln_Count_STR = std::string(std::string("[") + std::to_string(line_count) + std::string("]"));
							send(this_Client, ln_Count_STR.c_str(), ln_Count_STR.length(), NULL);
							std::cout << "\n[THREAD " << std::this_thread::get_id() << "] Sending " << line_count << " lines of the savegame to client\n";
							while (std::getline(ifstr, ln))
							{
								ln = ln.append("\n");
								send(this_Client, ln.c_str(), ln.length() + 1, NULL);
							}
						}
						else if (_stricmp(rec, "GAME_PROTOCOL_GET 0x02\r\n\r\n") == 0 || _stricmp(rec, "GAME_PROTOCOL_GET 0x02") == 0)
						{
							std::string ln = "";
							std::ifstream ifstr;
							std::ifstream ifstr2;
							if (std::filesystem::exists(std::filesystem::current_path().string() + "\\" + world_name))
							{
								ifstr.open(std::filesystem::current_path().string() + "\\" + world_name + "\\chat.txt");
								ifstr2.open(std::filesystem::current_path().string() + "\\" + world_name + "\\chat.txt");
							}
							else break;

							ifstr2.unsetf(std::ios_base::skipws);

							int64_t line_count = std::count(
								std::istream_iterator<char>(ifstr2),
								std::istream_iterator<char>(),
								'\n');
							std::string ln_Count_STR = std::string(std::string("[") + std::to_string(line_count) + std::string("]"));
							send(this_Client, ln_Count_STR.c_str(), ln_Count_STR.length(), NULL);
							std::cout << "\n[THREAD " << std::this_thread::get_id() << "] Sending " << line_count << " lines of the chat to client\n";
							while (std::getline(ifstr, ln))
							{
								ln = ln.append("\n");
								send(this_Client, ln.c_str(), ln.length() + 1, NULL);
							}
						}
						else if (_stricmp(rec, "GAME_PROTOCOL_WRITE 0x01\r\n\r\n") == 0 || _stricmp(rec, "GAME_PROTOCOL_WRITE 0x01") == 0)
						{
							std::cout << "[THREAD " << std::this_thread::get_id() << "] Writing to savegame!\n";
							std::string svgame = world_name;
							svgame.append("\\savegame.cmdgamesave");
							std::ofstream of{ svgame };
							char data[5000] = "";
							recv(this_Client, data, 5000, NULL);
							std::cout << "[THREAD " << std::this_thread::get_id() << "]"
								" Recived something to write! Data is\n"
							   << data;
							of << data;
							of.close();
						}
						else if (strlen(rec) == 15 || _stricmp(rec, "state is false"))
						{
						}
						else
						{
							std::cout << "[THREAD " << std::this_thread::get_id() << "]\n GAME_PROTOCOL_ERR 0x00 Invalid Request!\n";
							send(this_Client, "GAME_PROTOCOL_ERR 0x00 Invalid Request\n", 40, NULL);
							inetS.CloseSocket();
							break;
						}
					} while (ec > 0);
					shutdown(this_Client, (int)util::shutdown_how::both);
					closesocket(this_Client);
				}
			};
			newConnection.detach();
		} while (true);
		if (!inetS.is_good()) {
			inetS.CloseSocket();
			return 1;
		}
	std::cout << "\nPress any key to continue...";
	(void)_getch();
}