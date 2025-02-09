#ifndef VEST_PACK_H
#define VEST_PACK_H

#include <cstdint>
#include <vector>


namespace VestPack {

    void processPack(
        std::vector<uint8_t>& rData,
        size_t offset,
        std::string& dir
    );

}

#endif // VEST_PACK_H