#include "settings_manager.h"
#include "game_data.h"
#include <Windows.h>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

static std::string jsonEscape(const std::string &s) {
  std::string out;
  for (char c : s) {
    if (c == '"')
      out += "\\\"";
    else if (c == '\\')
      out += "\\\\";
    else
      out += c;
  }
  return out;
}

static std::string trim(const std::string &s) {
  size_t a = s.find_first_not_of(" \t\r\n");
  if (a == std::string::npos)
    return "";
  size_t b = s.find_last_not_of(" \t\r\n");
  return s.substr(a, b - a + 1);
}

static std::string extractJsonString(const std::string &json,
                                     const std::string &key,
                                     const std::string &def = "") {
  std::string needle = "\"" + key + "\"";
  auto pos = json.find(needle);
  if (pos == std::string::npos)
    return def;
  pos = json.find(':', pos + needle.size());
  if (pos == std::string::npos)
    return def;
  pos = json.find('"', pos + 1);
  if (pos == std::string::npos)
    return def;
  auto end = json.find('"', pos + 1);
  if (end == std::string::npos)
    return def;
  return json.substr(pos + 1, end - pos - 1);
}

static float extractJsonFloat(const std::string &json, const std::string &key,
                              float def = 0.0f) {
  std::string needle = "\"" + key + "\"";
  auto pos = json.find(needle);
  if (pos == std::string::npos)
    return def;
  pos = json.find(':', pos + needle.size());
  if (pos == std::string::npos)
    return def;
  pos++;
  while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t'))
    pos++;
  std::string numStr;
  while (pos < json.size() && (json[pos] == '-' || json[pos] == '.' ||
                               (json[pos] >= '0' && json[pos] <= '9')))
    numStr += json[pos++];
  if (numStr.empty())
    return def;
  try {
    return std::stof(numStr);
  } catch (...) {
    return def;
  }
}

static int extractJsonInt(const std::string &json, const std::string &key,
                          int def = 0) {
  return (int)extractJsonFloat(json, key, (float)def);
}

static bool extractJsonBool(const std::string &json, const std::string &key,
                            bool def = false) {
  std::string needle = "\"" + key + "\"";
  auto pos = json.find(needle);
  if (pos == std::string::npos)
    return def;
  pos = json.find(':', pos + needle.size());
  if (pos == std::string::npos)
    return def;
  auto rest = json.substr(pos + 1);
  rest = trim(rest);
  if (rest.substr(0, 4) == "true")
    return true;
  if (rest.substr(0, 5) == "false")
    return false;
  return def;
}

static std::vector<std::string> extractJsonArray(const std::string &json,
                                                  const std::string &key) {
  std::vector<std::string> result;
  std::string needle = "\"" + key + "\"";
  auto pos = json.find(needle);
  if (pos == std::string::npos)
    return result;
  pos = json.find('[', pos + needle.size());
  if (pos == std::string::npos)
    return result;

  int depth = 0;
  size_t arrStart = pos;
  for (size_t i = pos; i < json.size(); i++) {
    if (json[i] == '[')
      depth++;
    else if (json[i] == ']') {
      depth--;
      if (depth == 0) {

        std::string inner = json.substr(arrStart + 1, i - arrStart - 1);
        int objDepth = 0;
        size_t objStart = 0;
        bool inObj = false;
        for (size_t j = 0; j < inner.size(); j++) {
          if (inner[j] == '{') {
            if (!inObj) {
              objStart = j;
              inObj = true;
            }
            objDepth++;
          } else if (inner[j] == '}') {
            objDepth--;
            if (objDepth == 0 && inObj) {
              result.push_back(inner.substr(objStart, j - objStart + 1));
              inObj = false;
            }
          }
        }
        break;
      }
    }
  }
  return result;
}

std::string getSettingsPath() {
  char exePath[MAX_PATH];
  GetModuleFileNameA(NULL, exePath, MAX_PATH);
  std::string dir(exePath);
  auto lastSlash = dir.rfind('\\');
  if (lastSlash != std::string::npos)
    dir = dir.substr(0, lastSlash);
  return dir + "\\saved_settings.json";
}

