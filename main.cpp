#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>

#include <vector>
#include <array>
#include <map>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_opengl.h>

//#include <GLES2/gl2.h>
#include <GLES3/gl3.h>

#include <ft2build.h>
#include FT_FREETYPE_H

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
SDL_Window *windows;

// Modified Cxxdroid's load texture function
static SDL_Surface *load_surface(const char *path)
{
    SDL_Surface *img = IMG_Load(path);
    if (img == NULL)
    {
        fprintf(stderr, "IMG_Load Error: %s\n", IMG_GetError());
        return NULL;
    }
    return img;
}

struct Vec2f {
    float x, y;
    Vec2f() {}
    Vec2f(float x, float y) : x(x), y(y) {}
};

struct Vec3f {
    float x, y, z;
    Vec3f() {}
    Vec3f(float x, float y, float z) : x(x), y(y), z(z) {}
    Vec3f(const Vec3f &other) : x(other.x), y(other.y), z(other.z) {}
    
    void set_zero() {
        x = 0;
        y = 0;
        z = 0;
    }
    float dot_prod(Vec3f &other) {
        return x * other.x + y * other.y + z * other.z;
    }
    Vec3f cross_prod(Vec3f &other) { 
        float cx = y * other.z - z * other.y;
        float cy = z * other.x - x * other.z;
        float cz = x * other.y - y * other.x;
        
        Vec3f result = { cx, cy, cz };
        
        return result;
    }  
    float len() {
        return sqrt(x*x + y*y + z*z);
    }
    Vec3f norm() {
        return mul(1 / len());
    }
    Vec3f add(const Vec3f &other) {
        x = x + other.x;
        y = y + other.y;
        z = z + other.z;
        
        return *this;
    }
    Vec3f sub(const Vec3f &other) {
        x = x - other.x;
        y = y - other.y;
        z = z - other.z;
        
        return *this;
    }
    Vec3f mul(float scalar) {
        x = x * scalar;
        y = y * scalar;
        z = z * scalar;
        
        return *this;
    }
    Vec3f mul(const Vec3f &vector) {
        x = x * vector.x;
        y = y * vector.y;
        z = z * vector.z;
        
        return *this;
    }
    Vec3f set_to_random_position(int range) {
        x = rand() % range - range / 2.0f;
        y = rand() % range - range / 2.0f;
        z = rand() % range - range / 2.0f;
        
        return *this;
    }
    Vec3f abs() {
        x = fabs(x);
        y = fabs(y);
        z = fabs(z);
        
        return *this;
    }
    Vec3f max(float bound) {
        x = std::max(x, bound);
        y = std::max(y, bound);
        z = std::max(z, bound);
        
        return *this;
    }
};
struct Vec2i {
    int x, y;
};

class Mat4x4 {
    public:
       int M00 = 0,  M10 = 1,  M20 = 2,  M30 = 3,
           M01 = 4,  M11 = 5,  M21 = 6,  M31 = 7,
           M02 = 8,  M12 = 9,  M22 = 10, M32 = 11,
           M03 = 12, M13 = 13, M23 = 14, M33 = 15;
           
       float values[4 * 4] = { 0 };
       
       Mat4x4() {
           this->identity();
       }
       void identity() {
           for (int i = 0; i < 4 * 4; i++) {
               values[i] = 0.0f;
           }
           values[M00] = 1.0f;
           values[M11] = 1.0f;
           values[M22] = 1.0f;
           values[M33] = 1.0f;
       }
       void set_perspective(float fovDegrees, float zNear, float zFar, float aspectRatio) {            
           float fovR = float(1.0f / tan(fovDegrees * (M_PI / 180.0f) / 2.0f));           
           float range = zFar - zNear;
           
           identity();                    
           values[M00] = fovR / aspectRatio;            
           values[M11] = fovR;            
           
           values[M22] = -(zFar + zNear) / range;            
           values[M32] = -(2 * zFar * zNear) / range;           
           values[M23] = -1.0f;            
           values[M33] = 0.0f;
       }
       void set_orthographic(float left, float right, float bottom, float top, float near, float far) {
           identity();
           
           values[M00] = 2.0f / (right - left);
           values[M11] = 2.0f / (top - bottom);
           values[M22] = -2.0f / (far - near);
           
           values[M30] = -(right + left) / (right - left);
           values[M31] = -(top + bottom) / (top - bottom);
           values[M32] = -(far + near) / (far - near);
       }
       void set_look_at(Vec3f cameraPosition, Vec3f lookingAt, Vec3f up) {
           Vec3f fwd = { cameraPosition.x - lookingAt.x,
                          cameraPosition.y - lookingAt.y,
                          cameraPosition.z - lookingAt.z };
           fwd.norm();
           
           Vec3f cameraXAxis = fwd.cross_prod(up);
           cameraXAxis.norm();
           
           Vec3f cameraYAxis = cameraXAxis.cross_prod(fwd);
           
           identity();
           
           values[M00] = cameraXAxis.x;
           values[M10] = cameraXAxis.y;
           values[M20] = cameraXAxis.z;
     
           values[M01] = cameraYAxis.x;
           values[M11] = cameraYAxis.y;
           values[M21] = cameraYAxis.z;
           
           values[M02] = fwd.x;
           values[M12] = fwd.y;
           values[M22] = fwd.z;
           
           values[M30] = -cameraXAxis.dot_prod(cameraPosition);
           values[M31] = -cameraYAxis.dot_prod(cameraPosition);
           values[M32] = -fwd.dot_prod(cameraPosition);
           
       }
       
       
       void set_translation(float x, float y, float z) {
           identity();
           
           values[M30] = x;
           values[M31] = y;
           values[M32] = z;
       }
       void set_translation(Vec3f to) {
           set_translation(to.x, to.y, to.z);
       }
       
       void set_scaling(float x, float y, float z) {
           identity();
           
           values[M00] = x;
           values[M11] = y;
           values[M22] = z;
       }
       void set_scaling(Vec3f to) {
           set_scaling(to.x, to.y, to.z);
       }
       void set_scaling(float scalar) {
           set_scaling(scalar, scalar, scalar);
       }
       void set_rotationX(float radians) {
           identity();
           
           values[M11] = cos(radians);
           values[M21] = -sin(radians);
           values[M12] = sin(radians);
           values[M22] = cos(radians);
       }
       void set_rotationY(float radians) {
           identity();
           
           values[M00] = cos(radians);
           values[M20] = sin(radians);
           values[M02] = -sin(radians);
           values[M22] = cos(radians);
       }
       Mat4x4 multiply(Mat4x4 &with) {
           Mat4x4 r;
           
           int row = 0;
           int column = 0;
           for (int i = 0; i < 16; i++) {
               row = (i / 4) * 4;
               column = (i % 4);
               r.values[i] = (values[row + 0] * with.values[column + 0]) +
                             (values[row + 1] * with.values[column + 4]) +
                             (values[row + 2] * with.values[column + 8]) +
                             (values[row + 3] * with.values[column + 12]);
           }
           
           return r;
       }
       /*
       float determinant() {
           auto v = [&](int index) -> float {
                return values[index];
           };
           
           
       }
       */
       // Print the column-major matrix
       std::string to_string() {
           return 
           "[ " + std::to_string(values[M00]) + "|" + std::to_string(values[M10]) + "|" + std::to_string(values[M20]) + "|" + std::to_string(values[M30]) + "|\n  " +
                 std::to_string(values[M01]) + "|" + std::to_string(values[M11]) + "|" + std::to_string(values[M21]) + "|" + std::to_string(values[M31]) + "|\n  " +
                 std::to_string(values[M02]) + "|" + std::to_string(values[M12]) + "|" + std::to_string(values[M22]) + "|" + std::to_string(values[M32]) + "|\n  " + 
                 std::to_string(values[M03]) + "|" + std::to_string(values[M13]) + "|" + std::to_string(values[M23]) + "|" + std::to_string(values[M33]) + " ]\n";
       }
};

struct Plane {
    Vec3f position;
    Vec3f normal;
    Plane() {}
    Plane(Vec3f position, Vec3f normal) : position(position), normal(normal) {}
    
    Vec3f intersect_line(Vec3f linePosition, Vec3f lineDirection) {
          Vec3f normalized = Vec3f(lineDirection).norm();
          bool isParallel = normal.dot_prod(normalized) == 0;
          if (isParallel) {
               return position;
          }
          
          // Linear interpolation
          float alpha = (normal.dot_prod(position) - normal.dot_prod(linePosition)) / normal.dot_prod(normalized);
          return Vec3f(linePosition).add(normalized.mul(alpha));
    }
};

struct AABB {
    Vec3f min;
    Vec3f max;
    AABB() {}
    AABB(Vec3f min, Vec3f max) : min(min), max(max) {}
    
    bool point_inside(const Vec3f &point) {
         return (point.x >= min.x && point.x <= max.x) &&
                (point.y >= min.y && point.y <= max.y) && 
                (point.z >= min.z && point.z <= max.z);
    }
};

class Camera {
    public:
        Vec3f position;
        Vec3f lookingAt;
        Vec3f lookDirection;
        float rotationX;
        float rotationY;
        
        Camera(float fov, float zNear, float zFar, float width, float height, bool perspective) {
            position.set_zero();
            rotationX = 0.0f;
            rotationY = 0.0f;
            lookingAt = { 1.0f, 0.0f, 0.0f };
            
            this->width = width;
            this->height = height;
            
            this->fov = fov;
            this->zNear = zNear;
            this->zFar = zFar;
            this->perspective = perspective;
        }
        
        Camera() : Camera(90.0f, 0.1f, 1000.0f, float(SCREEN_WIDTH), float(SCREEN_HEIGHT), true) {
        }
        
        void update() {
            lookingAt.x = position.x + cos(rotationX) * cos(rotationY);
            lookingAt.y = position.y + sin(rotationY);
            lookingAt.z = position.z + sin(rotationX) * cos(rotationY);
           
            Vec3f up = { 0.0f, 1.0f, 0.0f };
            viewMat.set_look_at(position, lookingAt, up);
            
            if (perspective) {
                projMat.set_perspective(fov, zNear, zFar, width / height);
            } else {
                projMat.set_orthographic(-width / 2, width / 2, -height / 2, height / 2, zNear, zFar);
            }
            combined = viewMat.multiply(projMat);
        }
        void resize(float width, float height) {
            this->width = width;
            this->height = height;
        }
        Mat4x4 get_projection() {
            return projMat;
        }
        Mat4x4 get_view() {
            return viewMat;
        }
        Vec3f get_direction() {
            Vec3f direction = Vec3f(cos(rotationX) * cos(rotationY),
                                    sin(rotationY),
                                    sin(rotationX) * cos(rotationY));
            return direction;
        }
    protected:
        float fov;
        float zNear, zFar;
        float width, height;
        bool perspective;
        
