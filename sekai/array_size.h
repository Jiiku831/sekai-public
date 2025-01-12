#pragma once

#include <vector>

namespace sekai {

int AreaItemArraySize();
constexpr int kCardArraySize = 2048;
int CardArraySize();
const std::vector<int>& UniqueCharacterIds();
constexpr int kCharacterArraySize = 32;
int CharacterArraySize();
constexpr int kMySekaiGateArraySize = 8;
int MySekaiGateArraySize();

}  // namespace sekai
