#include "bone_helper/uffWriter.hpp"

#include <chrono>
#include <iterator>
#include <string>
#include <vector>

#include "bone_helper/fsHelper.hpp"
#include "fmt/chrono.h"
#include "fmt/core.h"
#include "fmt/format.h"

namespace bestsens {
	auto UffFile::setFilename(const std::string& filename) -> void {
		this->filename = filename;
	};

	auto UffFile::write() -> void {
		system_helper::fs::OutputFile file(this->filename, std::ios::out);
		this->writeHeader(file);
		this->writeData(file);
	}

	auto UffFile::writeHeader(system_helper::fs::OutputFile& file) -> void {
		auto out = fmt::memory_buffer();

		constexpr auto unknown = -1;
		constexpr auto dset_number = 58;

		fmt::format_to(std::back_inserter(out), "{:6d}\n{:6d}", unknown, dset_number);
		fmt::format_to(std::back_inserter(out), "{:74s}\n", "");

		constexpr auto dset_id1 = "NONE";
		constexpr auto dset_id2 = "NONE";
		constexpr auto dset_id3 = "NONE";
		const auto dset_id4 = fmt::format("{:%d-%b-%y %H:%M:%S}", std::chrono::system_clock::from_time_t(this->ds.date));
		constexpr auto dset_id5 = "NONE";

		fmt::format_to(std::back_inserter(out), "{:<80s}\n", dset_id1);
		fmt::format_to(std::back_inserter(out), "{:<80s}\n", dset_id2);
		fmt::format_to(std::back_inserter(out), "{:<80s}\n", dset_id3);
		fmt::format_to(std::back_inserter(out), "{:<80s}\n", dset_id4);
		fmt::format_to(std::back_inserter(out), "{:<80s}\n", dset_id5);


		constexpr auto func_type = 1;
		constexpr auto func_id = 0;
		constexpr auto ver_num = 0;
		constexpr auto load_case_id = 0;
		constexpr auto rsp_ent_name = "NONE";
		constexpr auto rsp_node = 1;
		constexpr auto rsp_dir = 0;
		const auto ref_ent_name = this->ds.position_name;
		const auto ref_node = this->ds.position;
		constexpr auto ref_dir = 0;

		fmt::format_to(std::back_inserter(out), "{:5d}{:10d}{:5d}{:10d} {:10s}{:10d}{:4d} {:10s}{:10d}{:4d}\n",
					   func_type, func_id, ver_num, load_case_id, rsp_ent_name, rsp_node, rsp_dir, ref_ent_name,
					   ref_node, ref_dir);

		constexpr auto ord_data_type = 4;
		const auto num_pts = this->data.size();
		constexpr auto is_even= 1;
		constexpr auto abcissa_min = 0.0;
		const auto dx = this->ds.dt;
		constexpr auto z_axis_value = 0.0;

		fmt::format_to(std::back_inserter(out), "{:10d}{:10d}{:10d}{:13.5e}{:13.5e}{:13.5e}\n", ord_data_type, num_pts,
					   is_even, abcissa_min, dx, z_axis_value);

		constexpr auto abcissa_spec_datatype = 17;
		constexpr auto abcissa_len_unit_exp = 0;
		constexpr auto abcissa_force_unit_exp = 0;
		constexpr auto abcissa_temp_unit_exp = 0;
		constexpr auto abcissa_axis_lab = "NONE";
		constexpr auto abcissa_axis_units_lab = "s";

		fmt::format_to(std::back_inserter(out), "{:10d}{:5d}{:5d}{:5d} {:<20s} {:<20s}\n", abcissa_spec_datatype,
					   abcissa_len_unit_exp, abcissa_force_unit_exp, abcissa_temp_unit_exp, abcissa_axis_lab,
					   abcissa_axis_units_lab);

		constexpr auto ordinate_spec_datatype = 12;
		constexpr auto ordinate_len_unit_exp = 0;
		constexpr auto ordinate_force_unit_exp = 0;
		constexpr auto ordinate_temp_unit_exp = 0;
		constexpr auto ordinate_axis_lab = "NONE";
		const auto ordinate_axis_units_lab = ds.y_unit;

		fmt::format_to(std::back_inserter(out), "{:10d}{:5d}{:5d}{:5d} {:<20s} {:<20s}\n", ordinate_spec_datatype,
					   ordinate_len_unit_exp, ordinate_force_unit_exp, ordinate_temp_unit_exp, ordinate_axis_lab,
					   ordinate_axis_units_lab);

		constexpr auto orddenom_spec_datatype = 0;
		constexpr auto orddenom_len_unit_exp = 0;
		constexpr auto orddenom_force_unit_exp = 0;
		constexpr auto orddenom_temp_unit_exp = 0;
		constexpr auto orddenom_axis_lab = "NONE";
		constexpr auto orddenom_axis_units_lab = "NONE";

		fmt::format_to(std::back_inserter(out), "{:10d}{:5d}{:5d}{:5d} {:<20s} {:<20s}\n", orddenom_spec_datatype,
					   orddenom_len_unit_exp, orddenom_force_unit_exp, orddenom_temp_unit_exp, orddenom_axis_lab,
					   orddenom_axis_units_lab);

		constexpr auto z_axis_spec_datatype = 0;
		constexpr auto z_axis_len_unit_exp = 0;
		constexpr auto z_axis_force_unit_exp = 0;
		constexpr auto z_axis_temp_unit_exp = 0;
		constexpr auto z_axis_axis_lab = "NONE";
		constexpr auto z_axis_axis_units_lab = "NONE";

		fmt::format_to(std::back_inserter(out), "{:10d}{:5d}{:5d}{:5d} {:<20s} {:<20s}\n", z_axis_spec_datatype,
					   z_axis_len_unit_exp, z_axis_force_unit_exp, z_axis_temp_unit_exp, z_axis_axis_lab,
					   z_axis_axis_units_lab);

		file.write(out.data(), out.size());
	};

	auto UffFile::writeData(system_helper::fs::OutputFile& file) -> void {
		auto out = fmt::memory_buffer();

		int blocks_written = 0;
		for (const auto& e : this->data) {
			fmt::format_to(std::back_inserter(out), "{:20.11e}", e);

			if (++blocks_written == 4) {
				fmt::format_to(std::back_inserter(out), "\n");
				blocks_written = 0;
			}
		}

		if (blocks_written != 0) {
			fmt::format_to(std::back_inserter(out), "\n");
		}

		fmt::format_to(std::back_inserter(out), "{:6d}\n", -1);

		file.write(out.data(), out.size());
	}

	auto UffFile::setParameters(const dataset& ds) -> void {
		this->ds = ds;
	}

	auto UffFile::append(const std::vector<double>& data) -> void {
		this->data.insert(this->data.end(), data.cbegin(), data.cend());
	}

	auto UffFile::resize(size_t new_size) -> void {
		this->data.resize(new_size);
	}

	auto UffFile::clear() -> void {
		this->data.clear();
	}
}  // namespace bestsens