        Mat4x4 viewMat;
        Mat4x4 projMat;
        Mat4x4 combined;
};
class CameraControls {
    public:
        CameraControls(Camera *camera) {
            this->camera = camera;
        }
        void handle_event(SDL_Event ev, float timeTook) {
            int w = 0, h = 0;
            SDL_GetWindowSize(windows, &w, &h);
            Vec2i click = this->get_mouse_position(ev);
           
            if (ev.type == SDL_MOUSEMOTION) {       
                float sensitivity = 0.1f;
               
                if (click.y < h / 2) {
                    camera->rotationX -= ev.motion.xrel * sensitivity * timeTook;
                    camera->rotationY -= ev.motion.yrel * sensitivity * timeTook;
                }
               
                if (camera->rotationX > 2 * M_PI) camera->rotationX = 0;
                if (camera->rotationX < 0) camera->rotationX = 2 * M_PI;
               
                if (camera->rotationY > (89.0f / 180.0f * M_PI)) camera->rotationY = (89.0f / 180.0f * M_PI);
                if (camera->rotationY < -(89.0f / 180.0f * M_PI)) camera->rotationY = -(89.0f / 180.0f * M_PI);
              
                float s = 5.0f;
                Vec3f vel = { cos(camera->rotationX) * cos(camera->rotationY),
                              sin(camera->rotationY),
                              sin(camera->rotationX) * cos(camera->rotationY) };
                Vec3f v = vel.mul(s * timeTook);
                
                if (click.y > h / 2 && click.x < w / 2) {
                    camera->position.add(v);
                }
                else if (click.y > h / 2 && click.x > w / 2) {
                    Vec3f back = v.mul(-1);
                    camera->position.add(back);
                }
            }
        }
        Vec2i get_mouse_position(SDL_Event event) {
            int dx = 0, dy = 0;
            SDL_GetMouseState(&dx, &dy);
            Vec2i result = { dx, dy };
           
            return result;
        }
    private:
        Camera *camera;
};

class Shader {
    public:
       const char *vertexFile, *fragmentFile;
       
       Shader(const char *vertexFile, const char *fragmentFile) {
            this->vertexFile = vertexFile;
            this->fragmentFile = fragmentFile;
            
            std::string vertContent, fragContent;
               
            std::ifstream vert;
            std::ifstream frag;
            vert.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            frag.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            try {
                vert.open(vertexFile);
                frag.open(fragmentFile);
                
                std::stringstream stringVert, stringFrag;
                stringVert << vert.rdbuf();
                stringFrag << frag.rdbuf();
                
                vert.close();
                frag.close();
                  
                vertContent = stringVert.str();
                fragContent = stringFrag.str();
             } catch (std::ifstream::failure except) {
                printf("Couldn't open the shader file: %s\n", except.what());
             }
             
             
             this->vertex = vertContent.c_str();
             this->fragment = fragContent.c_str();
             
             
             this->load(this->vertex, this->fragment);
       }
       
       void load(const char *vertSource, const char *fragSource) {
            int check;
            char log[512];
            
            
            GLuint vert = glCreateShader(GL_VERTEX_SHADER);
        	glShaderSource(vert, 1, &vertSource, NULL);
            glCompileShader(vert);
               
            glGetShaderiv(vert, GL_COMPILE_STATUS, &check); 
            if (!check) {
                 glGetShaderInfoLog(vert, 512, NULL, log);
                 printf("%s: %s\n", this->vertexFile, log);
            }
            
            
            GLuint fragm = glCreateShader(GL_FRAGMENT_SHADER);
        	glShaderSource(fragm, 1, &fragSource, NULL);
            glCompileShader(fragm);
             
            glGetShaderiv(fragm, GL_COMPILE_STATUS, &check); 
            if (!check) {
                glGetShaderInfoLog(fragm, 512, NULL, log);
                printf("%s: %s\n", this->fragmentFile, log);
            }
               
            this->program = glCreateProgram();
            glAttachShader(this->program, vert);
            glAttachShader(this->program, fragm);
            glLinkProgram(this->program);
               
            glGetProgramiv(this->program, GL_LINK_STATUS, &check);
            if (!check) {
                glGetProgramInfoLog(this->program, 512, NULL, log);
                printf("%s\n", log);
            }
            glDeleteShader(vert);
            glDeleteShader(fragm);
       }
       void use() {
           glUseProgram(this->program);
       }
       void clear() {
           glDeleteProgram(this->program);
       }
       GLint attribute_location(const char *name) {
           return glGetAttribLocation(program, name);
       }
       GLint uniform_location(const char *name) {
           return glGetUniformLocation(program, name);
       }
       GLuint get_program() {
           return program;
       }
       
       void set_uniform_int(const char *name, int value) {
           glUniform1i(this->uniform_location(name), value);
       }
       void set_uniform_bool(const char *name, bool value) {
           glUniform1i(this->uniform_location(name), (int)value);
       }
       void set_uniform_float(const char *name, float value) {
           glUniform1f(this->uniform_location(name), value);
       }
       void set_uniform_vec2f(const char *name, float x, float y) {
           glUniform2f(this->uniform_location(name), x, y);
       }
       void set_uniform_vec3f(const char *name, float x, float y, float z) {
           glUniform3f(this->uniform_location(name), x, y, z);
       }
       void set_uniform_vec4f(const char *name, float x, float y, float z, float t) {
           glUniform4f(this->uniform_location(name), x, y, z, t);
       }
       void set_uniform_mat4(const char *name, const Mat4x4 &input) {
           glUniformMatrix4fv(this->uniform_location(name), 1, GL_FALSE, input.values);
       }
       
       void set_uniform_vec3f(const char *name, const Vec3f &input) {
           set_uniform_vec3f(name, input.x, input.y, input.z);
       }
       
    protected:
       const char *vertex;
       const char *fragment;
       
       GLuint program;
};

struct RenderVertex {
    Vec3f Position;
    Vec3f Color;
    Vec3f Normal;
    
    RenderVertex() {}
    RenderVertex(Vec3f position, Vec3f color = Vec3f(1.0f, 1.0f, 1.0f), Vec3f normal = Vec3f(0.0f, 0.0f, 0.0f)) : Position(position), Color(color), Normal(normal) {}
    
    RenderVertex(float x, float y, float z) : Position(x, y, z), Color(1.0f, 1.0f, 1.0f) {}
    RenderVertex(float x, float y, float z, float norX, float norY, float norZ) : Position(x, y, z), Normal(norX, norY, norZ) {}
};
struct TextureVertex {
    Vec3f Position;
    Vec3f Color = Vec3f(1.0f, 1.0f, 1.0f);
    Vec2f TextureCoords = Vec2f(0.0f, 0.0f);
    
    TextureVertex() {}
    TextureVertex(Vec3f position, Vec3f color, Vec2f textureCoords) : Position(position), Color(color), TextureCoords(textureCoords) {}
    
    TextureVertex(float x, float y, float z) : Position(x, y, z) {}
    TextureVertex(float x, float y, float z, float tx, float ty) : Position(x, y, z), TextureCoords(tx, ty) {}
    TextureVertex(float x, float y, float z, float tx, float ty, Vec3f color) : Position(x, y, z), Color(color), TextureCoords(tx, ty) {}
};
using RenderVertices = std::vector<RenderVertex>;
using TextureVertices = std::vector<TextureVertex>;

using RenderIndices = std::vector<uint>;

struct CharacterInfo {
    float advX, advY;
    
    float bitmapWidth, bitmapHeight;
    float bitmapLeft, bitmapTop;
    
    float offsetX = 0.0f;
};
struct AtlasEntry {
    float x, y;
    float width, height;
    
    float offsetX;
};

class TextAtlas {
     public:
         TextAtlas(FT_Face font) {
              this->font = font;
              this->glyph = font->glyph;
              
              this->adjust_size();
         }
         void adjust_size() {
              int width = 0;
              int height = 0;
              
              // Load ASCII characters
              for (int i = 32; i < 128; i++) {
                   if (FT_Load_Char(font, i, FT_LOAD_RENDER)) {
                        printf("Couldn't load character %c.\n", i);
                        continue;
                   }
                   
                   width += glyph->bitmap.width;
                   if (glyph->bitmap.rows > height) {
                        height = glyph->bitmap.rows;
                   }
              }
              this->atlasWidth = width;
              this->atlasHeight = height;
         }
         
         void load() {
              this->add_empty_texture();
              
              int x = 0;
              for (int i = 32; i < 128; i++) {
                   if (FT_Load_Char(font, i, FT_LOAD_RENDER)) {
                        continue;
                   }
                   glTexSubImage2D(GL_TEXTURE_2D, 0, x, 0, glyph->bitmap.width, glyph->bitmap.rows, GL_RED, GL_UNSIGNED_BYTE, glyph->bitmap.buffer);
                   
                   CharacterInfo ch;
                   ch.advX = glyph->advance.x >> 6;
                   ch.advY = glyph->advance.y >> 6;
                   
                   ch.bitmapWidth = glyph->bitmap.width;
                   ch.bitmapHeight = glyph->bitmap.rows;
                   
                   ch.bitmapLeft = glyph->bitmap_left;
                   ch.bitmapTop = glyph->bitmap_top;
                   
                   ch.offsetX = (float) x / atlasWidth;
                   
                   characters[i] = ch;
                   
                   x += glyph->bitmap.width;
              }
         }
         void use() {
              glActiveTexture(GL_TEXTURE0);
              glBindTexture(GL_TEXTURE_2D, this->textureIndex);
         }
         
         void dispose() {
             glDeleteTextures(1, &textureIndex);
             FT_Done_Face(font);
         }
         
         std::array<CharacterInfo, 128> &get_characters() { return characters; }
         float get_width() { return atlasWidth; }
         float get_height() { return atlasHeight; }
         
     private:
         void add_empty_texture() {
              glActiveTexture(GL_TEXTURE0);
              glGenTextures(1, &textureIndex);
              
              glBindTexture(GL_TEXTURE_2D, textureIndex);
              glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
              
              glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlasWidth, atlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, 0);
              
              
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                  
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
         }
     private:
         FT_Face font;
         FT_GlyphSlot glyph;
         GLuint textureIndex;
         
         int atlasWidth, atlasHeight;
         std::array<CharacterInfo, 128> characters;
};
class SpriteAtlas {
     public:
         SpriteAtlas() {
              this->lastX = 0.0f;
              this->atlasWidth = 2048.0f;
              this->atlasHeight = 32.0f;
              this->initialize();
         }
         
