#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <vector>
#include <zlib.h>

#include <utils.h>
#include <commands/manager.h>


namespace Vest {

    CommandManager::CommandManager() :
        actions {
            {INIT, [this](int argc, char *argv[]) { return actionForInit(argc, argv); }},
            {CAT_FILE, [this](int argc, char *argv[]) { return actionForCatFile(argc, argv); }},
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

    uint8_t CommandManager::actionForInit(int argc, char *argv[]) {

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

    uint8_t CommandManager::actionForCatFile(int argc, char *argv[]) {

        std::string parameter = argv[2];
        std::string fileID = argv[3];
        std::ostringstream filePath {};

        filePath << ".git/objects/" << fileID[0] << fileID[1] << '/' + fileID.substr(2);
        std::ifstream file(filePath.str(), std::ios::binary);

        if (!file) {
            PRINT_ERROR("FAILED TO OPEN FILE: " + filePath.str());
            return EXIT_FAILURE;
        }

        std::vector<unsigned char> compressedData (
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>()
        );

        file.close();

        if (compressedData.empty()) {
            PRINT_ERROR("THE FILE IS EMPTY");
            return EXIT_FAILURE;
        }

        // PRINT_WARNING("TO READ: " + std::to_string(compressedData.size()) + " BYTES FROM FILE");

        std::vector<unsigned char> decompressedBuffer(DECOMPRESSED_SIZE);

        z_stream stream {};
        stream.next_in = const_cast<Bytef*>(compressedData.data());
        stream.avail_in = compressedData.size();
        stream.next_out = decompressedBuffer.data();
        stream.avail_out = DECOMPRESSED_SIZE;

        if (inflateInit(&stream) != Z_OK) {
            PRINT_ERROR("FAILED TO INIT ZLIB FOR DECOMPRESSION");
            return EXIT_FAILURE;
        }

        int result = inflate(&stream, Z_FINISH);
        if (result != Z_STREAM_END) {
            PRINT_ERROR("DECOMPRESSION_FAILED");
            return EXIT_FAILURE;
        }

        unsigned char* bufferData = decompressedBuffer.data() + 5;  // Start 5 bytes into the buffer this to avoid blob

        // Find the null character '\x00'
        while (*bufferData != '\x00') {
            bufferData++;
        }
        // Move past the null character
        bufferData++;

        uint8_t offset = bufferData - decompressedBuffer.data();
        std::cout.write(reinterpret_cast<char*>(bufferData), stream.total_out - offset);

        inflateEnd(&stream);
        return EXIT_SUCCESS;

    }
}
