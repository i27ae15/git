#include <string>
#include <openssl/sha.h>

#include <file/types.h>
#include <file/utils.h>

namespace VestFileUtils {
    std::string constructFileLine(
        VestTypes::FileType& fileType,
        std::string& sha1Hex,
        std::string& fileName
    ) {

        std::string sha1Bytes = hexToBytes(sha1Hex);
        return std::string(fileType.mode) + " " + fileName + '\x00' + sha1Bytes;

    }

    std::string hexToBytes(const std::string& hex) {
        std::string bytes;
        for (size_t i = 0; i < hex.length(); i += 2) {
            std::string byteString = hex.substr(i, 2);
            char byte = static_cast<char>(strtol(byteString.c_str(), nullptr, 16));
            bytes.push_back(byte);
        }
        return bytes;
    }

    std::string computeSHA1(std::string& inputData) {
        std::vector<unsigned char> data(inputData.begin(), inputData.end());
        return computeSHA1(data);
    }

    std::string computeSHA1(const std::vector<unsigned char>& data) {
        unsigned char hash[SHA_DIGEST_LENGTH];
        SHA1(data.data(), data.size(), hash);

        // Convert to hexadecimal string
        std::stringstream ss;
        for (uint8_t i {}; i < SHA_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        }

        return ss.str();
    }

    std::string constructFilePath(std::string fileID, std::string root) {
        std::ostringstream filePath {};
        filePath << root << fileID[0] << fileID[1] << '/' + fileID.substr(2);

        return filePath.str();
    }
}
