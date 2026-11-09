#ifndef _CardEvaluator_CardEvaluator_h
#define _CardEvaluator_CardEvaluator_h

#include <vector>
#include <array>
#include <cstdint>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <memory>
#include <limits>
#include <cstddef>
#include <cassert>
#include <cstdint>
#include <memory>
#include <limits>
#include <cstddef>
#include <cassert>
#include <cstdint>
#include <algorithm>
#include <iterator>
#include <random>
#include <cassert>
#include <array>
#include <memory>
#include <cstdint>
#include <cassert>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <array>
#include <cstdint>
#include <functional>

#undef OMP_SSE2
#define OMP_SSE2 0

#if OMP_SSE2
    #include <xmmintrin.h> // SSE1
    #include <emmintrin.h> // SSE2
    #if OMP_SSE4
        #include <smmintrin.h> // SSE4.1
    #endif
#endif

#if _MSC_VER
#include <intrin.h>
#endif


#if _MSC_VER
#include <intrin.h>
#endif

#include <Core/Core.h>

#include "libdivide.h"
#include "Util.h"
#include "Constants.h"
#include "Hand.h"
#include "HandEvaluator.h"
#include "CardRange.h"
#include "CombinedRange.h"
#include "Random.h"
#include "EquityCalculator.h"
#include "Constants.h"




#endif
