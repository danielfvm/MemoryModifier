#ifndef RENDERER_H

#define RENDERER_H

#include <algorithm>
#include <functional>
#include <string>
#include <vector>

#include "Batch.h"

#include <glm/mat4x4.hpp>

#define MAX_RECT 1000

struct Vertex {
    float x, y;
    float r, g, b, a;
};

class Renderer : public Batch<Vertex, MAX_RECT * 4, MAX_RECT * 6>
{
private:
    unsigned int u_viewProj;
    unsigned int u_hasTexture;

public:
    Renderer(const std::string& frag, const std::string& vert);

    void render(const glm::mat4& proj, const int width, const int height);
};

#endif // RENDERER_H
