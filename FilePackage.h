#pragma once

#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

#pragma comment(lib, "FilePackage")

bool compress_stream(std::istream* src, std::ostream* dst, size_t Level);
bool decompress_stream(std::istream* src, std::ostream* dst);

class FilePackage
{

	std::stringstream Stream;
	// Tuple is [filename], [offset], [size]
	std::unordered_map<std::string, std::pair<size_t, size_t>> Files;
	size_t NewOffset;

public:

	// Default constructor
	FilePackage();

	// Default destructor
	~FilePackage();

	// Adds file to packer
	void AddFile(const std::string& FileContents, std::string FileName, int Size = -1);

	// Adds file to packer
	void AddFile(const std::istream& FileContents, std::string FileName, int Size = -1);

	// Deletes element from the list
	// Seeks position of the stream to 0!
	void DeleteFile(std::string FileName);

	// Clears the filelist
	void ClearFiles();

	// Saves the entire filelist to one file package
	void SavePackage(std::string FileName, size_t CompressionLevel = 5);

	// Loads the entire filelist from one file package
	void LoadPackage(std::string FileName);
	
	// Saves file from package to the disk
	void SaveToFile(std::string FileName, std::string DestinationFileName);

	// Saves file from package to the stream
	void SaveToFile(std::string FileName, std::ofstream& DestinationStream);

	// std::stringstream class adress getter
	// Seeks stream to pos of the target file
	std::stringstream& GetStream(std::string FileName);

	// std::stringstream class adress getter
	// Seeks to begin of file stream main data
	std::stringstream& GetStream(bool SeekToBegin = false);

	// Returns size of the target file
	// If empty returns size of the entire data
	size_t GetFileSize(std::string FileName = "");

	// Returns offset of the target file
	size_t GetOffset(std::string FileName = "");

	// Returns the entire pack data in string
	// Leave the filename empty to get full data
	// Write filename to get substring of target file
	std::string GetDataString(std::string FileName = "");

	// Returns the data of all files in package
	// In map:
	// As key - filename
	// As value - pair of [Offset],[Size]
	// Offset begins from 0
	std::unordered_map<std::string, std::pair<size_t, size_t>> GetFilesData();

};