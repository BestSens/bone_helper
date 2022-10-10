/*
 * fsHelper.hpp
 *
 *  Created on: 04.10.2022
 *	  Author: Jan Schöppach
 */

#pragma once

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>


namespace bestsens {
	namespace system_helper {
		namespace fs {
			auto deleteFilesRecursive(const std::string &directory_location) -> void;
			auto getDirectories(const std::string &directory_location, bool recursive) -> std::vector<std::string>;
			auto getDirectoriesUnsorted(const std::string &directory_location, bool recursive)
				-> std::vector<std::string>;

			auto readDirectory(const std::string &directory_location, const std::string &start_string,
							   const std::string &extension, bool full_path) -> std::vector<std::string>;

			auto readDirectoryUnsorted(const std::string &directory_location, const std::string &start_string,
									   const std::string &extension, bool full_path) -> std::vector<std::string>;

			auto readDirectoryNatural(const std::string &directory_location, const std::string &start_string,
									  const std::string &extension, bool full_path) -> std::vector<std::string>;

			auto isDirExist(const std::string& path) -> bool;
			auto makePath(const std::string& path) -> bool;

			class OutputFile {
			public:
				OutputFile() = default;
				explicit OutputFile(std::ios_base::openmode mode) : mode(mode) {}

				explicit OutputFile(const std::string& filename) {
					if (this->openFile(filename) != 0) {
						throw std::runtime_error("file could not be opened");
					}
				}

				OutputFile(const std::string &filename, std::ios_base::openmode mode) : mode(mode) {
					if (this->openFile(filename) != 0) {
						throw std::runtime_error("file could not be opened");
					}
				}

				~OutputFile() {
					this->closeFile();
				};

				OutputFile(const OutputFile&) = delete;
				OutputFile(OutputFile&&) = default;

				auto operator=(const OutputFile&) -> void = delete;
				auto operator=(OutputFile&&) -> OutputFile& = default;

				auto updateFilename(const std::string& new_filename) -> int {
					/*
					 * open new file when filename changed, oder underlying file is closed
					 */
					if (this->filename != new_filename) {
						return openFile(new_filename);
					} else {
						if (!this->filehandle.is_open()) {
							return openFile(new_filename);
						}
					}

					return 0;
				};

				auto closeFile() -> void {
					if (this->filehandle.is_open()) {
						this->filehandle.close();
						this->filename.clear();
					}
				};

				auto getFilename() const -> std::string {
					return this->filename;
				};

				// NOLINTNEXTLINE (readability-identifier-naming)
				auto is_open() const {
					return this->filehandle.is_open();
				}

				auto fail() const {
					return this->filehandle.fail();
				}

				auto bad() const {
					return this->filehandle.bad();
				}

				template <typename... Args>
				auto write(Args&&... args) -> std::ostream& {
					return this->filehandle.write(std::forward<Args>(args)...);
				}

				template <typename T>
				auto operator<<(T sb) -> std::ostream& {
					return this->filehandle << sb;
				}

			private:
				auto openFile(const std::string &filename) -> int {
					this->closeFile();
					OutputFile::createFolderPath(filename);
					this->filehandle.open(filename.c_str(), this->mode);

					if (this->filehandle.is_open()) {
						this->filename = filename;
						return 0;
					}

					return 1;
				};

				static auto createFolderPath(const std::string& filename) -> int {
					const auto pos = filename.find_last_of('/');

					if (pos == std::string::npos) {
						return 0;
					}

					const auto filepath = filename.substr(0, pos);
					return makePath(filepath) ? 0 : 1;
				};

				std::string filename;
				std::string type;
				std::string main_dir;

				std::ofstream filehandle;

				std::ios_base::openmode mode{std::ios::out | std::ios::binary | std::ios::app};
			};

		}  // namespace fs
	}	   // namespace system_helper
}  // namespace bestsens
