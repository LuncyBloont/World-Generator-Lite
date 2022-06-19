#ifndef CAMERA_H
#define CAMERA_H
#include <glm/glm/glm.hpp>

class Camera {
public:
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 front = glm::vec3(0.0f, 1.0f, 0.0f);
    float cnear = 0.1f;
    float cfar = 1000.0f;
    float uplock = 0.9f;
    float downlock = -0.9f;
    float scalex = 20.0f;
    float scaley = 20.0f;

public:
    Camera();
    void lookAt(glm::vec3 pos);
    void turn(glm::vec2 uv);
    void move(glm::vec3 off);
    glm::mat4 getV();
    glm::mat4 getP(glm::vec2 resoution);
    void lerp(const Camera& other, float p);
    glm::mat4 getShadowV(glm::vec3 sunDir, float size, int level, float back, float front);
};

#endif // CAMERA_H
