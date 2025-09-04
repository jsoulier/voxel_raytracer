#pragma once

class Camera;
class WorldProxy;

void NoiseSetCamera(Camera& camera);
void NoiseSetChunk(WorldProxy& proxy, int chunkX, int chunkZ);