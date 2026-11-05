#include "game_data.h"
#include <cmath>

PrecisionTimer g_timer;
GameState g_gameState;

std::atomic<bool> g_lmbPressed{false};
std::atomic<bool> g_rmbPressed{false};
std::mutex g_buttonLock;

std::mutex g_patternLock;
std::vector<std::pair<float, float>> g_scaledPattern;
float g_scaledDelayMs = 0.0f;

std::map<std::string, WeaponData> g_weaponData;
std::map<std::string, float> g_scopes;
std::map<std::string, float> g_barrels;
std::map<std::string, float> g_weaponSpeedFactors;
std::map<std::string, float> g_hipfireMultipliers;

void initWeaponData() {
  g_scopes = {
      {"None", 1.0f}, {"Holo", 1.2f}, {"Handmade", 0.8f}, {"8x", 7.20f}};

  g_barrels = {{"None", 1.0f},
               {"Silencer", 1.0f},
               {"Muzzle Boost", 1.14f},
               {"Muzzle Brake", 0.5f}};

  g_weaponSpeedFactors = {
      {"AK-47", 1.02f},    {"LR300", 0.99f},
      {"MP5A4", 1.02f},    {"Custom SMG", 1.22f},
      {"Thompson", 0.94f}, {"M249", 0.98f},
      {"HMLMG", 1.0f},     {"Semi Auto Rifle", 5.0f},
      {"Revolver", 5.0f},  {"Semi Automatic Pistol", 5.0f},
      {"M92", 5.0f},       {"M39", 5.0f},
      {"Python", 5.0f},    {"Nail Gun", 5.0f},
  };

  g_hipfireMultipliers = {
      {"AK-47", 0.6f},     {"LR300", 0.6f},
      {"MP5A4", 1.02f},    {"Custom SMG", 1.22f},
      {"Thompson", 0.94f}, {"M249", 0.98f},
      {"HMLMG", 1.0f},     {"Semi Auto Rifle", 5.0f},
      {"Revolver", 5.0f},  {"Semi Automatic Pistol", 5.0f},
      {"M92", 5.0f},       {"M39", 5.0f},
      {"Python", 5.0f},    {"Nail Gun", 5.0f},
  };

  g_weaponData["AK-47"] = {{{0.000000f, -2.257792f}, {0.323242f, -2.300758f},
                            {0.649593f, -2.299759f}, {0.848786f, -2.259034f},
                            {1.075408f, -2.323947f}, {1.268491f, -2.215956f},
                            {1.330963f, -2.236556f}, {1.336833f, -2.218203f},
                            {1.505516f, -2.143454f}, {1.504423f, -2.233091f},
                            {1.442116f, -2.270194f}, {1.478543f, -2.204318f},
                            {1.392874f, -2.165817f}, {1.480824f, -2.177887f},
                            {1.597069f, -2.270915f}, {1.449996f, -2.145893f},
                            {1.369179f, -2.270450f}, {1.582363f, -2.298334f},
                            {1.516872f, -2.235066f}, {1.498249f, -2.238401f},
                            {1.465769f, -2.331642f}, {1.564812f, -2.242621f},
                            {1.517519f, -2.303052f}, {1.422433f, -2.211946f},
                            {1.553195f, -2.248043f}, {1.510463f, -2.285327f},
                            {1.553878f, -2.240047f}, {1.520380f, -2.221839f},
                            {1.553878f, -2.240047f}, {1.553195f, -2.248043f}},
                           133.3f,
                           true};

  g_weaponData["LR300"] = {{{0.000000f, -2.052616f},  {0.055584f, -1.897695f},
                            {-0.247226f, -1.863222f}, {-0.243871f, -1.940010f},
                            {0.095727f, -1.966751f},  {0.107707f, -1.885520f},
                            {0.324888f, -1.946722f},  {-0.181137f, -1.880342f},
                            {0.162399f, -1.820107f},  {-0.292076f, -1.994940f},
                            {0.064575f, -1.837156f},  {-0.126699f, -1.887880f},
                            {-0.090568f, -1.832799f}, {0.065338f, -1.807480f},
                            {-0.197343f, -1.705888f}, {-0.216561f, -1.785949f},
                            {0.042567f, -1.806371f},  {-0.065534f, -1.757623f},
                            {0.086380f, -1.904010f},  {-0.097326f, -1.969296f},
                            {-0.213034f, -1.850288f}, {-0.017790f, -1.730867f},
                            {-0.045577f, -1.783686f}, {-0.053309f, -1.886260f},
                            {0.055072f, -1.793076f},  {-0.091874f, -1.921165f},
                            {-0.033719f, -1.796160f}, {0.266464f, -1.993952f},
                            {0.079090f, -1.921165f}},
                           120.0f,
                           true};

  g_weaponData["MP5A4"] = {{{0.125361f, -1.052446f},  {-0.099548f, -0.931548f},
                            {0.027825f, -0.954094f},  {-0.013715f, -0.851504f},
                            {-0.007947f, -1.070579f}, {0.096096f, -1.018017f},
                            {-0.045937f, -0.794216f}, {0.034316f, -1.112618f},
                            {-0.003968f, -0.930040f}, {-0.009403f, -0.888503f},
                            {0.140813f, -0.970807f},  {-0.015052f, -1.046551f},
                            {0.095699f, -0.860475f},  {-0.269643f, -1.038896f},
                            {0.000285f, -0.840478f},  {0.018413f, -1.038126f},
                            {0.099191f, -0.851701f},  {0.199659f, -0.893041f},
                            {-0.082660f, -1.069278f}, {0.006826f, -0.881493f},
                            {0.091709f, -1.150956f},  {-0.108677f, -0.965513f},
                            {0.169612f, -1.099499f},  {-0.038244f, -1.120084f},
                            {-0.085513f, -0.876956f}, {0.136279f, -1.047589f},
                            {0.196392f, -1.039977f},  {-0.152513f, -1.209291f},
                            {-0.214510f, -0.956648f}, {0.034276f, -0.095177f}},
                           100.0f,
                           true};

  g_weaponData["Custom SMG"] = {
      {{-0.114414f, -0.680635f}, {0.008685f, -0.676597f},
       {0.010312f, -0.682837f},  {0.064825f, -0.691344f},
       {0.104075f, -0.655617f},  {-0.088118f, -0.660429f},
       {0.089906f, -0.675183f},  {0.037071f, -0.632623f},
       {0.178466f, -0.634737f},  {0.034653f, -0.669444f},
       {-0.082658f, -0.664827f}, {0.025551f, -0.636631f},
       {0.082413f, -0.647118f},  {-0.123305f, -0.662104f},
       {0.028164f, -0.662354f},  {-0.117345f, -0.693474f},
       {-0.268777f, -0.661122f}, {-0.053086f, -0.677493f},
       {0.004238f, -0.647037f},  {0.014169f, -0.551440f},
       {-0.009907f, -0.552079f}, {0.044076f, -0.577694f},
       {-0.043187f, -0.549581f}},
      90.0f,
      true};

  g_weaponData["Thompson"] = {
      {{-0.114413f, -0.680635f}, {0.008686f, -0.676598f},
       {0.010312f, -0.682837f},  {0.064825f, -0.691345f},
       {0.104075f, -0.655618f},  {-0.088118f, -0.660429f},
       {0.089906f, -0.675183f},  {0.037071f, -0.632623f},
       {0.178465f, -0.634737f},  {0.034654f, -0.669443f},
       {-0.082658f, -0.664826f}, {0.025550f, -0.636631f},
       {0.082414f, -0.647118f},  {-0.123305f, -0.662104f},
       {0.028164f, -0.662354f},  {-0.117346f, -0.693475f},
       {-0.268777f, -0.661123f}, {-0.053086f, -0.677493f},
       {0.04238f, -0.647038f},   {0.04238f, -0.647038f}},
      90.0f,
      true};

  {
    std::vector<std::pair<float, float>> pat;
    pat.push_back({0.0f, -1.4f});
    pat.push_back({-0.39f, -1.4f});
    for (int i = 0; i < 58; i++)
      pat.push_back({-0.73f, -1.4f});
    g_weaponData["HMLMG"] = {pat, 100.0f, true};
  }

  {
    std::vector<std::pair<float, float>> pat;
    pat.push_back({0.0f, -1.49f});
    pat.push_back({0.39f, -1.49f});
    for (int i = 0; i < 55; i++)
      pat.push_back({0.72f, -1.49f});
    for (int i = 0; i < 40; i++)
      pat.push_back({0.0f, -1.49f});
    g_weaponData["M249"] = {pat, 100.0f, true};
  }

  g_weaponData["Semi Auto Rifle"] = {{{0.0f, -1.4f}}, 175.0f, false};
  g_weaponData["Revolver"] = {{{0.0f, -1.7f}}, 175.0f, false};
  g_weaponData["Semi Automatic Pistol"] = {{{0.0f, -0.95f}}, 150.0f, false};
  g_weaponData["M92"] = {{{0.0f, -3.0f}}, 150.0f, false};
  g_weaponData["M39"] = {{{0.9f, -1.6f}}, 175.0f, false};
  g_weaponData["Python"] = {{{0.0f, -5.8f}}, 150.0f, false};
  g_weaponData["Nail Gun"] = {{{0.2f, -2.1f}}, 150.0f, false};
}

