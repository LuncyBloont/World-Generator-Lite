#include "camera.h"
#include <cstdio>

Camera::Camera() {

}

void Camera::lookAt(glm::vec3 pos) {
    front = glm::normalize(pos - position);
}

void Camera::turn(glm::vec2 uv) {
    front = glm::normalize(front);
    glm::vec3 hv = glm::vec3(front.y, -front.x, 0.0f) * uv.x;
    glm::vec3 vv = glm::vec3(0.0f, 0.0f, glm::pow(2.0, glm::abs(front.z))) * uv.y;
    glm::vec3 nf = front + hv + vv;
    nf = glm::normalize(nf);
    if (nf.z > uplock) {
        nf.z = uplock;
    }
    if (nf.z < downlock) {
        nf.z = downlock;
    }
    front = glm::normalize(nf);
}

void Camera::move(glm::vec3 off) {
    glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 0.0f, 1.0f)));
    position += right * off.x + glm::vec3(0.0f, 0.0f, 1.0f) * off.z +
                glm::normalize(glm::vec3(front.x, front.y, 0.0f)) * off.y;
}

glm::mat4 Camera::getV() {
    glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 0.0f, 1.0f)));
    glm::vec3 up = glm::cross(right, front);
    glm::mat4 vt = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
                             0.0f, 1.0f, 0.0f, 0.0f,
                             0.0f, 0.0f, 1.0f, 0.0f,
                             -position.x, -position.y, -position.z, 1.0f);
    glm::mat4 v = glm::mat4(right.x, up.x, -front.x, 0.0f,
                            right.y, up.y, -front.y, 0.0f,
                            right.z, up.z, -front.z, 0.0f,
                            0.0f, 0.0f, 0.0f, 1.0f);
    return v * vt;
}

glm::mat4 Camera::getP(glm::vec2 resoution) {
    float k = cfar / (cfar - cnear);
    return glm::mat4(cnear * scalex * resoution.y / resoution.x, 0.0f, 0.0f, 0.0f,
                     0.0f, -cnear * scaley, 0.0f, 0.0f,
                     0.0f, 0.0f, -k, -1.0f,
                     0.0f, 0.0f, -k * cnear, 0.0);
}

void Camera::lerp(const Camera& other, float p) {
    position = glm::mix(position, other.position, p);
    front = glm::mix(front, other.front, p);
    front = glm::normalize(front);
}
