#include "cocoa.utils.hpp"

#include <ranges>

#import <CommonCrypto/CommonCrypto.h>

namespace saucer
{
    NSUUID *utils::uuid_from(const std::string &identifier)
    {
        // https://stackoverflow.com/questions/64011825/generate-the-same-uuid-from-the-same-string

        const auto guard = utils::autorelease_guard{};
        auto *const data = [NSString stringWithUTF8String:identifier.c_str()];

        unsigned char hash[32] = "";
        CC_SHA256(data.UTF8String, [data lengthOfBytesUsingEncoding:NSUTF8StringEncoding], hash);

        auto top = hash | std::views::drop(16) | std::ranges::to<std::vector<unsigned char>>();

        top[6] &= 0x0F;
        top[6] |= 0x50;

        top[8] &= 0x3F;
        top[8] |= 0x80;

        return [[NSUUID alloc] initWithUUIDBytes:top.data()];
    }
} // namespace saucer
