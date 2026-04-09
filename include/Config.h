#pragma once

namespace cfg {
    constexpr int windowWidth = 1280;
    constexpr int windowHeight = 720;

    constexpr float laneX[3] = {-3.0f, 0.0f, 3.0f};
    constexpr float roadWidth = 11.0f;
    constexpr float laneMarkerWidth = 0.08f;

    constexpr float playerY = 0.85f;
    constexpr float playerZ = 6.0f;
    constexpr float playerWidth = 1.8f;
    constexpr float playerHeight = 1.6f;
    constexpr float playerDepth = 2.8f;

    constexpr float obstacleSpawnZ = -130.0f;
    constexpr float obstacleRemoveZ = 16.0f;
    constexpr float obstacleBaseSpeed = 30.0f;

    constexpr float laneSwitchSpeed = 10.0f;
    constexpr float minSpawnInterval = 0.55f;
    constexpr float maxSpawnInterval = 1.25f;
}
