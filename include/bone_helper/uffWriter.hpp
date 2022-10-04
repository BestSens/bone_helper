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
		};

		UffFile() = default;
		explicit UffFile(std::string filename) : filename(std::move(filename)){};

		auto setFilename(const std::string& filename) -> void;
		auto write() -> void;

		auto setParameters(const dataset& ds) -> void;

		auto append(const std::vector<double>& data) -> void;
		auto resize(size_t new_size) -> void;
		auto clear() -> void;

	private:
		auto writeHeader(system_helper::fs::OutputFile& file) -> void;
		auto writeData(system_helper::fs::OutputFile& file) -> void;
		std::string filename;
		std::vector<double> data;
		dataset ds;
	};
}  // namespace bestsens