void loadSettings() {
  std::string path = getSettingsPath();
  std::ifstream file(path);
  if (!file.is_open()) {
    printf("[SETTINGS] No config found at %s, using defaults\n", path.c_str());
    return;
  }

  std::stringstream ss;
  ss << file.rdbuf();
  std::string json = ss.str();
  file.close();

  g_gameState.sensitivity =
      extractJsonFloat(json, "sensitivity", g_gameState.sensitivity);
  g_gameState.adsSensitivity =
      extractJsonFloat(json, "ads_sensitivity", g_gameState.adsSensitivity);
  g_gameState.fov = extractJsonFloat(json, "fov", g_gameState.fov);
  g_gameState.xSpeed = extractJsonFloat(json, "x_speed", g_gameState.xSpeed);
  g_gameState.ySpeed = extractJsonFloat(json, "y_speed", g_gameState.ySpeed);
  g_gameState.smoothingSteps =
      extractJsonInt(json, "smoothing_steps", g_gameState.smoothingSteps);
  g_gameState.hipfireEnabled =
      extractJsonBool(json, "hipfire_enabled", g_gameState.hipfireEnabled);
  g_gameState.movementDetectionEnabled = extractJsonBool(
      json, "movement_detection", g_gameState.movementDetectionEnabled);
  g_gameState.themeIndex =
      extractJsonInt(json, "theme_index", g_gameState.themeIndex);
  g_gameState.orbsEnabled =
      extractJsonBool(json, "orbs_enabled", g_gameState.orbsEnabled);
  g_gameState.inputMethod =
      extractJsonInt(json, "input_method", g_gameState.inputMethod);

  std::string mode = extractJsonString(json, "script_mode", "legit");
  g_gameState.scriptMode = (mode == "blatant") ? 1 : 0;

  g_gameState.toggleKey =
      extractJsonInt(json, "toggle_key", g_gameState.toggleKey);
  g_gameState.hideMenuKey =
      extractJsonInt(json, "hide_menu_key", g_gameState.hideMenuKey);
  g_gameState.modeSwitchKey =
      extractJsonInt(json, "mode_switch_key", g_gameState.modeSwitchKey);

  g_gameState.crouchKey = extractJsonInt(json, "crouch_key", g_gameState.crouchKey);
  g_gameState.moveLeftKey = extractJsonInt(json, "move_left_key", g_gameState.moveLeftKey);
  g_gameState.moveDownKey = extractJsonInt(json, "move_down_key", g_gameState.moveDownKey);
  g_gameState.moveRightKey = extractJsonInt(json, "move_right_key", g_gameState.moveRightKey);
  g_gameState.moveUpKey = extractJsonInt(json, "move_up_key", g_gameState.moveUpKey);

  g_gameState.customKeybinds.clear();
  auto bindObjs = extractJsonArray(json, "custom_keybinds");
  for (auto &obj : bindObjs) {
    CustomKeybind cb;
    cb.id = extractJsonString(obj, "id", "");
    cb.actionType = extractJsonString(obj, "action_type", "option");
    cb.actionValue = extractJsonString(obj, "action_value", "Master Toggle");
    cb.keybind = extractJsonInt(obj, "keybind", 0);
    cb.type = extractJsonString(obj, "type", "toggle");
    cb.enabled = extractJsonBool(obj, "enabled", true);
    cb.presetWeapon = extractJsonString(obj, "preset_weapon", "AK-47");
    cb.presetScope = extractJsonString(obj, "preset_scope", "None");
    cb.presetBarrel = extractJsonString(obj, "preset_barrel", "None");
    cb.useCustomSpeed = extractJsonBool(obj, "use_custom_speed", false);
    cb.presetXSpeed = extractJsonFloat(obj, "preset_x_speed", 1.0f);
    cb.presetYSpeed = extractJsonFloat(obj, "preset_y_speed", 1.0f);
    g_gameState.customKeybinds.push_back(cb);
  }

  printf("[SETTINGS] Loaded from %s\n", path.c_str());
}

