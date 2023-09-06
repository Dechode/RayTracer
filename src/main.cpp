#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_version.h>
#include <SDL2/SDL_video.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#include <glm/packing.hpp>
#include <glm/trigonometric.hpp>
#include <iostream>
#include <sys/types.h>
#include <vector>

class Ray {
public:
  Ray() {
    origin = glm::vec3(0.0f);
    target = glm::vec3(0.0f);
    target.z = -100.0f;
  }
  Ray(glm::vec3 &pOrigin, glm::vec3 &pTarget) {
    origin = pOrigin;
    target = pTarget;
  }
  void setTarget(glm::vec3 &pTarget) { target = pTarget; }
  void setOrigin(glm::vec3 &pOrigin) { origin = pOrigin; }

  glm::vec3 getTarget() const { return target; }
  glm::vec3 getOrigin() const { return origin; }
  glm::vec3 at(float t) const { return origin + target * t; }

private:
  glm::vec3 origin;
  glm::vec3 target;
};

struct Material {
  glm::vec3 albedo = {1.0f, 0.0f, 1.0f};
  float roughness = 0.1f;
};

struct Sphere {
  glm::vec3 position = {0.0f, 0.0f, 0.0f};
  float radius = 0.5f;
  Material material;
};

struct Scene {
  std::vector<Sphere> spheres;
};

struct Hit {
  glm::vec3 position;
  glm::vec3 normal;
  uint32_t color = 0x000000FF;
  float distance = -1.0f;
  int sphereID  = -1;
};

uint32_t convertToRGBA(const glm::vec4 &color) {
  uint32_t r = (uint8_t)(color.r * 255.0f);
  uint32_t g = (uint8_t)(color.g * 255.0f);
  uint32_t b = (uint8_t)(color.b * 255.0f);
  uint32_t a = (uint8_t)(color.a * 255.0f);
  return (uint32_t)((r << 24) | (g << 16) | (b << 8) | a);
}

Hit miss(const Ray& ray) {
  Hit rayHit;
  rayHit.distance = -1.0f;
  return rayHit;
}

Hit closestHit(const Ray &ray, float hitDistance, const Scene &scene, int id) {
  Hit rayHit;
  rayHit.distance = hitDistance;
  rayHit.sphereID = id;

  glm::vec3 origin = ray.getOrigin() - scene.spheres[id].position;
  rayHit.position = origin + ray.getTarget() * hitDistance;
  rayHit.normal = glm::normalize(rayHit.position);
  rayHit.position += scene.spheres[id].position;

  return rayHit;
}

Hit traceRay(const Scene &scene, const Ray &ray) {
  Hit rayHit;
  int closestSphere = -1;
  float hitDistance = FLT_MAX;

  for (int i = 0; i < scene.spheres.size(); i++) {
    glm::vec3 oc = ray.getOrigin() - scene.spheres[i].position;
    float a = glm::dot(ray.getTarget(), ray.getTarget());
    float b = 2.0f * glm::dot(oc, ray.getTarget());
    float c = glm::dot(oc, oc) - scene.spheres[i].radius * scene.spheres[i].radius;

    float discriminant = b * b - 4 * a * c;

    if (discriminant < 0.0f) {
      continue;
    }

    float t = (-b - glm::sqrt(discriminant)) / (2.0f * a);
    if (t > 0.0f && t < hitDistance) {
      hitDistance = t;
      closestSphere = i;
    }
  }

  if (closestSphere == -1) {
    return miss(ray);
  }

  return closestHit(ray, hitDistance, scene, closestSphere);
}

Hit perPixel(const Scene &scene, Ray &pRay) {
  glm::vec3 color(0.0);
  float multiplier = 1.0f;

  Ray ray = pRay;

  Hit rayHit;  
  int bounces = 10;
  for (int i = 0; i < bounces; i++){
    rayHit = traceRay(scene, ray);

    if (rayHit.distance >= 0.0f) {
      glm::vec3 lightDirection = glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f));

      float d = glm::max(glm::dot(rayHit.normal, -lightDirection), 0.0f);

      const Sphere &sphere = scene.spheres[rayHit.sphereID];
      
      glm::vec3 ambient = glm::vec3(1.0f) * 0.1f;
      glm::vec3 sphereColor = glm::clamp(glm::vec3((sphere.material.albedo * d) + ambient), 0.0f, 1.0f);

      color += sphereColor * multiplier;
      multiplier *= 0.7f;

      glm::vec3 newRayOrigin = rayHit.position + rayHit.normal * 0.0001f;
      glm::vec3 randomVec = {-0.5 + (float)rand() / (float) (RAND_MAX / (0.5 - (-0.5))),
                             -0.5 + (float)rand() / (float) (RAND_MAX / (0.5 - (-0.5))),
                            -0.5 + (float)rand() / (float) (RAND_MAX / (0.5 - (-0.5)))};
      randomVec = glm::normalize(randomVec);
      glm::vec3 newTarget = glm::reflect(ray.getTarget(), rayHit.normal + sphere.material.roughness * randomVec);
      ray.setOrigin(newRayOrigin);
      ray.setTarget(newTarget);
    } else {
      // glm::vec3 normalizedRay = glm::normalize(ray.getTarget());
      // float amount = 0.5f * (normalizedRay.y + 1.0f);
      // glm::vec3 white(1.0f, 1.0f, 1.0f);
      // glm::vec3 blue(0.0f, 0.5f, 1.0f);
      // glm::vec3 bgColor(blue + amount * (blue - white));

      glm::vec3 bgColor(0.5f, 0.7f, 1.0f);

      color += bgColor * multiplier;
      color = glm::clamp(color, 0.0f, 1.0f);
      break;
    }
  }
  rayHit.color = convertToRGBA(glm::vec4(color, 1.0));
  return rayHit;
}

