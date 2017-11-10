//
// Poisson disk kerenl width
//
#define PoissonKernel_ 4

//
// 2, 3, 5, 7
//
#define FilterSize_ 3

// Shadow filter method list
#define ShadowModeSingle_ 0
#define ShadowModeWeighted_ 1
#define ShadowModePoisson_ 2
#define ShadowModePoissonRotated_ 3
#define ShadowModePoissonStratified_ 4
#define ShadowModeOptimizedGaussinPCF_ 5
#define ShadowModeFixedSizePCF_ 6

// Shadow filter method
#define ShadowMode_ ShadowModeOptimizedGaussinPCF_

// #define UsePlaneDepthBias_ 1
static const float Bias = 0.f;