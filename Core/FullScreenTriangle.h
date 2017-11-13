#include <vector>
#include "InputLayout.h"

class GraphicsContext;

namespace FullScreenTriangle {
    extern std::vector<InputDesc> InputLayout;

    void Create( void );
    void Clear( void );
    void Draw( GraphicsContext& context );
};