void flipSurface(SDL_Surface *surface) {
  SDL_LockSurface(surface);

  int pitch = surface->pitch;
  char *temp = new char[pitch];
  char *pixels = (char *)surface->pixels;

  for (int i = 0; i < surface->h / 2; ++i) {
    char *row1 = pixels + i * pitch;
    char *row2 = pixels + (surface->h - i - 1) * pitch;

    memcpy(temp, row1, pitch);
    memcpy(row1, row2, pitch);
    memcpy(row2, temp, pitch);
  }

  delete[] temp;

  SDL_UnlockSurface(surface);
}

bool running = true;
uint32_t width = 1280;
uint32_t height = 720;

int main(void) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::printf("Could not init SDL: %s\n", SDL_GetError());
    return 1;
  }

  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_CreateWindowAndRenderer(width, height, SDL_WINDOW_RESIZABLE, &window,
                              &renderer);
  if (!window) {
    std::printf("Could not create window!");
    return 1;
  }

  SDL_SetWindowTitle(window, "RayTracer");
  
  SDL_Surface *surface = SDL_GetWindowSurface(window);
  SDL_Surface *pixels = SDL_CreateRGBSurfaceWithFormat(
      0, width, height, 32, SDL_PIXELFORMAT_RGBX8888);

  int pitch = pixels->pitch;

  uint32_t startTime = SDL_GetTicks();
  float deltaTime = 0.0f;

  float focalLength = 1.0;
  float viewportHeight = 1.0;
  float viewportWidth = viewportHeight * ((double)width / height);
  glm::vec3 cameraCenter(0.0);

  glm::vec3 viewportU(viewportWidth, 0.0, 0.0);
  glm::vec3 viewportV(0.0, viewportHeight, 0.0);

  glm::vec3 pixelDeltaU = {viewportU.x / width, viewportU.y / width,
                           viewportU.z / width};
  glm::vec3 pixelDeltaV = {viewportV.x / height, viewportV.y / height,
                           viewportV.z / height};

  glm::vec3 viewportUpperLeft = cameraCenter -
                                glm::vec3(0.0, 0.0, focalLength) -
                                viewportU * 0.5f - viewportV * 0.5f;
  glm::vec3 pixel00Loc = viewportUpperLeft + 0.5f * (pixelDeltaU + pixelDeltaV);

  glm::vec3 pixelCenter(0.0);
  glm::vec3 rayDirection(0.0);

  Scene scene;

  Material material;
  material.roughness = 0.01f;
  {
    Sphere sphere;
    sphere.material = material;
    sphere.position = glm::vec3(-1.0, 0.0, -2.0);
    sphere.radius = 0.5f;
    scene.spheres.push_back(sphere);
  }

  {
    Sphere sphere;
    sphere.material = material;
    sphere.position = glm::vec3(1.0, 0.0, -2.0);
    sphere.radius = 0.5f;
    scene.spheres.push_back(sphere);
  }

  {
    Sphere ground;
    ground.material.albedo = {0.0f, 1.0, 0.0f};
    ground.material.roughness = 0.9f;
    ground.position = glm::vec3(0.0, -200.5, 0.0);
    ground.radius = 200.0f;
    scene.spheres.push_back(ground);
  }

  Ray ray;
  ray.setOrigin(cameraCenter);

  SDL_Event event;

  while (running) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = false;
      }
    }

    SDL_FillRect(pixels, nullptr, 0);

    SDL_LockSurface(pixels);

    for (uint32_t j = 0; j < height; j++) {
      for (uint32_t i = 0; i < width; i++) {
        uint32_t *row = (uint32_t *)((char *)pixels->pixels + pitch * j);

        pixelCenter =
            pixel00Loc + ((float)i * pixelDeltaU) + ((float)j * pixelDeltaV);
        rayDirection = pixelCenter - cameraCenter;

        ray.setTarget(rayDirection);

        Hit hit = perPixel(scene, ray);
        row[i] = hit.color;
      }
    }
    SDL_UnlockSurface(pixels);
    flipSurface(pixels);
    SDL_BlitSurface(pixels, nullptr, surface, nullptr);
    SDL_UpdateWindowSurface(window);

    uint32_t endTime = SDL_GetTicks();
    deltaTime = ((float)endTime - startTime); // 1000.0f;
    startTime = endTime;

    std::printf("Delta time: %2.0f ms\n", deltaTime);
    std::printf("FPS: %2.2f\n", 1000.0 / deltaTime);
  }

  SDL_DestroyWindow(window);
  return 0;
}