#include <iostream>
#include <cstring>
#include <fstream>
#include "cxxopts.hpp"
#include "wave/file.h"

int wav_to_gtp(std::string input_file, std::string output_file, double level, int channel)
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
		std::cerr << "Something went wrong while reading input file." << std::endl;
		return 1;
	}
	std::cout << "Converting Galaksija WAV tape to GTP format." << std::endl;
	size_t numSamples = content.size();
	int cnt = 0;
	uint8_t prevVal = 0;
	uint8_t bitcounter = 0;
	uint8_t data = 0x00;
	bool wait_a5 = true;

	int half = 0;

	std::vector<uint8_t> buffer;

	printf("Sample rate       : %lu\n", read_file.sample_rate());
	printf("Bits per sample   : %u\n", read_file.bits_per_sample());
	printf("Channels          : %u\n", read_file.channel_number());
	printf("Number of samples : %lu\n", numSamples);
	printf("\n");

	if ((channel+1) > read_file.channel_number()) {
		std::cerr << "Channel number does not exist." << std::endl;	
		return 1;
	}
	for (size_t i = channel; i < numSamples; i+=read_file.channel_number())
	{
		double currentSample = content[i];
		uint8_t val = (currentSample < -level) ? 1 : 0;
		//printf("%f  %d\n",currentSample, val);
		if (prevVal == val && i < (numSamples-1)) {
			cnt++;
		} else {
			if (prevVal==0) { 
				//printf("%d cnt:%d %02x\n",prevVal,cnt,data);
				if (cnt<100) {
					// 1
					if (half) {
						data = (data >> 1) | 0x80;						
					}
					half ^= 1; 
				} else if (cnt < 230 ) {
					// 0
					data = (data >> 1) | 0;
				} else {
					data = (data >> 1) | (half ? 0x80 : 0);
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
		printf("No data found !\n");
		return -1;
	}
	if (buffer[1] == 0x36 || buffer[2] == 0x2c) {
		printf("BASIC program found\n");
	}
	uint16_t program_start = buffer[1] | buffer[2]<<8;
	uint16_t program_end   = buffer[3] | buffer[4]<<8;
	printf("Found PROGRAM START:%04x\n", program_start);
	printf("Found PROGRAM END  :%04x\n", program_end);
	// basic size 
	//  + 0xA5 byte +  4 bytes for basic start/end +  checksum + garbage byte
	uint16_t block_size = (program_end - program_start) + 1 + 4 + 2;
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
		cxxopts::Options options(name.c_str(), name + " - Galaksija cassette tape tool v0.1\nCopyright (c) 2024 Miodrag Milanovic");
		options
			.positional_help("[optional args]")
			.show_positional_help();

		options
			.allow_unrecognised_options()
			.add_options()
			("i,input", "Input", cxxopts::value<std::string>())
			("o,output", "Output file", cxxopts::value<std::string>())
			("gtp", "Convert WAV to GTP", cxxopts::value<bool>())
			("level", "Egde level for cassette", cxxopts::value<double>()->default_value("0.1"))
			("ch", "Select channel if STEREO", cxxopts::value<int>()->default_value("1"))
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
		if (result["ch"].as<int>()<1 || result["ch"].as<int>()>2) {
			std::cerr << "Channel number can be only 1 or 2." << std::endl;	
			return 1;
		}
		if (result.count("gtp")) {
			return wav_to_gtp(input_file, output_file, result["level"].as<double>(), result["ch"].as<int>()-1);
		}

	}
	catch (const cxxopts::exceptions::exception& e)
	{
		std::cerr << "Error parsing options: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
