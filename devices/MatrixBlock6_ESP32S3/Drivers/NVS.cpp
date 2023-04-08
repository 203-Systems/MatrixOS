#include "Device.h"
#include "framework/Hash.h"

#define U32_TO_CHAR_ARRAY(value)                                                                                     \
  {                                                                                                                  \
    (char)((value >> 24) & 0xFF), (char)((value >> 16) & 0xFF), (char)((value >> 8) & 0xFF), (char)(value & 0xFF), 0 \
  }  // Little Endian

namespace Device::NVS
{
  nvs_handle_t nvs_handle;
  void Init() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
      // NVS partition was truncated and needs to be erased
      // Retry nvs_flash_init
      ESP_ERROR_CHECK(nvs_flash_erase());
      err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    nvs_open("matrix_os", NVS_READWRITE, &nvs_handle);
  }

  size_t Size(uint32_t hash) {
    char hash_array[5] = U32_TO_CHAR_ARRAY(hash);
    size_t length;

    if (nvs_get_blob(nvs_handle, hash_array, NULL, &length) != ESP_OK)
    { return -1; }
    return length;
  }

  vector<char> Read(uint32_t hash) {
    char hash_array[5] = U32_TO_CHAR_ARRAY(hash);
    size_t length;

    if (nvs_get_blob(nvs_handle, hash_array, NULL, &length) != ESP_OK)
    { return vector<char>(0); }
    vector<char> value(length);
    if (nvs_get_blob(nvs_handle, hash_array, value.data(), &length) != ESP_OK)
    { return vector<char>(0); }
    return value;
  }

  bool Write(uint32_t hash, void* pointer, uint16_t length) {
    char hash_array[5] = U32_TO_CHAR_ARRAY(hash);
    bool success = nvs_set_blob(nvs_handle, hash_array, pointer, length) == ESP_OK;
    nvs_commit(nvs_handle);
    return success;
  }

  bool Delete(uint32_t hash) {
    char hash_array[5] = U32_TO_CHAR_ARRAY(hash);
    bool success = nvs_erase_key(nvs_handle, hash_array) == ESP_OK;
    nvs_commit(nvs_handle);
    return success;
  }

  void Clear() {
    nvs_erase_all(nvs_handle);
    nvs_commit(nvs_handle);
  }
}