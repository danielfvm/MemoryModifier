#include "Renderer.h"

Renderer::Renderer(const std::string& frag, const std::string& vert) : Batch(frag, vert)
{
    startDynamicVertexArray();
        addAttribute(0, 2, GL_FLOAT, offsetof(Vertex, x));  // coords
        addAttribute(1, 4, GL_FLOAT, offsetof(Vertex, r));  // color
        setIndicesAsQuad();
    endVertexArray();

    getShader()->bind();
        u_viewProj = getShader()->get_uniform("u_ViewProj");
        u_hasTexture = getShader()->get_uniform("u_HasTexture");
    getShader()->unbind();
}

void Renderer::render(const glm::mat4& proj, const int width, const int height)
{
    std::vector<Vertex> vertices;

    // Enable blend for transparent textures and colors
    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  

    // Bind texture if component has one
    // if (texture) 
    // {
    //     glEnable(GL_TEXTURE_2D);
    //     glActiveTexture(GL_TEXTURE0);
    //     glBindTexture(GL_TEXTURE_2D, texture->id);
    // }

    float x = width / 4, y = 100;
    float w = width / 2, h = 200;

    float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
    
    vertices.push_back(Vertex { x,     y,     r, g, b, a, });
    vertices.push_back(Vertex { x + w, y,     r, g, b, a, });
    vertices.push_back(Vertex { x + w, y + h, r, g, b, a, });
    vertices.push_back(Vertex { x,     y + h, r, g, b, a, });

    // Render component
    bind();
        getShader()->set_uniform(u_viewProj, proj);
        drawDynamic((float*)vertices.data(), vertices.size() / 4);
    unbind();
}