         void use() {
              glActiveTexture(GL_TEXTURE0);
              glBindTexture(GL_TEXTURE_2D, this->textureIndex);
         }
         
         
         
         AtlasEntry &get_entry(const char *location) {
              return entries[location];
         }
         void add_entry(const char *location, const char *fileName) {
              SDL_Surface *surface = load_surface(fileName);
              if (surface == NULL) {
                   return;
              }
              int width = surface->w;
              int height = surface->h;
              
              AtlasEntry entry;
              entry.x = this->lastX;
              entry.y = 0;
              entry.width = width;
              entry.height = height;
              if (atlasWidth) {
                   entry.offsetX = (float) this->lastX / atlasWidth;
              }  
              entries[location] = entry;
              
              // Compute size 
              int h = 0;
              for (auto &entry : entries) {
                   AtlasEntry second = entry.second;
                   if (second.height > h) {
                        h = second.height;
                   }
              }
              glTexSubImage2D(GL_TEXTURE_2D, 0, lastX, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);

              this->lastX += width;
         }
         void dispose() {
              glDeleteTextures(1, &textureIndex);
         }
         float get_width() { return atlasWidth; }
         float get_height() { return atlasHeight; }
         
     private:
         void initialize() {
              glActiveTexture(GL_TEXTURE0);
              glGenTextures(1, &textureIndex);
              
              glBindTexture(GL_TEXTURE_2D, textureIndex);
              
              glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, atlasWidth, atlasHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
              
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                  
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
         }
     private:
         GLuint textureIndex;
         
         int atlasWidth, atlasHeight;
         int lastX;
         std::map<const char*, AtlasEntry> entries;
};

class FreeType {
     public:
         static FreeType& get() {
              static FreeType ins;
              return ins;
         }
         
         void load() {
              errorHandler = FT_Init_FreeType(&library);
              if (errorHandler) {
                   throw std::runtime_error("FreeType library couldn't load.");
              }
              add_atlas("roboto", "/system/fonts/Roboto-Regular.ttf", 48);
         }
         
         void add_atlas(const char *name, const char *fileName, int height) {
              FT_Face font;
              
              errorHandler = FT_New_Face(library, fileName, 0, &font);
              if (errorHandler == FT_Err_Unknown_File_Format) {
                  throw std::runtime_error("The font file has an unknown format."); 
              } else if (errorHandler) {
                  throw std::runtime_error("Other error that occured when loading font.");
              }
              
              FT_Set_Pixel_Sizes(font, 0, height);
              TextAtlas *atlas = new TextAtlas(font);
              atlas->load();
              
              atlases[name] = atlas;
         }
         TextAtlas *find_atlas(const char *name) {
              return atlases[name];
         }
         
         void dispose() {
              for (auto &atlas : atlases) {
                   TextAtlas *second = atlas.second;
                   second->dispose();
              }
              FT_Done_FreeType(library);
         }
         
     private:
        FreeType() {}
        ~FreeType() {}
     public:
        FreeType(FreeType const&) = delete;
        void operator = (FreeType const&) = delete;
        
     private:
         FT_Library library;
         FT_Error errorHandler;
         std::map<const char*, TextAtlas*> atlases;
};


class TextureArray {
    public:
       TextureArray() {
           texturesUsed = 0;
           textureIndex = 0;
       }
       void setup(GLsizei textureSize, GLsizei numberOfTextures) {
           this->textureSize = textureSize;
           this->numberOfTextures = numberOfTextures;
           
           glGenTextures(1, &this->textureIndex);
           glBindTexture(GL_TEXTURE_2D_ARRAY, this->textureIndex);
           glActiveTexture(GL_TEXTURE0);
           
           glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, textureSize, textureSize, numberOfTextures, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
           
           glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
           glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
                                     
           glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);     
           glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
       }
       
       void add_texture(std::string fileName) {
           
           SDL_Surface *surface = load_surface(fileName.c_str());
           if (surface != NULL) {
               this->flip_vertically(surface);
               glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, texturesUsed, textureSize, textureSize, 1, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
               glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
           
               glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);     
               glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_LOD_BIAS, -1);
               texturesUsed++;
               
               printf("Texture added.\n");
           }
           else {
               printf("Counldn't add texture to array!\n");
           }
       }
       void flip_vertically(SDL_Surface *source) {
           SDL_LockSurface(source);
           int width = source->pitch;
           int height = source->h;
            
           char *buffer = new char[width];
           char *data = (char*)(source->pixels);
           
           for (int y = 0; y < height / 2; y++) {
                char *r1 = data + y * width,
                     *r2 = data + (height - y - 1) * width;
                      
                memcpy(buffer, r1, width);
                memcpy(r1, r2, width);
                memcpy(r2, buffer, width);
           }
           delete[] buffer;
           
           SDL_UnlockSurface(source);
       }
       void use() { 
           glActiveTexture(GL_TEXTURE0);
           glBindTexture(GL_TEXTURE_2D_ARRAY, this->textureIndex);
       }
       void clear() {
           glDeleteTextures(1, &this->textureIndex);
       }
    protected:
       GLuint textureIndex;
       GLuint texturesUsed;
       
       GLsizei numberOfTextures;
       // Usually 16
       GLsizei textureSize;
};

struct BatchType {
    GLenum renderType;
};

class Batch {
    public:
       BatchType type;
       Batch(int capacity, GLenum renderType, Shader *shader) {
           this->vertexCapacity = capacity;
           this->verticesUsed = 0;
           this->vbo = this->vao = 0;
         
           this->type.renderType = renderType;
           this->shader = shader;
           
           setup();
       }
       
       void setup() {
           glGenVertexArrays(1, &this->vao);
           glBindVertexArray(this->vao);
           
           glGenBuffers(1, &this->vbo);
           glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
           glBufferData(GL_ARRAY_BUFFER, this->vertexCapacity * sizeof(RenderVertex), nullptr, GL_STREAM_DRAW); 
           
           GLint position = this->shader->attribute_location("position");
           glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, sizeof(RenderVertex), (void*) offsetof(RenderVertex, Position));
           glEnableVertexAttribArray(position);
           
           GLint color = this->shader->attribute_location("color");
           glVertexAttribPointer(color, 3, GL_FLOAT, GL_FALSE, sizeof(RenderVertex), (void*) offsetof(RenderVertex, Color));
           glEnableVertexAttribArray(color);
           
           GLint normal = this->shader->attribute_location("normal");
           glVertexAttribPointer(normal, 3, GL_FLOAT, GL_FALSE, sizeof(RenderVertex), (void*) offsetof(RenderVertex, Normal));
           glEnableVertexAttribArray(normal);
           
           glBindVertexArray(0);
           glDisableVertexAttribArray(position);
           glDisableVertexAttribArray(color);
           glDisableVertexAttribArray(normal);
           glBindBuffer(GL_ARRAY_BUFFER, 0);
       }
       
       void add(RenderVertices vertices) {
           int extra = this->get_extra_vertices();
           if (vertices.size() + extra > vertexCapacity - verticesUsed) {
               return;
           }
           if (vertices.empty()) {
               return;
           }
           if (vertices.size() > vertexCapacity) {
               return;
           }
           
           glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
           if (extra > 0) {
               glBufferSubData(GL_ARRAY_BUFFER, (verticesUsed + 0) * sizeof(RenderVertex), sizeof(RenderVertex), &lastUsed);
               glBufferSubData(GL_ARRAY_BUFFER, (verticesUsed + 1) * sizeof(RenderVertex), sizeof(RenderVertex), &vertices[0]);
           }
           glBufferSubData(GL_ARRAY_BUFFER, verticesUsed * sizeof(RenderVertex), vertices.size() * sizeof(RenderVertex), &vertices[0]);
            
           glBindBuffer(GL_ARRAY_BUFFER, 0);
           verticesUsed += vertices.size() + extra;
           lastUsed = vertices.back();
       }
       
       void render() {
           if (verticesUsed == 0) {
               return;
           }
           glBindVertexArray(this->vao);
           glDrawArrays(this->type.renderType, 0, verticesUsed);
           
           this->verticesUsed = 0;
       }
       
       
       int get_extra_vertices() {
           bool mode = (this->type.renderType == GL_TRIANGLE_STRIP && verticesUsed > 0);
           return mode ? 2 : 0;
       }
       void dispose() {
           if (this->vbo) {
               glDeleteBuffers(1, &this->vbo);
           }
           if (this->vao) {
               glDeleteBuffers(1, &this->vao);
           }
       }
    protected:
       int vertexCapacity;
       int verticesUsed;
       RenderVertex lastUsed;
       
       GLuint vbo;
       GLuint vao;
       Shader *shader;
};

class TextureBatch {
    public:
       BatchType type;
       TextureBatch(int capacity, GLenum renderType, Shader *shader) {
           this->vertexCapacity = capacity;
           this->verticesUsed = 0;
           this->vbo = this->vao = 0;
         
           this->type.renderType = renderType;
           this->shader = shader;
           
           setup();
       }
       
       void setup() {
           glGenVertexArrays(1, &this->vao);
           glBindVertexArray(this->vao);
           
           glGenBuffers(1, &this->vbo);
           glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
           glBufferData(GL_ARRAY_BUFFER, this->vertexCapacity * sizeof(TextureVertex), nullptr, GL_STREAM_DRAW); 
           
           GLint position = this->shader->attribute_location("position");
           glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, sizeof(TextureVertex), (void*) offsetof(TextureVertex, Position));
           glEnableVertexAttribArray(position);
           
           GLint color = this->shader->attribute_location("color");
           glVertexAttribPointer(color, 3, GL_FLOAT, GL_FALSE, sizeof(TextureVertex), (void*) offsetof(TextureVertex, Color));
           glEnableVertexAttribArray(color);
           
           GLint textureCoords = this->shader->attribute_location("textureCoords");
           glVertexAttribPointer(textureCoords, 2, GL_FLOAT, GL_FALSE, sizeof(TextureVertex), (void*) offsetof(TextureVertex, TextureCoords));
           glEnableVertexAttribArray(textureCoords);
           
