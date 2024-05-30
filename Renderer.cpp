#include <fstream>
#include "Scene.hpp"
#include "Renderer.hpp"
#include <mutex>
#include<thread>

std::mutex mtx;

inline float deg2rad(const float& deg) { return deg * M_PI / 180.0; }

const float EPSILON = 0.001;

void Renderer::Render(const Scene& scene)
{
    std::vector<Vector3f> framebuffer(scene.width * scene.height);

    float scale = tan(deg2rad(scene.fov * 0.5));
    float imageAspectRatio = scene.width / (float)scene.height;
    Vector3f eye_pos(278, 273, -800);

    // change the spp value to change sample ammount
    int spp = 2;
    std::cout << "SPP: " << spp << "\n";

    int process = 0;
    const int thred = 20;
    int times = scene.height / thred;
    std::thread th[thred];

    auto castRayMultiThread = [&](uint32_t y_min, uint32_t y_max){
        for (uint32_t j = y_min; j < y_max; j++) {
            int m = j * scene.width;
            for (uint32_t i = 0; i < scene.width; i++) {
                float x = (2 * (i + 0.5) / (float)scene.width - 1) *
                          imageAspectRatio * scale;
                float y = (1 - 2 * (j + 0.5) / (float)scene.height) * scale;

                Vector3f dir = normalize(Vector3f(-x, y, 1));
                for (int k = 0; k < spp; k++) {
                    framebuffer[m] += scene.castRay(Ray(eye_pos, dir), 0) / spp;
                }
                m++;
            }
            mtx.lock();
            process++;
            UpdateProgress(1.0 * process / scene.height);
            mtx.unlock();
        }
    };

    //分行进行路径追踪
    for (int i = 0; i < thred; i++) {//从第0行出发，一共有0~by-1行
        th[i] = std::thread(castRayMultiThread, i * times, (i + 1) * times);
    }
    //每个线程执行join
    for (int i = 0; i < thred; i++) {
        th[i].join();
    }
    UpdateProgress(1.f);

    // 保存成ppm
    FILE* fp = fopen("binary.ppm", "wb");
    (void)fprintf(fp, "P6\n%d %d\n255\n", scene.width, scene.height);
    for (auto i = 0; i < scene.height * scene.width; ++i) {
        static unsigned char color[3];
        color[0] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].x), 0.6f));
        color[1] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].y), 0.6f));
        color[2] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].z), 0.6f));
        fwrite(color, 1, 3, fp);
    }
    fclose(fp);
}