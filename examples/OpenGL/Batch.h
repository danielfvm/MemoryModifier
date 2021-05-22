#ifndef BATCH_H
#define BATCH_H

#include <GL/glew.h>
#include <GL/gl.h>

#include <vector>

#include "shader.h"

template<typename Vertex, int MaxVertexCount, int MaxIndexCount>
class Batch 
{
private:
    Shader* m_shader;

    GLuint m_vao, m_vbo, m_ibo;

public:
    Batch(const std::string& fragmentCode, const std::string& vertexCode) 
    {
        m_shader = new Shader(fragmentCode.c_str(), vertexCode.c_str());
    }

    inline int getMaxVertexCount() { return MaxVertexCount; }
    inline int getMaxIndicesCount() { return MaxIndexCount; }
    inline Shader* getShader() { return m_shader; }

protected:
    void bind() 
    {
        m_shader->bind();
    }

    void drawDynamic(float* vertices, size_t count) 
    {
        // Set dynamic buffer
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, count * 4 * sizeof(Vertex), vertices);

        // Bind and draw VertexArray
        glBindVertexArray(m_vao);
        glDrawElements(GL_TRIANGLES, 6 * count, GL_UNSIGNED_INT, nullptr);
    }

    void drawStatic() 
    {
        glBindVertexArray(m_vao);
        glDrawElements(GL_TRIANGLES, MaxIndexCount, GL_UNSIGNED_INT, nullptr);
    }

    void unbind() 
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0); 
        m_shader->unbind();
    }


    void setIndicesAsQuad() 
    {
        uint32_t indices[MaxIndexCount];
        uint32_t offset = 0;

        for (size_t i = 0; i < MaxIndexCount; i += 6)
        {
            indices[i + 0] = 0 + offset;
            indices[i + 1] = 1 + offset;
            indices[i + 2] = 3 + offset;
            indices[i + 3] = 3 + offset;
            indices[i + 4] = 1 + offset;
            indices[i + 5] = 2 + offset;

            offset += 4;
        }

        setIndices(indices, sizeof(indices));
    }

    void setIndices(uint32_t* indices, size_t size) 
    {
        glGenBuffers(1, &m_ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, GL_STATIC_DRAW);
    }

    void startDynamicVertexArray() 
    {
        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao); 

        glGenBuffers(1, &m_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * MaxVertexCount, nullptr, GL_DYNAMIC_DRAW);
    }

    /* NOT TESTED */
    void startStaticVertexArray(void** vertices) 
    {
        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao); 

        glGenBuffers(1, &m_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * MaxVertexCount, vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void addAttribute(int location, int size, int type, unsigned long offset) 
    {
        glEnableVertexAttribArray(location);
        glVertexAttribPointer(location, size, type, GL_FALSE, sizeof(Vertex), (const void*)offset);
    }

    void endVertexArray() 
    {
        glBindVertexArray(0); 
    }
};

#endif // BATCH_H
