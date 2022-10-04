#include "bone_helper/fsHelper.hpp"

#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>

#include "bone_helper/stdlib_backports.hpp"
#include "bone_helper/strnatcmp.hpp"
#include "bone_helper/system_helper.hpp"
#include "tinydir.h"

namespace bestsens {
	namespace system_helper {
		namespace fs {
			/*
			 * source: https://stackoverflow.com/a/29828907/481329
			 */
			auto isDirExist(const std::string &path) -> bool {
				struct stat info {};
				if (stat(path.c_str(), &info) != 0) {
					return false;
				}

				return (info.st_mode & S_IFDIR) != 0;
			}

			auto makePath(const std::string &path) -> bool {
				const mode_t mode = 0755;

				if (mkdir(path.c_str(), mode) == 0) {
					return true;
				}

				switch (errno) {
				case ENOENT:
					// parent didn't exist, try to create it
					{
						const auto pos = path.find_last_of('/');

						if (pos == std::string::npos) {
							return false;
						}

						if (!makePath(path.substr(0, pos))) {
							return false;
						}
					}

					// now, try to create again
					return 0 == mkdir(path.c_str(), mode);
				case EEXIST:
					// done!
					return isDirExist(path);
				default:
					return false;
				}
			}

			auto deleteFilesRecursive(const std::string &directory_location) -> void {
				tinydir_dir dir;

				if (tinydir_open(&dir, directory_location.c_str()) != 0) {
					throw std::runtime_error("error opening directory " + directory_location);
				}

				while (dir.has_next != 0) {
					tinydir_file file;

					tinydir_readfile(&dir, &file);
					tinydir_next(&dir);

					const std::string current_filename(static_cast<char *>(file.name));

					/*
					 * this check is mandatory to not traverse upwards in folder structure!
					 */
					if (current_filename == "." || current_filename == "..") {
						continue;
					}

					const auto full_path = directory_location + "/" + current_filename;

					if (file.is_dir != 0) {
						deleteFilesRecursive(full_path);
					}

					/*
					 * we only want to delete regular files or folders, no symlinks or stuff
					 * we don't know what happens if we just delete them
					 */
					if (file.is_dir == 0 && file.is_reg == 0) {
						throw std::runtime_error("cannot delete " + full_path + ": not a regular file");
					}

					if (std::remove(full_path.c_str()) != 0) {
						throw std::runtime_error("error deleting file " + full_path + ": " +
												 strerror_s(errno));  // NOLINT(concurrency-mt-unsafe)
					}
				}

				if (std::remove(directory_location.c_str()) != 0) {
					throw std::runtime_error("error deleting file " + directory_location + ": " +
											 strerror_s(errno));  // NOLINT(concurrency-mt-unsafe)
				}
			}

			auto getDirectoriesUnsorted(const std::string &directory_location, bool recursive)
				-> std::vector<std::string> {
				std::vector<std::string> result;

				tinydir_dir dir;

				if (tinydir_open(&dir, directory_location.c_str()) != 0) {
					throw std::runtime_error("error opening directory " + directory_location);
				}

				try {
					while (dir.has_next != 0) {
						tinydir_file file;

						tinydir_readfile(&dir, &file);
						tinydir_next(&dir);

						if (file.is_dir == 0) {
							continue;
						}

						const std::string entry(static_cast<char *>(file.name));

						if (entry != "." && entry != "..") {
							const auto new_directory = directory_location + "/" + entry;
							result.push_back(new_directory);

							if (recursive) {
								const auto new_result = getDirectories(new_directory, recursive);
								result.insert(std::end(result), std::begin(new_result), std::end(new_result));
							}
						}
					}
				} catch (...) {}

				tinydir_close(&dir);

				return result;
			}

			auto getDirectories(const std::string &directory_location, bool recursive) -> std::vector<std::string> {
				auto result = getDirectoriesUnsorted(directory_location, recursive);
				std::sort(result.begin(), result.end(), compareNat);

				return result;
			}

			auto readDirectoryUnsorted(const std::string &directory_location, const std::string &start_string,
									   const std::string &extension, bool full_path = true)
				-> std::vector<std::string> {
				std::vector<std::string> result;

				std::string lc_extension(extension);
				std::transform(lc_extension.begin(), lc_extension.end(), lc_extension.begin(), ::tolower);

				tinydir_dir dir;

				if (tinydir_open(&dir, directory_location.c_str()) != 0) {
					throw std::runtime_error("error opening directory" + directory_location);
				}

				try {
					while (dir.has_next != 0) {
						tinydir_file file;

						tinydir_readfile(&dir, &file);
						tinydir_next(&dir);

						if (file.is_dir != 0) {
							continue;
						}

						const std::string entry(static_cast<char *>(file.name));
						std::string lc_entry(entry);
						std::transform(lc_entry.begin(), lc_entry.end(), lc_entry.begin(), ::tolower);

						if (backports::startsWith(entry, start_string) && backports::endsWith(lc_entry, lc_extension)) {
							if (full_path) {
								result.push_back(directory_location + "/" + entry);
							} else {
								result.push_back(entry);
							}
						}
					}
				} catch (...) {}

				tinydir_close(&dir);

				return result;
			}

			auto readDirectory(const std::string &directory_location, const std::string &start_string,
							   const std::string &extension, bool full_path = true) -> std::vector<std::string> {
				auto result = readDirectoryUnsorted(directory_location, start_string, extension, full_path);
				std::sort(result.begin(), result.end());

				return result;
			}

			auto readDirectoryNatural(const std::string &directory_location, const std::string &start_string,
									  const std::string &extension, bool full_path = true) -> std::vector<std::string> {
				auto result = readDirectoryUnsorted(directory_location, start_string, extension, full_path);
				std::sort(result.begin(), result.end(), compareNat);

				return result;
			}

		}  // namespace fs
	}	   // namespace system_helper
}  // namespace bestsens