           glBindVertexArray(0);
           glDisableVertexAttribArray(position);
           glDisableVertexAttribArray(color);
           glDisableVertexAttribArray(textureCoords);
           glBindBuffer(GL_ARRAY_BUFFER, 0);
       }
       
       void add(TextureVertices vertices) {
           int extra = this->get_extra_vertices();
           if (vertices.size() + extra > vertexCapacity - verticesUsed) {
               return;
           }
           if (vertices.empty()) {
               return;
           }
           if (vertices.size() > vertexCapacity) {
               return;
           }
           
           glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
           if (extra > 0) {
               glBufferSubData(GL_ARRAY_BUFFER, (verticesUsed + 0) * sizeof(TextureVertex), sizeof(TextureVertex), &lastUsed);
               glBufferSubData(GL_ARRAY_BUFFER, (verticesUsed + 1) * sizeof(TextureVertex), sizeof(TextureVertex), &vertices[0]);
           }
           glBufferSubData(GL_ARRAY_BUFFER, verticesUsed * sizeof(TextureVertex), vertices.size() * sizeof(TextureVertex), &vertices[0]);
            
           glBindBuffer(GL_ARRAY_BUFFER, 0);
           verticesUsed += vertices.size() + extra;
           lastUsed = vertices.back();
       }
       
       void render() {
           if (verticesUsed == 0) {
               return;
           }
           glBindVertexArray(this->vao);
           glDrawArrays(this->type.renderType, 0, verticesUsed);
           
           this->verticesUsed = 0;
       }
       
       
       int get_extra_vertices() {
           bool mode = (this->type.renderType == GL_TRIANGLE_STRIP && verticesUsed > 0);
           return mode ? 2 : 0;
       }
       void dispose() {
           if (this->vbo) {
               glDeleteBuffers(1, &this->vbo);
           }
           if (this->vao) {
               glDeleteBuffers(1, &this->vao);
           }
       }
    protected:
       int vertexCapacity;
       int verticesUsed;
       TextureVertex lastUsed;
       
       GLuint vbo;
       GLuint vao;
       Shader *shader;
};

namespace Renderer {
     TextAtlas *textAtlas;
     SpriteAtlas *atlas;
     
     TextureBatch *textBatch;
     TextureBatch *uiBatch;
     Shader *overlayShader;
     
     void load() {
           overlayShader = new Shader("overlay.vert", "overlay.frag"); 
           textBatch = new TextureBatch(4096, GL_TRIANGLES, overlayShader);
           uiBatch = new TextureBatch(1000, GL_TRIANGLES, overlayShader);
           
           textAtlas = FreeType::get().find_atlas("roboto");
           atlas = new SpriteAtlas();
           atlas->add_entry("table-background", "table_background.png");
           atlas->add_entry("button-background", "button_background.png");
     
           atlas->add_entry("checkbox-on", "checkbox_on.png");
           atlas->add_entry("checkbox-off", "checkbox_off.png");
     }
     
     void draw_string(const std::string &text, float x, float y, float sclX, float sclY, const Vec3f &color) {
           TextureVertices vertices;
              
           float px = x;
           float py = y;
           std::string::const_iterator iterator;
           for (iterator = text.begin(); iterator != text.end(); iterator++) {
                CharacterInfo ch = textAtlas->get_characters().at(*iterator);
                   
                float x2 = px + ch.bitmapLeft * sclX;
                float y2 = -py - ch.bitmapTop * sclY;
                float width = ch.bitmapWidth * sclX;
                float height = ch.bitmapHeight * sclY;
                   
                px += ch.advX * sclX;
                py += ch.advY * sclY;
                   
                   
                vertices.push_back(TextureVertex(x2, -y2, -1, ch.offsetX, 0, color));
                vertices.push_back(TextureVertex(x2 + width, -y2, -1, ch.offsetX + ch.bitmapWidth / textAtlas->get_width(), 0, color));
                vertices.push_back(TextureVertex(x2, -y2 - height, -1, ch.offsetX, ch.bitmapHeight / textAtlas->get_height(), color));
                   
                vertices.push_back(TextureVertex(x2 + width, -y2, -1, ch.offsetX + ch.bitmapWidth / textAtlas->get_width(), 0, color));
                vertices.push_back(TextureVertex(x2 + width, -y2 - height, -1, ch.offsetX + ch.bitmapWidth / textAtlas->get_width(), ch.bitmapHeight / textAtlas->get_height(), color));
                vertices.push_back(TextureVertex(x2, -y2 - height, -1, ch.offsetX, ch.bitmapHeight / textAtlas->get_height(), color));
           }
              
           textBatch->add(vertices);
     }
     void draw_string(const std::string &text, float x, float y, float sclX, float sclY) {
           draw_string(text, x, y, sclX, sclY, Vec3f(1.0f, 1.0f, 1.0f));
     }
     void draw_string_centered(const std::string &text, float x, float y, float sclX, float sclY, const Vec3f &color) {
           TextureVertices vertices;
              
           float w = 0.0f;
           float h = 0.0f;
           std::string::const_iterator iterator;
           for (iterator = text.begin(); iterator != text.end(); iterator++) {
                CharacterInfo ch = Renderer::textAtlas->get_characters().at(*iterator);
                
                w += ch.advX;
                
                if (ch.bitmapHeight > h) {
                    h = ch.bitmapHeight;
                }
           }
           if (!text.empty()) {
                CharacterInfo ch = Renderer::textAtlas->get_characters().at(*text.end());
                w -= (ch.advX - (ch.bitmapLeft + ch.bitmapWidth));
           }
           
           w *= sclX;
           h *= sclY;
           
           
           float px = x - w / 2.0f;
           float py = y - h / 2.0f;
           for (iterator = text.begin(); iterator != text.end(); iterator++) {
                CharacterInfo ch = textAtlas->get_characters().at(*iterator);
                   
                float x2 = px + ch.bitmapLeft * sclX;
                float y2 = -py - ch.bitmapTop * sclY;
                float width = ch.bitmapWidth * sclX;
                float height = ch.bitmapHeight * sclY;
                   
                px += ch.advX * sclX;
                py += ch.advY * sclY;
                   
                   
                vertices.push_back(TextureVertex(x2, -y2, -1, ch.offsetX, 0, color));
                vertices.push_back(TextureVertex(x2 + width, -y2, -1, ch.offsetX + ch.bitmapWidth / textAtlas->get_width(), 0, color));
                vertices.push_back(TextureVertex(x2, -y2 - height, -1, ch.offsetX, ch.bitmapHeight / textAtlas->get_height(), color));
                   
                vertices.push_back(TextureVertex(x2 + width, -y2, -1, ch.offsetX + ch.bitmapWidth / textAtlas->get_width(), 0, color));
                vertices.push_back(TextureVertex(x2 + width, -y2 - height, -1, ch.offsetX + ch.bitmapWidth / textAtlas->get_width(), ch.bitmapHeight / textAtlas->get_height(), color));
                vertices.push_back(TextureVertex(x2, -y2 - height, -1, ch.offsetX, ch.bitmapHeight / textAtlas->get_height(), color));
           }
              
           textBatch->add(vertices);
     }
     void draw_string_centered(const std::string &text, float x, float y, float sclX, float sclY) {
           draw_string_centered(text, x, y, sclX, sclY, Vec3f(1.0f, 1.0f, 1.0f));
     }
     void draw_rectangle(const char *name, float x, float y, float width, float height, const Vec3f &color) {
           AtlasEntry entry = atlas->get_entry(name);
           TextureVertices vertices;
           
           float x1 = x - width / 2.0f;
           float x2 = x + width / 2.0f;
           
           float y1 = y - height / 2.0f;
           float y2 = y + height / 2.0f;
           
           float w = atlas->get_width();
           float h = atlas->get_height();
           
           vertices.push_back(TextureVertex(x1, y1, -1, entry.offsetX, 0, color));
           vertices.push_back(TextureVertex(x2, y1, -1, entry.offsetX + entry.width / w, 0, color));
           vertices.push_back(TextureVertex(x1, y2, -1, entry.offsetX, -entry.height / h, color));
           
           vertices.push_back(TextureVertex(x2, y1, -1, entry.offsetX + entry.width / w, 0, color));
           vertices.push_back(TextureVertex(x2, y2, -1, entry.offsetX + entry.width / w, -entry.height / h, color));
           vertices.push_back(TextureVertex(x1, y2, -1, entry.offsetX, -entry.height / h, color));
           
           uiBatch->add(vertices);
     }
     
     void dispose() {
           overlayShader->clear();
           textBatch->dispose();
           uiBatch->dispose();
           
           atlas->dispose();
           //textAtlas->dispose();
     }
};

enum class TextFieldFilters {
     integers,
     floats,
     
     characters
};

namespace UI {
};
static class Table;

namespace UIPallete {
     Vec3f textColor;

     Vec3f buttonHovered;
     Vec3f checkboxHovered;
     
     Vec3f tableBackground;
     Vec3f buttonBackground;
     Vec3f textFieldBackground;
     Vec3f checkboxButton;
     void load() {
          textColor = Vec3f(1.0f, 1.0f, 1.0f);
          
          buttonHovered = Vec3f(0.8f, 0.8f, 0.8f);
          checkboxHovered = Vec3f(0.0f, 1.0f, 0.0f);
          
          tableBackground = Vec3f(0.3f, 0.3f, 0.3f);
          buttonBackground = Vec3f(0.5f, 0.5f, 0.5f);
          textFieldBackground = Vec3f(0.15f, 0.15f, 0.15f);
          checkboxButton = Vec3f(0.85f, 0.85f, 0.85f);
     }
};

class UIObject {
     public:
         virtual void render() {}
         virtual void mouse_down(float mx, float my) {}
         virtual void mouse_up(float mx, float my) {}
         virtual void mouse_enter() {}
         virtual void mouse_exit() {}
         
         virtual bool can_focus(float mx, float my) { return false; }
};

class Cell : public UIObject {
     public:
         Vec2f position;
         float width, height;
         float paddingX;
         float paddingY;
         Table *table;
         
         Cell() {
              this->reset();
         }
         void reset() {
              width = height = 0.0f;
              paddingX = 5.0f;
              paddingY = 5.0f;
        
              visible = true;
              this->table = nullptr;
         }
         
         Cell *set_size(float w, float h) {
              width = w;
              height = h;
              
              return this;
         }
         Cell *set_position(float x, float y) {
              this->position = Vec2f(x, y);
              
              return this;
         }
         Cell *set_position_centered(float x, float y) {
              this->position = Vec2f(x - width / 2.0f, y - height / 2.0f);
              
              return this;
         }
         Cell *set_paddingX(float x) {
              this->paddingX = x;
              
              return this;
         }
         Cell *set_paddingY(float y) {
              this->paddingY = y;
              
              return this;
         }
         Cell *update(std::function<void()> to) {
              this->updateListener = to;
              
              return this;
         }
         std::function<void()> &get_update_listener() { return this->updateListener; }
         