float getMovementMultiplier() {
  if (!g_gameState.movementDetectionEnabled)
    return 1.0f;

  const bool isCrouch =
      (GetAsyncKeyState(g_gameState.crouchKey) & 0x8000) != 0;
  const bool keyW = (GetAsyncKeyState(g_gameState.moveUpKey) & 0x8000) != 0;
  const bool keyA = (GetAsyncKeyState(g_gameState.moveLeftKey) & 0x8000) != 0;
  const bool keyS = (GetAsyncKeyState(g_gameState.moveDownKey) & 0x8000) != 0;
  const bool keyD = (GetAsyncKeyState(g_gameState.moveRightKey) & 0x8000) != 0;

  // Treat opposite keys as a cancelled axis (W+S or A+D), rather than
  // classifying the player as moving when the effective input is zero.
  const bool isForwardBack = keyW != keyS;
  const bool isStrafing = keyA != keyD;
  const bool isMoving = isForwardBack || isStrafing;

  const bool isLmg = (g_gameState.currentWeapon == "M249" ||
                      g_gameState.currentWeapon == "HMLMG");

  if (isLmg && isMoving) {
    // These weapon-specific values are already movement multipliers.
    // Dividing them again made standing movement 0.8/1.75 = 0.457,
    // which magnified compensation by about 2.19x and was discontinuous
    // with every other weapon's moving value (0.85).
    return isCrouch ? 1.75f : 0.8f;
  }

  if (isCrouch && isMoving)
    return 1.8f;
  if (isCrouch && !isMoving)
    return 2.0f;
  if (!isCrouch && isMoving)
    return 0.85f;
  return 1.0f;
}

