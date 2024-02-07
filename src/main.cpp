#include <iostream>
#include <cstring>
#include <fstream>
#include "cxxopts.hpp"
#include "wave/file.h"

int wav_to_gtp(std::string input_file, std::string output_file)
{
	wave::File read_file;
	std::vector<float> content;
	wave::Error err = read_file.Open(input_file, wave::kIn);
	if (err) {
		std::cerr << "Something went wrong while opening input file." << std::endl;
		return 1;
	}
	err = read_file.Read(&content);
	if (err) {
		std::cout << "Something went wrong while reading input file." << std::endl;
		return 1;
	}
	std::cout << "Converting Galaksija WAV tape to GTP format." << std::endl;
	size_t numSamples = content.size();
	int cnt = 0;
	int8_t prevVal = 0;
	uint8_t bitcounter = 0;
	uint8_t data = 0x00;
	bool wait_a5 = true;

	int half = 0;

	std::vector<uint8_t> buffer;

	printf("Number of samples : %lu\n", numSamples);
	for (size_t i = 0; i < numSamples; i++)
	{
		double currentSample = content[i];
		//printf("%f\n",currentSample);
		int8_t val = 0;
		if (currentSample >  0.3) val = 1;
		if (currentSample < -0.3) val = -1;
		if (prevVal == val && i < (numSamples-1)) {
			cnt++;
		} else {
			if (prevVal==0) { 
				//printf("%d cnt:%d %02x\n",prevVal,cnt,data);
				if (cnt<50) {
					// 1
					if (half) {
						data = (data >> 1) | 0x80;
						half = 0;
					} else {
						half = 1;
					} 
				} else if (cnt < 200 ) {
					// 0
					data = (data >> 1) | 0;
				} else {
					// 1
					if (half) {
						data = (data >> 1) | 0x80;
					} else {
						data = (data >> 1) | 0;
					}
					half = 0;
					if (!wait_a5) {
						buffer.push_back(data);
					}
				}

				if (wait_a5) {
					if (data==0xa5) {
						wait_a5 = false;
						buffer.push_back(data);
					}
				}
			}

			cnt = 0;
		}
		prevVal = val;
	}
	if (wait_a5) {
		printf("Not able to find sync signal !\n");
		return -1;
	}
	// add garbage byte
	buffer.push_back(data);
	printf("Number of data bytes found: %lu\n", buffer.size());
	if (buffer.size() < 5) {
		printf("No BASIC data found !\n");
		return -1;
	}
	if (buffer[1] != 0x36 || buffer[2] != 0x2c) {
		printf("BASIC START not found !\n");
		return -1;
	}
	uint16_t basic_start = buffer[1] | buffer[2]<<8;
	uint16_t basic_end   = buffer[3] | buffer[4]<<8;
	printf("Found BASIC START:%04x\n", basic_start);
	printf("Found BASIC END  :%04x\n", basic_end);
	// basic size 
	//  + 0xA5 byte +  4 bytes for basic start/end +  checksum + garbage byte
	uint16_t block_size = (basic_end - basic_start) + 1 + 4 + 2;
	printf("Found SIZE :%04x\n", block_size);
	if (buffer.size() < block_size) {
		printf("There is not enough data found !\n");
		return -1;
	}
	uint8_t sum = 0;
	for(uint16_t i=0;i<block_size-1;i++) {
		sum += buffer[i];
	}
	printf("Checksum is %s\n", (sum == 0xff) ? "OK" : "WRONG");

	FILE *fp = fopen(output_file.c_str(), "wb");
	fputc(0, fp);
	fputc((block_size) & 0xff, fp);
	fputc((block_size >> 8) & 0xff, fp);
	fputc(0, fp);
	fputc(0, fp);
	for(uint16_t i=0;i<block_size;i++)
		fputc(buffer[i],fp);
	fclose(fp);
	return 0;
}

int main(int argc, const char* argv[])
{
	std::string input_file;
	std::string output_file;
	try
	{
		std::string name = argv[0];
		cxxopts::Options options(name.c_str(), name + " - Galaksija cassette tape tool\nCopyright (c) 2024 Miodrag Milanovic");
		options
			.positional_help("[optional args]")
			.show_positional_help();

		options
			.allow_unrecognised_options()
			.add_options()
			("i,input", "Input", cxxopts::value<std::string>())
			("o,output", "Output file", cxxopts::value<std::string>())
			("gtp", "Convert WAV to GTP", cxxopts::value<bool>())
			("help", "Print help")
		;

		options.parse_positional({"input", "output"});

		auto result = options.parse(argc, argv);

		if (result.count("help") || result.arguments().empty())
		{
			std::cout << options.help() << std::endl;
			exit(0);
		}

		if (result.count("input"))
		{
			input_file = result["input"].as<std::string>();
		} else {
			std::cerr << "Input file is mandatory." << std::endl;
			return 1;
		}

		if (result.count("output"))
		{
			output_file = result["output"].as<std::string>();
		} else {
			std::cerr << "Output file is mandatory." << std::endl;	
			return 1;
		}

		if (!result.count("wav") && !result.count("gtp"))
		{
			std::cerr << "Target format is mandatory." << std::endl;	
			return 1;
		}
		if (result.count("gtp")) {
			return wav_to_gtp(input_file, output_file);
		}

	}
	catch (const cxxopts::exceptions::exception& e)
	{
		std::cerr << "Error parsing options: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