         Cell *set_visibility(bool to) {
              this->visible = to;
              
              return this;
         }
         bool is_visible() { return this->visible; }
         
         bool inside_rectangle(float pointX, float pointY, float x1, float y1, float x2, float y2) {
              bool inside = (pointX >= x1 && pointY >= y1) && 
                            (pointX < x2 && pointY < y2);
              return inside;
         }
         bool point_inside(float x, float y) {
              return this->inside_rectangle(x, y, position.x - width, position.y - height, position.x + width, position.y + height);
         }
     private:
         bool visible;
         std::function<void()> updateListener;
};

class Label : public Cell {
     public:
         Label() : Cell() {
              scaling = Vec2f(0.4f, 0.4f);
              color = UIPallete::textColor;
              
              this->compute_size();
         }
         Label(std::string text) : Cell() {
              this->text = text;
              scaling = Vec2f(0.4f, 0.4f);
              color = UIPallete::textColor;
              
              this->compute_size();
         }
         
         
         void render() override {
              Renderer::draw_string_centered(text, position.x, position.y, scaling.x, scaling.y, color);
         }
         
         void compute_size() {
              float w = 0.0f, h = 0.0f;
              
              std::string::const_iterator iterator;
              for (iterator = text.begin(); iterator != text.end(); iterator++) {
                   CharacterInfo ch = Renderer::textAtlas->get_characters().at(*iterator);
                
                   w += ch.advX;
                
                   if (ch.bitmapHeight > h) {
                        h = ch.bitmapHeight;
                   }
              }
              if (!text.empty()) {
                   CharacterInfo ch = Renderer::textAtlas->get_characters().at(*text.end());
                   w -= (ch.advX - (ch.bitmapLeft + ch.bitmapWidth));
              }
              
              this->width = w * scaling.x;
              this->height = h * scaling.y;
         }
         
         Label *set_text(const std::string &to) {
              this->text = to;
              this->compute_size();
              
              return this;
         }
         Label *set_scaling(float scale) {
              this->scaling = Vec2f(scale, scale);
              return this;
         }
         
         Label *set_color(float r, float g, float b) {
              this->color = Vec3f(r, g, b);
              return this;
         }
         std::string &get_text() { return text; }
     protected:
         Vec2f scaling;
         Vec3f color;
         
         std::string text;
};

class CheckBox : public Cell {
     public:
         CheckBox(std::string label, bool checked) : Cell() {
              this->label = label;
              this->checked = checked;
              
              width = height = 25.0f;
         }
         CheckBox(std::string label, bool checked, std::function<void(bool)> clicked) : Cell() {
              this->label = label;
              this->checked = checked;
              this->clickListener = clicked;
              
              width = height = 25.0f;
         }
         
         void render() override {
              if (hovered) {
                   Renderer::draw_rectangle(this->checked ? "checkbox-on" : "checkbox-off", position.x, position.y, width, height, UIPallete::checkboxHovered);
              } else {
                   Renderer::draw_rectangle(this->checked ? "checkbox-on" : "checkbox-off", position.x, position.y, width, height, UIPallete::checkboxButton);
              }
              Renderer::draw_string(label, position.x + 17.0f, position.y - 7.5f, 0.4f, 0.4f, UIPallete::textColor);
         }
         void mouse_up(float mx, float my) override {
              bool contains = this->point_inside(mx, my);
              if (contains) {
                   checked = !checked;
                   
                   if (clickListener != NULL) clickListener(this->checked);
              }
         }
         void mouse_enter() override {
              hovered = true;
         }
         void mouse_exit() override {
              hovered = false;
         }
         bool can_focus(float mx, float my) override {
              bool contains = this->point_inside(mx, my);
              
              return contains;
         }
     private:
         std::string label;
         bool checked;
         bool hovered;
         std::function<void(bool)> clickListener;
};

class Button : public Cell {
     public:
         Button(std::string label) : Cell() {
              this->label = label;
              
              width = 80.0f;
              height = 25.0f;
         }
         Button(std::string label, std::function<void()> clicked) : Cell() {
              this->label = label;
              this->clickListener = clicked;
              
              width = 80.0f;
              height = 25.0f;
         }
         
         void render() override {
              Renderer::draw_rectangle("button-background", position.x, position.y, width, height, hovered ? UIPallete::buttonHovered : UIPallete::buttonBackground);
              
              Renderer::draw_string_centered(label, position.x, position.y, 0.4f, 0.4f, UIPallete::textColor);
         }
         void mouse_up(float mx, float my) override {
              bool contains = this->point_inside(mx, my);
              if (contains) {
                   if (clickListener != NULL) clickListener();
              }
         }
         void mouse_enter() override {
              hovered = true;
         }
         void mouse_exit() override {
              hovered = false;
         }
         bool can_focus(float mx, float my) override {
              bool contains = this->point_inside(mx, my);
              
              return contains;
         }
         Button *set_label(const std::string &to) {
              this->label = to;
              
              return this;
         }
     private:
         std::string label;
         bool hovered;
         std::function<void()> clickListener;
};

class TextField : public Cell {
     public:
         TextField(std::string label, std::string text, TextFieldFilters filter) : Cell() {
              this->label = label;
              this->text = text;
              this->filter = filter;
              this->labelPadX = 10.0f;
         }
         TextField(std::string label, TextFieldFilters filter) : TextField(label, "", filter) {}
         
         
         void render() override {
              Renderer::draw_rectangle("button-background", position.x, position.y, width, height, focused ? UIPallete::buttonHovered : UIPallete::textFieldBackground);
              Renderer::draw_string_centered(text, position.x, position.y, 0.2f, 0.2f);
              
              Renderer::draw_string_centered(label, position.x - width / 2.0f - labelPadX, position.y, 0.2f, 0.2f);
         }
         
         void mouse_up(float mx, float my) override {
              bool contains = this->point_inside(mx, my);
              if (contains) {
                   SDL_StartTextInput();
                   set_focused(true);
              }
         }
         void input_text(char input) {
              if (filter == TextFieldFilters::characters) {
                  this->text += input;
              } else if (filter == TextFieldFilters::integers) {
                  bool sign = input == '-' && this->text.find("-") == std::string::npos;
                  
                  if (isdigit(input) || sign) {
                       this->text += input;
                  }
              } else if (filter == TextFieldFilters::floats) {
                  bool isRealNumber = input == '.' && this->text.find(".") == std::string::npos;
                  bool sign = input == '-' && this->text.find("-") == std::string::npos;
                  
                  if (isdigit(input) || isRealNumber || sign) {
                       this->text += input;
                  }
              }
         }
         void input_key(SDL_KeyboardEvent *event) {
              if (event->keysym.scancode == SDL_SCANCODE_BACKSPACE && text.size()) {
                     text.pop_back();
              }
         }
         
         TextField *set_focused(bool to);
         bool is_focused() { return this->focused; }
         
         TextField *set_labelPaddingX(float x) {
              this->labelPadX = x;
              
              return this;
         }
         TextField *set_text(const std::string &to) {
              this->text = to;
         }
         std::string &get_text() { return this->text; }
     private:
         std::string label;
         std::string text;
         TextFieldFilters filter;
         
         bool focused;
         float labelPadX;
};
namespace UI {
     TextField *focusedTextField;
};
TextField *TextField::set_focused(bool to) {
      this->focused = to;
      if (to) {
          UI::focusedTextField = this;
      } else if (UI::focusedTextField == this) {
          UI::focusedTextField = nullptr;
      }
              
      return this;
};

class Table {
     public:
        Table(std::string label, float x, float y, float width, float height) {
             this->label = label;
             this->position = Vec2f(x, y);
             this->width = width;
             this->height = height;
             
             this->reset(); 
        }
        void reset() {
             this->labelPadY = 25.0f;
             this->componentOffsetX = 0.0f;
             this->componentOffsetY = this->labelPadY * 2.0f + 5.0f;
             this->rows = this->columns = 0;
             this->lastRowCount = 0;
             
             this->visible = true;
        }
        
        void update() {
             if (updateListener != NULL) updateListener();
             
             for (auto &object : objects) {
                  std::function<void()> listener = object->get_update_listener();
                  if (listener != NULL) {
                       listener();
                  }
             }
        }
        
        void render() {
             if (!visible) return;
             
             Renderer::draw_rectangle("table-background", position.x, position.y, width, height, UIPallete::tableBackground);
             Renderer::draw_rectangle("button-background", position.x, position.y + (height - labelPadY) / 2.0f, width, labelPadY, UIPallete::buttonHovered);
             Renderer::draw_string_centered(label, position.x, position.y + (height - labelPadY) / 2.0f, 0.4f, 0.4f, UIPallete::textColor);
         
             for (auto &object : objects) {
                  object->render();
             }
        }
        
        void add_object(Cell *object) {
             object->set_position(position.x + componentOffsetX / 2.0f, position.y + height / 2.0f - componentOffsetY);
             object->table = this;
             objects.push_back(object);
             
             for (int i = 0; i < columns; i++) {
                  Cell *object = this->objects.at(i * lastRowCount + rows - 1);
                  object->set_position(object->position.x - componentOffsetX / 2.0f, object->position.y);
             }
             componentOffsetY += object->paddingY + object->height;
             
             
             rows++;
        }
        void column(float paddingX) {
             if (this->objects.empty()) return;
             
             lastRowCount = rows;
             /*
             for (int i = 0; i < columns; i++) {
                  Cell *object = this->objects.at(i + (rows - 1) );
                  object->set_position(object->position.x - componentOffsetX / 2.0f, object->position.y);
             }
             */
             
             Cell *object = this->objects.at(lastRowCount * columns);
             componentOffsetX += paddingX;
             componentOffsetY = this->labelPadY * 2.0f + 5.0f;
             
             rows = 1;
             columns++;
        }
        
