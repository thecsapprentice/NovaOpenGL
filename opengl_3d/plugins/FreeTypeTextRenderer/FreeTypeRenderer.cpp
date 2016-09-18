#include "../../ApplicationFactory.h"
#include "../../TextRenderingService.h"
#include "../../Shader.h"

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H  

#include <map>
#include <iostream>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../../shaders/BasicTextShader.h"

namespace Nova {

    class FreeTypeRenderer : public TextRenderer {
    public:

        /// Holds all state information relevant to a character as loaded using FreeType
        struct Character {
            GLuint TextureID;   // ID handle of the glyph texture
            glm::ivec2 Size;    // Size of glyph
            glm::ivec2 Bearing; // Offset from baseline to left/top of glyph
            GLuint Advance;     // Horizontal offset to advance to next glyph
        };
        
        FreeTypeRenderer( std::string fontname ) : TextRenderer() {
            // Load and configure shader
            _shader = std::unique_ptr<Shader>( new Shader() );
            _shader->LoadFromString(NovaBuiltinShaders::BasicTextShader::vertex_shader,
                                    NovaBuiltinShaders::BasicTextShader::fragment_shader);
            _shader->SetInteger("text", 0);
            // Configure VAO/VBO for texture quads
            glGenVertexArrays(1, &this->VAO);
            glGenBuffers(1, &this->VBO);
            glBindVertexArray(this->VAO);
            glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);

            LoadFont( fontname, 12 );
        }
        
        virtual ~FreeTypeRenderer() {

            
        };
        
        virtual void RenderText( std::string text, float scale, float x, float y, float width, float height ){

            // Activate corresponding render state	
            this->_shader->Use();
            this->_shader->SetVector3f("textColor", glm::vec3(1,1,1) );
            _shader->SetMatrix4("projection", glm::ortho(0.0f,
                                                         static_cast<GLfloat>(width),
                                                         static_cast<GLfloat>(height),
                                                         0.0f));
            glActiveTexture(GL_TEXTURE0);
            glBindVertexArray(this->VAO);

            // Iterate through all characters
            std::string::const_iterator c;
            for (c = text.begin(); c != text.end(); c++)
                {
                    Character ch = Characters[*c];

                    GLfloat xpos = x + ch.Bearing.x * scale;
                    GLfloat ypos = y + (this->Characters['H'].Bearing.y - ch.Bearing.y) * scale;

                    GLfloat w = ch.Size.x * scale;
                    GLfloat h = ch.Size.y * scale;
                    // Update VBO for each character
                    GLfloat vertices[6][4] = {
                        { xpos,     ypos + h,   0.0, 1.0 },
                        { xpos + w, ypos,       1.0, 0.0 },
                        { xpos,     ypos,       0.0, 0.0 },

                        { xpos,     ypos + h,   0.0, 1.0 },
                        { xpos + w, ypos + h,   1.0, 1.0 },
                        { xpos + w, ypos,       1.0, 0.0 }
                    };
                    // Render glyph texture over quad
                    glBindTexture(GL_TEXTURE_2D, ch.TextureID);
                    // Update content of VBO memory
                    glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
                    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // Be sure to use glBufferSubData and not glBufferData

                    glBindBuffer(GL_ARRAY_BUFFER, 0);
                    // Render quad
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                    // Now advance cursors for next glyph
                    x += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (1/64th times 2^6 = 64)
                }
            glBindVertexArray(0);
            glBindTexture(GL_TEXTURE_2D, 0);

        }
        
    private:
        
        std::unique_ptr<Shader> _shader;
        // Holds a list of pre-compiled Characters
        std::map<GLchar, Character> Characters;
        // Render state
        GLuint VAO, VBO;
                
        void LoadFont( std::string font, int fontSize ){
            // First clear the previously loaded Characters
            this->Characters.clear();
            // Then initialize and load the FreeType library
            FT_Library ft;    
            if (FT_Init_FreeType(&ft)) // All functions return a value different than 0 whenever an error occurred
                std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
            // Load font as face
            FT_Face face;
            if (FT_New_Face(ft, font.c_str(), 0, &face))
                std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
            // Set size to load glyphs as
            FT_Set_Pixel_Sizes(face, 0, fontSize);
            // Disable byte-alignment restriction
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1); 
            // Then for the first 128 ASCII characters, pre-load/compile their characters and store them
            for (GLubyte c = 0; c < 128; c++) // lol see what I did there 
                {
                    // Load character glyph 
                    if (FT_Load_Char(face, c, FT_LOAD_RENDER))
                        {
                            std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
                            continue;
                        }
                    // Generate texture
                    GLuint texture;
                    glGenTextures(1, &texture);
                    glBindTexture(GL_TEXTURE_2D, texture);
                    glTexImage2D(
                                 GL_TEXTURE_2D,
                                 0,
                                 GL_RED,
                                 face->glyph->bitmap.width,
                                 face->glyph->bitmap.rows,
                                 0,
                                 GL_RED,
                                 GL_UNSIGNED_BYTE,
                                 face->glyph->bitmap.buffer
                                 );
                    // Set texture options
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
       
                    // Now store character for later use
                    Character character = {
                        texture,
                        glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                        glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                        GLuint(face->glyph->advance.x)
                    };
                    Characters.insert(std::pair<GLchar, Character>(c, character));
                }
            glBindTexture(GL_TEXTURE_2D, 0);
            // Destroy FreeType once we're finished
            FT_Done_Face(face);
            FT_Done_FreeType(ft);

        }
        
    };
    
}

extern "C" void registerPlugin(Nova::ApplicationFactory& app) {
    app.GetTextRenderingService().RegisterProvider( std::move( std::unique_ptr<Nova::TextRenderer>( new Nova::FreeTypeRenderer("fonts/arial.ttf") )));
}

extern "C" int getEngineVersion() {
    return Nova::API_VERSION;
}