void saveSettings() {
  std::string path = getSettingsPath();
  std::ofstream file(path);
  if (!file.is_open()) {
    printf("[SETTINGS] Failed to save to %s\n", path.c_str());
    return;
  }

  file << "{\n";
  file << "    \"sensitivity\": " << g_gameState.sensitivity << ",\n";
  file << "    \"ads_sensitivity\": " << g_gameState.adsSensitivity << ",\n";
  file << "    \"fov\": " << g_gameState.fov << ",\n";
  file << "    \"x_speed\": " << g_gameState.xSpeed << ",\n";
  file << "    \"y_speed\": " << g_gameState.ySpeed << ",\n";
  file << "    \"smoothing_steps\": " << g_gameState.smoothingSteps << ",\n";
  file << "    \"hipfire_enabled\": "
       << (g_gameState.hipfireEnabled ? "true" : "false") << ",\n";
  file << "    \"movement_detection\": "
       << (g_gameState.movementDetectionEnabled ? "true" : "false") << ",\n";
  file << "    \"script_mode\": \""
       << (g_gameState.scriptMode == 0 ? "legit" : "blatant") << "\",\n";
  file << "    \"theme_index\": " << g_gameState.themeIndex << ",\n";
  file << "    \"orbs_enabled\": "
       << (g_gameState.orbsEnabled ? "true" : "false") << ",\n";
  file << "    \"input_method\": " << g_gameState.inputMethod << ",\n";
  file << "    \"toggle_key\": " << g_gameState.toggleKey << ",\n";
  file << "    \"hide_menu_key\": " << g_gameState.hideMenuKey << ",\n";
  file << "    \"mode_switch_key\": " << g_gameState.modeSwitchKey << ",\n";
  file << "    \"crouch_key\": " << g_gameState.crouchKey << ",\n";
  file << "    \"move_left_key\": " << g_gameState.moveLeftKey << ",\n";
  file << "    \"move_down_key\": " << g_gameState.moveDownKey << ",\n";
  file << "    \"move_right_key\": " << g_gameState.moveRightKey << ",\n";
  file << "    \"move_up_key\": " << g_gameState.moveUpKey << ",\n";

  file << "    \"custom_keybinds\": [\n";
  for (size_t i = 0; i < g_gameState.customKeybinds.size(); i++) {
    auto &cb = g_gameState.customKeybinds[i];
    file << "        {\n";
    file << "            \"id\": \"" << jsonEscape(cb.id) << "\",\n";
    file << "            \"action_type\": \"" << jsonEscape(cb.actionType) << "\",\n";
    file << "            \"action_value\": \"" << jsonEscape(cb.actionValue) << "\",\n";
    file << "            \"keybind\": " << cb.keybind << ",\n";
    file << "            \"type\": \"" << jsonEscape(cb.type) << "\",\n";
    file << "            \"enabled\": " << (cb.enabled ? "true" : "false") << ",\n";
    file << "            \"preset_weapon\": \"" << jsonEscape(cb.presetWeapon) << "\",\n";
    file << "            \"preset_scope\": \"" << jsonEscape(cb.presetScope) << "\",\n";
    file << "            \"preset_barrel\": \"" << jsonEscape(cb.presetBarrel) << "\",\n";
    file << "            \"use_custom_speed\": " << (cb.useCustomSpeed ? "true" : "false") << ",\n";
    file << "            \"preset_x_speed\": " << cb.presetXSpeed << ",\n";
    file << "            \"preset_y_speed\": " << cb.presetYSpeed << "\n";
    file << "        }" << (i + 1 < g_gameState.customKeybinds.size() ? "," : "") << "\n";
  }
  file << "    ]\n";
  file << "}\n";
  file.close();

  printf("[SETTINGS] Saved to %s\n", path.c_str());
}