        void handle_event(SDL_Event event, float timeTook) {
             if (!visible) return;
             
             int cx, cy;
             SDL_GetMouseState(&cx, &cy);
             int w = 0, h = 0;
             SDL_GetWindowSize(windows, &w, &h);
             
             float mx = (float) cx, my = (float) cy; 
           
             // Normalized coordinates
             mx /= w;
             my /= h;
           
             mx *= SCREEN_WIDTH;
             my *= SCREEN_HEIGHT;
             mx -= SCREEN_WIDTH / 2.0f;
             my -= SCREEN_HEIGHT / 2.0f;
             my *= -1;
             
           
             if (event.type == SDL_MOUSEBUTTONDOWN) {
                  for (auto &object : objects) {
                       if (object->is_visible()) object->mouse_down(mx, my);
                  }
                  if (UI::focusedTextField != nullptr) {
                       UI::focusedTextField->set_focused(false);
                       SDL_StopTextInput();
                  }
             }
             if (event.type == SDL_MOUSEBUTTONUP) {
                  for (auto &object : objects) {
                       if (object->is_visible()) object->mouse_up(mx, my);
                  }
             }
             if (event.type == SDL_MOUSEMOTION) {
                  /*
                  for (auto &object : objects) {
                       if (object->is_visible() && object->can_focus(mx, my)) {
                            break;
                       }
                  }
                  */
                  for (auto &object : objects) {
                       if (!object->is_visible()) continue;
                     
                       if (object->point_inside(mx, my)) {
                           object->mouse_enter();
                       } else {
                           object->mouse_exit();
                       }
                  }
             }
             
             if (event.type == SDL_TEXTINPUT) {
                  if (UI::focusedTextField != nullptr && UI::focusedTextField->table == this) {
                       UI::focusedTextField->input_text(*event.text.text);
                  }
             }
             if (event.type == SDL_KEYDOWN) {
                  if (UI::focusedTextField != nullptr && UI::focusedTextField->table == this) {
                       UI::focusedTextField->input_key(&event.key);
                  }
             }
        }
        Table *update(std::function<void()> to) {
             this->updateListener = to;
              
             return this;
        }
        std::function<void()> &get_update_listener() { return this->updateListener; }
          
        Table *set_visibility(bool to) {
             this->visible = to;
              
             return this;
        }
        bool is_visible() { return this->visible; }
         
     private:
        Vec2f position;
        float width, height;
        float labelPadY;
        float componentOffsetX;
        float componentOffsetY;
        
        int rows;
        int columns;
        int lastRowCount;
        
        bool visible;
        std::function<void()> updateListener;
        
        std::vector<Cell*> objects;
        std::string label;
};

struct Mesh {
     RenderVertices renderVertices;
     RenderIndices indices;
     Mesh() {}
     Mesh(RenderVertices vertices, RenderIndices indices) : renderVertices(vertices), indices(indices) {}
     
     Mesh set_color(const Vec3f color) {
          for (auto &vertex : renderVertices) {
               vertex.Color = color;
          }
          
          return *this;
     }
     Mesh set_color(float r, float g, float b) {
          return set_color(Vec3f(r, g, b));
     }
     uint max_index() {
          uint result = 0;
          for (auto &index : indices) {
               if (index > result) result = index;
          }
          
          return result;
     }
};

// A batched object
class SceneObject {
     public:
        Vec3f position, scaling;
        
        SceneObject(Mesh mesh) {
             this->mesh = mesh;
             
             position = Vec3f(0.0f, 0.0f, 0.0f);
             scaling = Vec3f(1.0f, 1.0f, 1.0f);
             boundingBox = AABB(Vec3f(-0.5f, -0.5f, -0.5f), Vec3f(0.5f, 0.5f, 0.5f));
        }
        
        void render(Batch *batch) {
             if (mesh.indices.empty()) return;
             
             RenderVertices vertices;
             Vec3f minimum, maximum;
             minimum = maximum = Vec3f(mesh.renderVertices.at(0).Position);
             
             for (auto &index : mesh.indices) {
                  RenderVertex vertex = mesh.renderVertices.at(index);
                  
                  // X coordinates
                  if (vertex.Position.x < minimum.x) minimum.x = vertex.Position.x;
                  if (vertex.Position.x > maximum.x) maximum.x = vertex.Position.x;
              
                  // Y coordinates
                  if (vertex.Position.y < minimum.y) minimum.y = vertex.Position.y;
                  if (vertex.Position.y > maximum.y) maximum.y = vertex.Position.y;
                  
                  // Z coordinates
                  if (vertex.Position.z < minimum.z) minimum.z = vertex.Position.z;
                  if (vertex.Position.z > maximum.z) maximum.z = vertex.Position.z;
             }
             
             for (auto &index : mesh.indices) {
                  RenderVertex vertex = mesh.renderVertices.at(index);
                  
                  vertex.Position.mul(this->scaling);
                  vertex.Position.add(this->position);
                  
                  vertices.emplace_back(vertex);
             }
             this->boundingBox.min = minimum.mul(scaling).add(position);
             this->boundingBox.max = maximum.mul(scaling).add(position);
             
             batch->add(vertices);
        }
        Mesh &get_mesh() { return this->mesh; }
        AABB &get_bounding_box() { return this->boundingBox; }
        
        SceneObject *set_position(const Vec3f &to) {
             this->position = to;
            
             return this;
        }
        SceneObject *set_scaling(const Vec3f &to) {
             this->scaling = to;
            
             return this;
        }
        SceneObject *set_position(float x, float y, float z) {
             this->position = Vec3f(x, y, z);
            
             return this;
        }
        SceneObject *set_scaling(float width, float height, float depth) {
             this->scaling = Vec3f(width, height, depth);
            
             return this;
        }
     private:
        Mesh mesh;
        AABB boundingBox;
};


namespace BaseMeshes {
     const RenderVertices cubeVertices = {
         //           Position         normal
         RenderVertex(-0.5, -0.5, 0.5,  0.0, 0.0, 1.0),
         RenderVertex(0.5, -0.5, 0.5,  0.0, 0.0, 1.0),
         RenderVertex(0.5, 0.5, 0.5,  0.0, 0.0, 1.0),
         RenderVertex(-0.5, 0.5, 0.5,  0.0, 0.0, 1.0),
       
         RenderVertex(-0.5, 0.5, 0.5,  0.0, 1.0, 0.0),
         RenderVertex(0.5, 0.5, 0.5,  0.0, 1.0, 0.0),
         RenderVertex(0.5, 0.5, -0.5,  0.0, 1.0, 0.0),
         RenderVertex(-0.5, 0.5, -0.5,  0.0, 1.0, 0.0),
       
         RenderVertex(0.5, -0.5, -0.5,  0.0, 0.0, -1.0),
         RenderVertex(-0.5, -0.5, -0.5,  0.0, 0.0, -1.0),
         RenderVertex(-0.5, 0.5, -0.5,  0.0, 0.0, -1.0),
         RenderVertex(0.5, 0.5, -0.5,  0.0, 0.0, -1.0),
       
         RenderVertex(-0.5, -0.5, -0.5,  0.0, -1.0, 0.0),
         RenderVertex(0.5, -0.5, -0.5,  0.0, -1.0, 0.0),
         RenderVertex(0.5, -0.5, 0.5,  0.0, -1.0, 0.0),
         RenderVertex(-0.5, -0.5, 0.5, 0.0, -1.0, 0.0),
       
         RenderVertex(-0.5, -0.5, -0.5,  -1.0, 0.0, 0.0),
         RenderVertex(-0.5, -0.5, 0.5,  -1.0, 0.0, 0.0),
         RenderVertex(-0.5, 0.5, 0.5,  -1.0, 0.0, 0.0),
         RenderVertex(-0.5, 0.5, -0.5,  -1.0, 0.0, 0.0),
       
         RenderVertex(0.5, -0.5, 0.5,  1.0, 0.0, 0.0),
         RenderVertex(0.5, -0.5, -0.5,  1.0, 0.0, 0.0),
         RenderVertex(0.5, 0.5, -0.5,  1.0, 0.0, 0.0),  
         RenderVertex(0.5, 0.5, 0.5,  1.0, 0.0, 0.0)
     };
     const RenderIndices cubeIndices = {
         0, 1, 2,
         2, 3, 0,
               
         4, 5, 6,
         6, 7, 4,
               
         8, 9, 10,
         10, 11, 8,
               
         12, 13, 14,
         14, 15, 12,
               
         16, 17, 18,
         18, 19, 16,
               
         20, 21, 22,
         22, 23, 20
     };
     
     const Mesh cube = Mesh(cubeVertices, cubeIndices);
};

namespace TemporarySettings {
     bool displayGrid;
     void load() {
          displayGrid = true;
     }
};

class Scene {
     public:
        float offset = 0.0f;
        Scene() {
             objectShader = new Shader("model.vert", "model.frag");
             gridShader = new Shader("grid.vert", "grid.frag");
             axisShader = new Shader("grid.vert", "axis.frag");
             outlineShader = new Shader("grid.vert", "outline.frag");
             
             objectBatch = new Batch(4096, GL_TRIANGLES, objectShader);
             gridBatch = new Batch(1000, GL_LINES, gridShader);
             axisBatch = new Batch(1000, GL_LINES, axisShader);
             outlineBatch = new Batch(6 * 4, GL_LINES, outlineShader);
           
             this->xz = Plane(Vec3f(0.0f, 0.0f, 0.0f), Vec3f(0.0f, 1.0f, 0.0f));
             
             int gridWidth = 50, gridHeight = 50, gridDepth = 50;
             setup_grid(gridWidth, gridDepth);
             setup_axes(gridWidth, gridHeight, gridDepth);
             
             
             Mesh cube = BaseMeshes::cube;
             SceneObject *obj = new SceneObject(cube.set_color(0.8f, 0.8f, 0.8f));
             add_object(obj);
             this->selectedObject = nullptr;
        }
       
