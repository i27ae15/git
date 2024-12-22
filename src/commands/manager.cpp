#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <vector>
#include <zlib.h>

#include <utils.h>
#include <commands/manager.h>
#include <commands/file.h>


namespace Vest {

    CommandManager::CommandManager() :
        actions {
            {INIT, [this](int argc, char *argv[]) { return actionForInit(argc, argv); }},
            {CAT_FILE, [this](int argc, char *argv[]) { return actionForCatFile(argc, argv); }},
            {HASH_OBJECT, [this](int argc, char *argv[]) { return actionForHashObject(argc, argv); }},
            {LS_TREE, [this](int argc, char *argv[]) { return actionForLsTree(argc, argv); }},
        } {}
    CommandManager::~CommandManager() {}

    uint8_t CommandManager::processCommand(int& argc, char *argv[]) {

        std::string command {argv[1]};

        if (argc < 2) return EXIT_FAILURE;

        if (!actions.count(command)) {
            PRINT_ERROR("COMMAND NOT FOUND: " + command);
            return EXIT_FAILURE;
        }

        uint8_t r {};
        try {
            r = actions[command](argc, argv);
        } catch (const std::filesystem::filesystem_error& e) {
            PRINT_ERROR("ERROR WHILE PROCESSING COMMAND: " + command);
            PRINT_ERROR("ERROR: " + std::string(e.what()));
        }

        return r;
    }

    uint8_t CommandManager::actionForInit(int argc, char* argv[]) {

        std::filesystem::create_directory(".git");
        std::filesystem::create_directory(".git/objects");
        std::filesystem::create_directory(".git/refs");

        std::ofstream headFile(".git/HEAD");
        if (headFile.is_open()) {
            headFile << "ref: refs/heads/main\n";
            headFile.close();
        } else {
            PRINT_ERROR("FAILED TO CREATE .git/HEAD file.");
            return EXIT_FAILURE;
        }

        PRINT_SUCCESS("INITIALIZED GIT DIRECTORY");
        return EXIT_SUCCESS;
    }

    uint8_t CommandManager::actionForCatFile(int argc, char* argv[]) {

        std::string parameter = argv[2];
        std::string fileID = argv[3];
        std::string filePath {constructFilePath(fileID)};
        std::vector<unsigned char> compressedData {readFile(filePath)};

        if (compressedData.empty()) {
            PRINT_ERROR("THE FILE IS EMPTY");
            return EXIT_FAILURE;
        }

        // PRINT_WARNING("TO READ: " + std::to_string(compressedData.size()) + " BYTES FROM FILE");

        DecompressedData data {decompressData(compressedData)}; if (data.isEmpty()) return EXIT_FAILURE;
        unsigned char* bufferData = data.data.data() + 5;  // Start 5 bytes into the buffer this to avoid blob

        // Find the null character '\x00'
        while (*bufferData != '\x00') {
            bufferData++;
        }
        // Move past the null character
        bufferData++;

        uint8_t offset = bufferData - data.data.data();
        std::cout.write(reinterpret_cast<char*>(bufferData), data.stream.total_out - offset);

        inflateEnd(&data.stream);
        return EXIT_SUCCESS;

    }

    uint8_t CommandManager::actionForHashObject(int argc, char* argv[]) {

        std::string var {argv[2]};
        std::string filePath {argv[3]};
        std::vector<unsigned char> fileContent = readFile(filePath);
        fileContent = prepareBlob(fileContent);

        std::vector<unsigned char> compressedContent = compressData(fileContent);
        std::string sha1 = computeSHA1(fileContent);

        std::ostringstream pathToSaveFile {};
        pathToSaveFile << ".git/objects/" << sha1[0] << sha1[1];

        std::filesystem::create_directory(pathToSaveFile.str());
        pathToSaveFile << '/' << sha1.substr(2); // Create the directory

        saveToFile(pathToSaveFile.str(), compressedContent);

        std::cout << sha1;
        return EXIT_SUCCESS;
    }

    uint8_t CommandManager::actionForLsTree(int argc, char* argv[]) {
        std::string parameter = argv[2];
        std::string fileID = argv[3];
        std::string filePath {constructFilePath(fileID)};
        std::vector<unsigned char> compressedData {readFile(filePath)};

        DecompressedData data {decompressData(compressedData)};

        unsigned char* bufferData = data.data.data();
        unsigned char* completeString {bufferData}; // This modifies the original data but is more efficient
        unsigned char* toOut {completeString};

        bool isListening {};

        uLong& totalOut = data.stream.total_out;

        while (totalOut-- > 0) {
            unsigned char c {*bufferData};

            if (c == '\x00' && !isListening) {
                isListening = true;
            }
            else if (c == '\x00' && isListening) {
                *completeString++ = '\n';
                bufferData += SHA_SIZE;
                totalOut -= SHA_SIZE;
            }

            bufferData++;
            if (!isListening) continue;

            if (std::isalpha(c)) {
                *completeString++ = c;
            }

        }

        std::cout.write(reinterpret_cast<char*>(toOut), completeString - toOut);

        inflateEnd(&data.stream);
        return EXIT_SUCCESS;

    }

    std::vector<unsigned char> CommandManager::prepareBlob(std::vector<unsigned char>& fileContent) {
        // Allocate a vector to hold the header and file content
        std::vector<unsigned char> blobData;
        std::string header = "blob " + std::to_string(fileContent.size()) + '\0';

        blobData.reserve(header.size() + fileContent.size()); // Reserve space for efficiency
        // Append the header
        blobData.insert(blobData.end(), header.begin(), header.end());

        // Append the file content
        blobData.insert(blobData.end(), fileContent.begin(), fileContent.end());

        return blobData;
    }
}
