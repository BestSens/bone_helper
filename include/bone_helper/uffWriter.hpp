#pragma once

#include <string>
#include <vector>

#include "bone_helper/fsHelper.hpp"

namespace bestsens {
	class UffFile {
	public:
		struct dataset {
			time_t date{};
			std::string position_name;
			int position{0};
			double dt{0};
			std::string y_unit;
			size_t sample_count{0};
		};

		explicit UffFile(const std::string& filename) : file{filename, std::ios::out} {};

		auto updateFilename(const std::string& filename) -> void;
		auto writeHeader(const dataset& ds) -> void;

		auto appendData(const std::vector<float>& data) -> void;

		auto endFile() -> void;

		auto clearFile() -> void;

	private:
		system_helper::fs::OutputFile file;
		size_t samples_written{0};
	};
}  // namespace bestsens