        void add_object(SceneObject *object) {
             objects.push_back(object);
        }
        void remove_object(SceneObject *object) {
             int index = this->object_index(object);
             if (index == -1) return;
             
             objects.erase(objects.begin() + index);
             delete object;
        }
        void update(float timeTook) {
             offset += timeTook;
        }
        void render(Camera *camera) {
             Mat4x4 model;
             
             // 1st pass - grid
             if (TemporarySettings::displayGrid) {
                  gridShader->use();
                  gridShader->set_uniform_mat4("model", model);
                  gridShader->set_uniform_mat4("view", camera->get_view());
                  gridShader->set_uniform_mat4("projection", camera->get_projection());
           
                  gridBatch->add(grid);
                  glLineWidth(2);
                  gridBatch->render();
             }
             
             // 2nd pass - coordinate axes
             axisShader->use();
             axisShader->set_uniform_mat4("model", model);
             axisShader->set_uniform_mat4("view", camera->get_view());
             axisShader->set_uniform_mat4("projection", camera->get_projection());
             
             axisBatch->add(axis);
             if (selectedObject != nullptr) {
                  this->draw_bounding_box(selectedObject);
             }
             glLineWidth(3);
             axisBatch->render();
             
             outlineShader->use();
             outlineShader->set_uniform_mat4("model", model);
             outlineShader->set_uniform_mat4("view", camera->get_view());
             outlineShader->set_uniform_mat4("projection", camera->get_projection());
             outlineShader->set_uniform_float("uTime", offset);
             
             if (selectedObject != nullptr) {
                  this->draw_bounding_box(selectedObject);
             }
             outlineBatch->render();
             
             // 3rd pass - model
             objectShader->use();
             objectShader->set_uniform_mat4("model", model);
             objectShader->set_uniform_mat4("view", camera->get_view());
             objectShader->set_uniform_mat4("projection", camera->get_projection());
             
             objectShader->set_uniform_vec3f("lightPosition", -2.0f, 3.0f, 2.0f);
           
             for (auto &object : objects) {
                  object->render(objectBatch);
             }
             glLineWidth(1);
             objectBatch->render();
        }
        void draw_bounding_box(SceneObject *object) {
             RenderVertices outline = {
                   RenderVertex(-1.0, -1.0, -1.0),
                   RenderVertex(1.0, -1.0, -1.0),
                   RenderVertex(-1.0, 1.0, -1.0),
                   RenderVertex(1.0, 1.0, -1.0),
            
                   RenderVertex(-1.0, -1.0, 1.0),
                   RenderVertex(1.0, -1.0, 1.0),
                   RenderVertex(-1.0, 1.0, 1.0),
                   RenderVertex(1.0, 1.0, 1.0),
            
                   RenderVertex(-1.0, -1.0, -1.0),
                   RenderVertex(-1.0, 1.0, -1.0),
                   RenderVertex(1.0, -1.0, -1.0),
                   RenderVertex(1.0, 1.0, -1.0),
            
                   RenderVertex(-1.0, -1.0, 1.0),
                   RenderVertex(-1.0, 1.0, 1.0),
                   RenderVertex(1.0, 1.0, 1.0),
                   RenderVertex(1.0, -1.0, 1.0),
                 
                   RenderVertex(-1.0, -1.0, -1.0),
                   RenderVertex(-1.0, -1.0, 1.0),
                   RenderVertex(1.0, -1.0, -1.0),
                   RenderVertex(1.0, -1.0, 1.0),
            
                   RenderVertex(-1.0, 1.0, -1.0),
                   RenderVertex(-1.0, 1.0, 1.0),
                   RenderVertex(1.0, 1.0, -1.0),
                   RenderVertex(1.0, 1.0, 1.0),
             };
             
             AABB aabb = object->get_bounding_box();
             Vec3f gradient = Vec3f(aabb.max).sub(aabb.min);
             for (auto &vertex : outline) {
                  vertex.Position.mul(gradient);
                  vertex.Position.mul(0.5f);
                  vertex.Position.add(object->position);
             }
             
             outlineBatch->add(outline);
        }
        
        Plane get_XZ_plane() { return xz; }
        void set_selected(SceneObject *object) {
             this->selectedObject = object;
        }
        SceneObject *get_selected() { return this->selectedObject; }
        std::vector<SceneObject*> &get_objects() { return this->objects; }
        
        int object_index(SceneObject *object) {
             for (int i = 0; i < objects.size(); i++) {
                  if (object == this->objects.at(i)) {
                       return i;
                  }
             }
             
             return -1;
        }
        
        void export_obj(const std::string &fileName) {
             std::ofstream write(fileName);
             
             write << "# Generated using Emanuel G's model editor.\n";
             write << std::fixed;
             write << std::setprecision(5);
             
             // Vertex positions
             for (auto &object : objects) {
                  RenderVertices vertices = object->get_mesh().renderVertices;
                  for (auto &vertex : vertices) {
                        Vec3f position = vertex.Position;
                        position.mul(object->scaling);
                        position.add(object->position);
                        
                        write << "v" << " " << position.x << " " << position.y << " " << position.z << "\n";
                  }
             }
             write << "\n";
             
             // Vertex normals
             for (auto &object : objects) {
                  RenderVertices vertices = object->get_mesh().renderVertices;
                  for (auto &vertex : vertices) {
                        Vec3f normal = vertex.Normal;
                        
                        write << "vn" << " " << normal.x << " " << normal.y << " " << normal.z << "\n";
                  }
             }
             write << "\n";
             
             // Vertex triangle indices
             RenderIndices indices;
             uint indexCount = 0;
             for (auto &object : objects) {
                  RenderIndices indices2 = object->get_mesh().indices;
                  for (auto &index : indices2) {
                       uint indx = index + indexCount + 1;
                       indices.push_back(indx);
                  }
                  indexCount += object->get_mesh().max_index() + 1;
             } 
                
             for (int i = 0; i < indices.size() / 3; i++) {
                  uint index1 = indices.at(i * 3 + 0);
                  uint index2 = indices.at(i * 3 + 1);
                  uint index3 = indices.at(i * 3 + 2);
                        
                  std::string vertex1 = std::to_string(index1) + "/" + std::to_string(index1) + "/" + std::to_string(index1);
                  std::string vertex2 = std::to_string(index2) + "/" + std::to_string(index2) + "/" + std::to_string(index2);
                  std::string vertex3 = std::to_string(index3) + "/" + std::to_string(index3) + "/" + std::to_string(index3);
                        
                        
                  write << "f" << " " << vertex1 << " " << vertex2 << " " << vertex3 << "\n";
             }
             write << "\n";
             
             write.close();
        }
        
        void dispose() {
             gridShader->clear();
             axisShader->clear();
             objectShader->clear();
             outlineShader->clear();
             
             gridBatch->dispose();
             axisBatch->dispose();
             objectBatch->dispose();
             outlineBatch->dispose();
        }
     private:
        void setup_grid(int width, int depth) {
             Vec3f gridColor = Vec3f(0.85f, 0.85f, 0.85f);
             
             // X lines
             for (int x = 0; x <= width; x++) {
                  if (x != width / 2) {
                       grid.push_back(RenderVertex(Vec3f(x - width / 2, 0, -depth / 2), gridColor));
                       grid.push_back(RenderVertex(Vec3f(x - width / 2, 0, depth / 2), gridColor));
                  }
             }
           
             // Z lines
             for (int z = 0; z <= depth; z++) {
                  if (z != depth / 2) {
                       grid.push_back(RenderVertex(Vec3f(-width / 2, 0, z - depth / 2), gridColor));
                       grid.push_back(RenderVertex(Vec3f(width / 2, 0, z - depth / 2), gridColor));
                  }
             }
        }
        void setup_axes(int width, int height, int depth) {
             // X axis
             axis.push_back(RenderVertex(Vec3f(-width / 2, 0, 0), Vec3f(1.0f, 0.0f, 0.0f)));
             axis.push_back(RenderVertex(Vec3f(width / 2, 0, 0), Vec3f(1.0f, 0.0f, 0.0f)));
           
             // Y axis
             axis.push_back(RenderVertex(Vec3f(0, -height / 2, 0), Vec3f(0.0f, 1.0f, 0.0f)));
             axis.push_back(RenderVertex(Vec3f(0, height / 2, 0), Vec3f(0.0f, 1.0f, 0.0f)));
           
             // Z axis
             axis.push_back(RenderVertex(Vec3f(0, 0, -depth / 2), Vec3f(0.0f, 0.0f, 1.0f)));
             axis.push_back(RenderVertex(Vec3f(0, 0, depth / 2), Vec3f(0.0f, 0.0f, 1.0f)));
        }
        
     private:
        std::vector<SceneObject*> objects;
        Batch *objectBatch, *gridBatch, *axisBatch, *outlineBatch;
        Shader *objectShader, *gridShader, *axisShader, *outlineShader;
        SceneObject *selectedObject;
        
        RenderVertices grid, axis;
        Plane xz;
};

namespace Variables {
     Camera *camera;
     CameraControls *controls;
     Scene *scene;
    
     void load() {
          camera = new Camera();
          controls = new CameraControls(camera);
          camera->position = Vec3f(1.0f, 1.0f, 1.0f);
           
          scene = new Scene();
          
     }
     void dispose() {
          scene->dispose();
     }
};

struct Ray {
     Vec3f start;
     Vec3f direction;
     float maxLength;
     Ray() {}
     Ray(Vec3f start, Vec3f direction, float maxLength) : start(start), direction(direction), maxLength(maxLength) {}
     
     SceneObject *intersect() {
          if (Variables::scene->get_objects().empty()) return nullptr;
          
          float length = 0.0f;
          Vec3f tip = Vec3f(this->start);
          Vec3f offset = Vec3f(this->direction).mul(0.01f);
          
          int steps = 0;
          while (length < maxLength) {
                steps++;
                tip.add(offset);
                for (auto &object : Variables::scene->get_objects()) {
                     if (object->get_bounding_box().point_inside(tip)) {
                          return object;
                     }
                }
                
                
                length += 0.01f;
          }
          return nullptr;
     }
};

namespace UI {
     Label *positionLabel;
     Button *select;
     TextField *x, *y, *z, *scalingX, *scalingY, *scalingZ;
     TextField *projectName;
     Table *meshesTable, *propertiesTable, *projectTable;
     std::vector<Cell*> uiObjects;
     Mat4x4 projection;
     bool focused = false;
     int selectionIndex = 0;
     
