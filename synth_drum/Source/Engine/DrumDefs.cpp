#include "DrumDefs.h"

#include <algorithm>

namespace mds
{
std::string makePadName(const int padIndex)
{
    const auto clamped = std::clamp(padIndex, 0, kNumPads - 1);
    return std::string(kPadNames[static_cast<std::size_t>(clamped)]);
}
} // namespace mds