void updateScaledPattern() {
  auto it = g_weaponData.find(g_gameState.currentWeapon);
  if (it == g_weaponData.end())
    return;

  const auto &wdata = it->second;
  const auto &pattern = wdata.pattern;
  float delayMs = wdata.delay;

  auto scopeIt = g_scopes.find(g_gameState.currentScope);
  float scopeMult = (scopeIt != g_scopes.end()) ? scopeIt->second : 1.0f;

  auto barrelIt = g_barrels.find(g_gameState.currentBarrel);
  float barrelMult = (barrelIt != g_barrels.end()) ? barrelIt->second : 1.0f;

  float movementMult = getMovementMultiplier();
  float moveMultiplier =
      -0.03f *
      (g_gameState.sensitivity * g_gameState.adsSensitivity * movementMult) *
      3.0f * (g_gameState.fov / 100.0f);
  if (fabs(moveMultiplier) < 0.0001f)
    moveMultiplier = -0.02f;

  std::vector<std::pair<float, float>> scaled;
  scaled.reserve(pattern.size());
  for (const auto &[x, y] : pattern) {
    float pixelX =
        (x / moveMultiplier) * scopeMult * barrelMult * g_gameState.xSpeed;
    float pixelY =
        (y / moveMultiplier) * scopeMult * barrelMult * g_gameState.ySpeed;
    scaled.push_back({pixelX, pixelY});
  }

  std::lock_guard<std::mutex> lock(g_patternLock);
  g_scaledPattern = std::move(scaled);
  g_scaledDelayMs = delayMs;
}
