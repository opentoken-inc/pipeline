#ifndef _OPENTOKEN__HARE__COINS_H_
#define _OPENTOKEN__HARE__COINS_H_

#include <inttypes.h>

namespace opentoken {

enum class CoinCode : uint32_t {
  Unknown = 0,
  Bitcoin,
  Ethereum,
  Tether,
  LiteCoin,
  Tron,
  ZeroX,
};

}  // namespace opentoken

#endif  // _OPENTOKEN__HARE__COINS_H_