     void add(Cell *obj) {
           uiObjects.push_back(obj);
     }
     void load() {
           projection.set_orthographic(-SCREEN_WIDTH / 2.0f, SCREEN_WIDTH / 2.0f, -SCREEN_HEIGHT / 2.0f, SCREEN_HEIGHT / 2.0f, -2.0f, 1000.0f);
           focusedTextField = nullptr;
           
           Label *label = new Label("+");
           add(label);
           
           CheckBox *check = new CheckBox("Display grid", true, [](bool checked){ TemporarySettings::displayGrid = checked; });
           check->set_position(SCREEN_WIDTH * 0.25f + 10, SCREEN_HEIGHT * 0.35f + 10);
           add(check);
           
           select = new Button("Select", [](){
                 Camera *camera = Variables::camera;
                 Vec3f direction = camera->get_direction();
                 Ray ray = Ray(camera->position, direction, 100.0f);
                 
                 /*
                 Vec3f intersection = ray.intersect();
                 for (auto &object : Variables::scene->get_objects()) {
                      if (object->get_bounding_box().point_inside(intersection)) {
                           Variables::scene->set_selected(object);
                      }
                 }
                 */
                 Variables::scene->set_selected(ray.intersect());
           });
           
           select->set_size(70.0f, 70.0f);
           select->set_position(0.4f * SCREEN_WIDTH, -0.4f * SCREEN_HEIGHT);  
           add(select);
             
           meshesTable = new Table("Meshes", SCREEN_WIDTH * 0.4f, 0.0f, 120.0f, 200.0f);
           
           propertiesTable = new Table("Object Properties", -SCREEN_WIDTH * 0.32f, -SCREEN_HEIGHT * 0.2f, 180.0f, 220.0f);
           propertiesTable->update([](){
                bool selected = Variables::scene->get_selected() != nullptr;
                propertiesTable->set_visibility(selected);
           });
           
           projectTable = new Table("Project", -SCREEN_WIDTH * 0.34f, SCREEN_HEIGHT * 0.35f, 180.0f, 110.0f);
           
           Button *button = new Button("Cube", [](){
                 Plane plane = Variables::scene->get_XZ_plane();
                 Camera *camera = Variables::camera;
                 Vec3f direction = camera->get_direction();
                 Vec3f intersection = plane.intersect_line(camera->position, direction);
                 
                 Mesh mesh = BaseMeshes::cube;
                 SceneObject *cube = new SceneObject(mesh.set_color(0.8f, 0.8f, 0.8f));
                 cube->set_position(intersection);
                 
                 Variables::scene->add_object(cube);
                 Variables::scene->set_selected(cube);
           });
           
           Button *button2 = new Button("Sphere (soon)", [](){});
           
           Button *button3 = new Button("Deselect", [](){
                 Variables::scene->set_selected(nullptr);
           });
           button3->set_size(150.0f, 25.0f);
           
           x = new TextField("X:", "", TextFieldFilters::floats);
           x->set_size(60.0f, 15.0f);
           
           y = new TextField("Y:", "", TextFieldFilters::floats);
           y->set_size(60.0f, 15.0f);
           
           z = new TextField("Z:", "", TextFieldFilters::floats);
           z->set_size(60.0f, 15.0f);
           z->set_paddingY(15.0f);
           
           scalingX = new TextField("SclX:", "", TextFieldFilters::floats);
           scalingX->set_size(60.0f, 15.0f);
           scalingX->set_paddingX(25.0f);
           
           scalingY = new TextField("SclY:", "", TextFieldFilters::floats);
           scalingY->set_size(60.0f, 15.0f);
           scalingY->set_paddingX(25.0f);
           
           scalingZ = new TextField("SclZ:", "", TextFieldFilters::floats);
           scalingZ->set_size(60.0f, 15.0f);
           scalingZ->set_paddingX(25.0f);
           
           projectName = new TextField("Name:", "", TextFieldFilters::characters);
           projectName->set_size(80.0f, 25.0f);
           projectName->set_labelPaddingX(15.0f);
           
           Button *button4 = new Button("Apply", [](){
                  float px, py, pz, sclX, sclY, sclZ;
                  px = py = pz = 0.0f;
                  sclX = sclY = sclZ = 1.0f;
                  
                  if (x->get_text().length() > 0) px = std::stof(x->get_text());
                  if (y->get_text().length() > 0) py = std::stof(y->get_text());
                  if (z->get_text().length() > 0) pz = std::stof(z->get_text());
                 
                  if (scalingX->get_text().length() > 0) sclX = std::stof(scalingX->get_text());
                  if (scalingY->get_text().length() > 0) sclY = std::stof(scalingY->get_text());
                  if (scalingZ->get_text().length() > 0) sclZ = std::stof(scalingZ->get_text());
                 
                  SceneObject *selected = Variables::scene->get_selected();
                  if (selected != nullptr) {
                      selected->set_position(px, py, pz);
                      selected->set_scaling(sclX, sclY, sclZ);
                  }
           });
           button4->set_size(150.0f, 25.0f);
           
           Button *button5 = new Button("Remove", [](){
                  SceneObject *selected = Variables::scene->get_selected();
                  if (selected != nullptr) {
                       Variables::scene->remove_object(selected);
                       Variables::scene->set_selected(nullptr);
                  }
           });
           button5->set_size(100.0f, 25.0f);
           
           Button *button6 = new Button("Export as .obj", [](){
                  if (projectName->get_text().length() > 0) {
                        std::string name = projectName->get_text();
                        Variables::scene->export_obj(name + ".obj");
                        printf("Project file got exported.\n");
                  } else {
                        printf("Project name length > 0.\n");
                  }
           });
           button6->set_size(150.0f, 25.0f);
           
           
           meshesTable->add_object(button);
           meshesTable->add_object(button2);
           
           propertiesTable->add_object(x);
           propertiesTable->add_object(y);
           propertiesTable->add_object(z);
           propertiesTable->add_object(button4);
           propertiesTable->add_object(button3);
           propertiesTable->add_object(button5);
           
           propertiesTable->column(scalingX->paddingX + scalingX->width);
           propertiesTable->add_object(scalingX);
           propertiesTable->add_object(scalingY);
           propertiesTable->add_object(scalingZ);
           
           projectTable->add_object(projectName);
           projectTable->add_object(button6);
           
           positionLabel = new Label();
           positionLabel->update([](){
                positionLabel->set_text("Position: " + 
                               std::to_string(int(Variables::camera->position.x)) + ", " +
                               std::to_string(int(Variables::camera->position.y)) + ", " +
                               std::to_string(int(Variables::camera->position.z)));
                               
                positionLabel->set_position(0.32f * SCREEN_WIDTH, 0.46f * SCREEN_HEIGHT);
           });
           
           add(positionLabel);
     }
     void handle_event(SDL_Event event, float timeTook) {
           int cx, cy;
           SDL_GetMouseState(&cx, &cy);
           int w = 0, h = 0;
           SDL_GetWindowSize(windows, &w, &h);
           
           float mx = (float) cx, my = (float) cy; 
           
           // Normalized coordinates
           mx /= w;
           my /= h;
           
           mx *= SCREEN_WIDTH;
           my *= SCREEN_HEIGHT;
           mx -= SCREEN_WIDTH / 2.0f;
           my -= SCREEN_HEIGHT / 2.0f;
           my *= -1;
           
           
           if (event.type == SDL_MOUSEBUTTONDOWN) {
                for (auto &object : uiObjects) {
                     if (object->is_visible()) object->mouse_down(mx, my);
                }
           }
           if (event.type == SDL_MOUSEBUTTONUP) {
                for (auto &object : uiObjects) {
                     if (object->is_visible()) object->mouse_up(mx, my);
                }
           }
           if (event.type == SDL_MOUSEMOTION) {
                for (auto &object : uiObjects) {
                     if (object->is_visible() && object->can_focus(mx, my)) {
                          focused = true;
                          break;
                     }
                }
                for (auto &object : uiObjects) {
                     if (!object->is_visible()) continue;
                     
                     if (object->point_inside(mx, my)) {
                          object->mouse_enter();
                     } else {
                          object->mouse_exit();
                     }
                }
           }
           meshesTable->handle_event(event, timeTook);
           propertiesTable->handle_event(event, timeTook);
           projectTable->handle_event(event, timeTook);
     }
     void update() {
           for (auto &object : uiObjects) {
                 std::function<void()> listener = object->get_update_listener();
                 if (listener != NULL) {
                      listener();
                 }
           }
           meshesTable->update();
           propertiesTable->update();
           projectTable->update();
     }
     void render() {
           glDisable(GL_DEPTH_TEST);
           Renderer::overlayShader->use();
           Renderer::overlayShader->set_uniform_mat4("projection", projection);
           
           for (auto &object : uiObjects) {
                if (!object->is_visible()) continue;
                
                object->render();
           }
           meshesTable->render();
           propertiesTable->render();
           projectTable->render();
           
           Renderer::overlayShader->set_uniform_bool("renderingText", false);
           Renderer::atlas->use();
           Renderer::uiBatch->render();
           
           Renderer::overlayShader->set_uniform_bool("renderingText", true);
           Renderer::textAtlas->use();
           Renderer::textBatch->render();
           glEnable(GL_DEPTH_TEST);
     }
};

class Game
{
  public:
    const char *displayName = "";
    virtual ~Game() {};
    virtual void init() {};
    virtual void load() {};
    
    virtual void handle_event(SDL_Event ev, float timeTook) {};

    virtual void update(float timeTook) {};
    virtual void dispose() {};
};

class Modeling : public Game {
    public:  
       void init() override {
           displayName = "Modeling";
       }
       void load() override {
           TemporarySettings::load();
           UIPallete::load();
           FreeType::get().load();
           Renderer::load();
           UI::load();
                    
           Variables::load();
       }
       void handle_event(SDL_Event ev, float timeTook) override {
           UI::focused = false;
           UI::handle_event(ev, timeTook);
           if (!UI::focused) {
               Variables::controls->handle_event(ev, timeTook);
           }
       }
       void update(float timeTook) override {
           UI::update();
           
           Variables::camera->update();
           Variables::scene->update(timeTook);
           
           Variables::scene->render(Variables::camera);
           
           UI::render();
       }
       
       void dispose() override {
           Variables::dispose();
           Renderer::dispose();
           FreeType::get().dispose();
       }
};

int main()
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
		return 1;
	}
    
	// We use OpenGL ES 3.0
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	// We want at least 8 bits per color
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    
    
    Modeling game;
    game.init();
    
	SDL_Window *window = SDL_CreateWindow("OpenGL Example", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL);
	if (window == NULL)
	{
		fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
		return 1;
	}
	
	// We will not actually need a context created, but we should create one
	SDL_GLContext context = SDL_GL_CreateContext(window);
    windows = window;
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glEnable(GL_BLEND);
    
    glDepthFunc(GL_LESS);
    glPolygonOffset(1, 0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    game.load();    
    
	float then = 0.0f, delta = 0.0f;
    bool disabled = false;
    SDL_Event e;
    while (!disabled)
	{
		while (SDL_PollEvent(&e))
		{
			// Event-handling code
            game.handle_event(e, delta);
            if (e.type == SDL_QUIT) {
                disabled = true;
                break;
            }
		}
		float now = SDL_GetTicks();
        delta = (now - then) * 1000 / SDL_GetPerformanceFrequency();
        then = now;
   
		// Drawing
		glClearColor(0.4f, 0.5f, 0.9f, 1.0f);
    	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    	
    	// Update and render to screen code
    	game.update(delta);
    	
		// Swap buffers
		SDL_GL_SwapWindow(window);
	}
	game.dispose();
	printf("Modeling exiting");
	
    SDL_GL_DeleteContext(context);
    
	SDL_DestroyWindow(window);
	SDL_Quit();
	IMG_Quit();
	
	return 0;
}