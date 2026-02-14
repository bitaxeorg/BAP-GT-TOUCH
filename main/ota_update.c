/**
 * OTA Update Module for BAP-GT-TOUCH
 */

#include "ota_update.h"
#include "esp_log.h"
#include "esp_https_ota.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_app_desc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "OTA";

#define OTA_STACK_SIZE 10240
#define VERSION_CHECK_STACK_SIZE 8192
#define GITHUB_API_URL "https://api.github.com/repos/bitaxeorg/BAP-GT-TOUCH/releases/latest"
#define FIRMWARE_DOWNLOAD_URL "https://github.com/bitaxeorg/BAP-GT-TOUCH/releases/latest/download/esp-display-ota.bin"

static const char *ALLOWED_URL_PREFIX = "https://github.com/bitaxeorg/BAP-GT-TOUCH";

static SemaphoreHandle_t ota_mutex = NULL;
static ota_info_t ota_current_info = {0};
static TaskHandle_t ota_task_handle = NULL;
static TaskHandle_t version_check_task_handle = NULL;

static void ota_init_mutex(void)
{
    if (ota_mutex == NULL) {
        ota_mutex = xSemaphoreCreateMutex();
        configASSERT(ota_mutex != NULL);
    }
}

static void ota_set_status(ota_status_t status, int progress, const char *error)
{
    ota_init_mutex();
    xSemaphoreTake(ota_mutex, portMAX_DELAY);

    ota_current_info.status = status;
    ota_current_info.progress_percent = progress;

    if (error) {
        strlcpy(ota_current_info.error_msg, error, sizeof(ota_current_info.error_msg));
    } else {
        ota_current_info.error_msg[0] = '\0';
    }

    const char *ver = ota_get_current_version();
    if (ver) {
        strlcpy(ota_current_info.current_version, ver, sizeof(ota_current_info.current_version));
    }

    xSemaphoreGive(ota_mutex);
}

static void ota_set_latest_version(const char *version)
{
    ota_init_mutex();
    xSemaphoreTake(ota_mutex, portMAX_DELAY);
    if (version) {
        strlcpy(ota_current_info.latest_version, version, sizeof(ota_current_info.latest_version));
    }
    xSemaphoreGive(ota_mutex);
}

static bool is_safe_url(const char *url)
{
    if (!url || strlen(url) == 0) {
        return false;
    }

    if (strncmp(url, ALLOWED_URL_PREFIX, strlen(ALLOWED_URL_PREFIX)) != 0) {
        ESP_LOGE(TAG, "URL not from allowed repository: %s", url);
        return false;
    }

    if (strstr(url, "../") || strstr(url, "..\\")) {
        ESP_LOGE(TAG, "URL contains path traversal");
        return false;
    }

    return true;
}

static int parse_version_number(const char *version)
{
    int major = 0, minor = 0, patch = 0;
    if (version[0] == 'v' || version[0] == 'V') {
        version++;
    }
    sscanf(version, "%d.%d.%d", &major, &minor, &patch);
    return (major << 16) | (minor << 8) | patch;
}

static bool is_newer_version(const char *latest, const char *current)
{
    int latest_num = parse_version_number(latest);
    int current_num = parse_version_number(current);
    ESP_LOGI(TAG, "Version comparison: latest=%d, current=%d", latest_num, current_num);
    return latest_num > current_num;
}

static char* extract_tag_from_json(const char *json, size_t len)
{
    const char *tag_key = "\"tag_name\"";
    char *tag_start = strstr(json, tag_key);
    if (!tag_start) {
        return NULL;
    }

    tag_start = strchr(tag_start, ':');
    if (!tag_start) {
        return NULL;
    }
    tag_start++;

    while (*tag_start && (*tag_start == ' ' || *tag_start == '\t' || *tag_start == '"')) {
        tag_start++;
    }

    char *tag_end = strchr(tag_start, '"');
    if (!tag_end) {
        return NULL;
    }

    size_t tag_len = tag_end - tag_start;
    char *tag = malloc(tag_len + 1);
    if (tag) {
        strncpy(tag, tag_start, tag_len);
        tag[tag_len] = '\0';
    }
    return tag;
}

static void version_check_task(void *param)
{
    ESP_LOGI(TAG, "Checking for firmware updates...");
    ota_set_status(OTA_STATUS_CHECKING, 0, NULL);

    esp_http_client_config_t config = {
        .url = GITHUB_API_URL,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 15000,
        .buffer_size = 2048,
        .user_agent = "BAP-GT-TOUCH-OTA",
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to init HTTP client");
        ota_set_status(OTA_STATUS_ERROR, 0, "HTTP init failed");
        goto cleanup;
    }

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        ota_set_status(OTA_STATUS_ERROR, 0, "Connection failed");
        goto cleanup;
    }

    int content_length = esp_http_client_fetch_headers(client);
    if (content_length < 0) {
        ESP_LOGE(TAG, "Failed to fetch headers");
        ota_set_status(OTA_STATUS_ERROR, 0, "Failed to fetch headers");
        goto cleanup;
    }

    int status = esp_http_client_get_status_code(client);
    if (status != 200) {
        ESP_LOGE(TAG, "HTTP status code: %d", status);
        ota_set_status(OTA_STATUS_ERROR, 0, "Server returned error");
        goto cleanup;
    }

    char *buffer = malloc(4096);
    if (!buffer) {
        ota_set_status(OTA_STATUS_ERROR, 0, "Out of memory");
        goto cleanup;
    }

    int total_read = 0;
    int read_len;
    while ((read_len = esp_http_client_read(client, buffer + total_read, 4096 - total_read - 1)) > 0) {
        total_read += read_len;
        if (total_read >= 4095) break;
    }
    buffer[total_read] = '\0';

    char *latest_tag = extract_tag_from_json(buffer, total_read);
    free(buffer);

    if (!latest_tag) {
        ESP_LOGE(TAG, "Failed to parse version from response");
        ota_set_status(OTA_STATUS_ERROR, 0, "Failed to parse version");
        goto cleanup;
    }

    ESP_LOGI(TAG, "Latest version: %s", latest_tag);
    ota_set_latest_version(latest_tag);

    const char *current_version = ota_get_current_version();
    ESP_LOGI(TAG, "Current version: %s", current_version ? current_version : "unknown");

    if (is_newer_version(latest_tag, current_version)) {
        ESP_LOGI(TAG, "New version available!");
        ota_set_status(OTA_STATUS_UPDATE_AVAILABLE, 0, NULL);
    } else {
        ESP_LOGI(TAG, "Already on latest version");
        ota_set_status(OTA_STATUS_NO_UPDATE, 0, NULL);
    }

    free(latest_tag);

cleanup:
    if (client) {
        esp_http_client_cleanup(client);
    }

    ota_init_mutex();
    xSemaphoreTake(ota_mutex, portMAX_DELAY);
    version_check_task_handle = NULL;
    xSemaphoreGive(ota_mutex);

    vTaskDelete(NULL);
}

static void ota_task(void *param)
{
    char *url = (char *)param;
    esp_err_t err;
    esp_https_ota_handle_t ota_handle = NULL;

    ESP_LOGI(TAG, "Starting OTA update from: %s", url);
    ota_set_status(OTA_STATUS_DOWNLOADING, 0, NULL);

    esp_http_client_config_t http_config = {
        .url = url,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 30000,
        .buffer_size = 8192,
        .buffer_size_tx = 2048,
        .keep_alive_enable = true,
        .max_redirection_count = 10,
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &http_config,
    };

    err = esp_https_ota_begin(&ota_config, &ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_https_ota_begin failed: %s", esp_err_to_name(err));
        ota_set_status(OTA_STATUS_ERROR, 0, "OTA begin failed");
        goto cleanup;
    }

    int image_size = esp_https_ota_get_image_size(ota_handle);
    ESP_LOGI(TAG, "Image size: %d bytes", image_size);

    ota_set_status(OTA_STATUS_FLASHING, 0, NULL);

    while (1) {
        err = esp_https_ota_perform(ota_handle);
        if (err == ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            int read_len = esp_https_ota_get_image_len_read(ota_handle);
            int progress = 0;
            if (image_size > 0) {
                progress = (read_len * 100) / image_size;
                if (progress > 100) progress = 100;
            }
            ota_set_status(OTA_STATUS_FLASHING, progress, NULL);

            if ((read_len & 0xFFFF) < 4096) {
                ESP_LOGI(TAG, "Progress: %d%% (%d/%d bytes)", progress, read_len, image_size);
            }
            continue;
        }
        if (err == ESP_OK) {
            break;
        }
        ESP_LOGE(TAG, "esp_https_ota_perform failed: %s", esp_err_to_name(err));
        ota_set_status(OTA_STATUS_ERROR, 0, "Download/flash failed");
        esp_https_ota_abort(ota_handle);
        ota_handle = NULL;
        goto cleanup;
    }

    err = esp_https_ota_finish(ota_handle);
    ota_handle = NULL;
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_https_ota_finish failed: %s", esp_err_to_name(err));
        ota_set_status(OTA_STATUS_ERROR, 0, "OTA validation failed");
        goto cleanup;
    }

    ota_set_status(OTA_STATUS_SUCCESS, 100, NULL);
    ESP_LOGI(TAG, "OTA update successful! Rebooting in 3 seconds...");

    vTaskDelay(pdMS_TO_TICKS(3000));
    esp_restart();

cleanup:
    if (ota_handle) {
        esp_https_ota_abort(ota_handle);
    }
    if (url) {
        free(url);
    }

    ota_init_mutex();
    xSemaphoreTake(ota_mutex, portMAX_DELAY);
    ota_task_handle = NULL;
    xSemaphoreGive(ota_mutex);

    vTaskDelete(NULL);
}

void ota_check_for_updates(void)
{
    ota_init_mutex();
    xSemaphoreTake(ota_mutex, portMAX_DELAY);

    if (version_check_task_handle != NULL || ota_task_handle != NULL) {
        ESP_LOGW(TAG, "Update check or OTA already in progress");
        xSemaphoreGive(ota_mutex);
        return;
    }

    xSemaphoreGive(ota_mutex);

    BaseType_t ret = xTaskCreate(
        version_check_task,
        "version_check",
        VERSION_CHECK_STACK_SIZE,
        NULL,
        5,
        &version_check_task_handle
    );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create version check task");
        ota_set_status(OTA_STATUS_ERROR, 0, "Task creation failed");
    }
}

esp_err_t ota_update_start_latest(void)
{
    return ota_update_start(FIRMWARE_DOWNLOAD_URL);
}

esp_err_t ota_update_start(const char *url)
{
    if (!url || strlen(url) == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!is_safe_url(url)) {
        ESP_LOGE(TAG, "URL validation failed");
        return ESP_ERR_INVALID_ARG;
    }

    ota_init_mutex();
    xSemaphoreTake(ota_mutex, portMAX_DELAY);

    if (ota_task_handle != NULL) {
        ESP_LOGW(TAG, "OTA update already in progress");
        xSemaphoreGive(ota_mutex);
        return ESP_FAIL;
    }

    xSemaphoreGive(ota_mutex);

    char *url_copy = strdup(url);
    if (!url_copy) {
        return ESP_ERR_NO_MEM;
    }

    BaseType_t ret = xTaskCreate(
        ota_task,
        "ota_task",
        OTA_STACK_SIZE,
        url_copy,
        5,
        &ota_task_handle
    );

    if (ret != pdPASS) {
        free(url_copy);
        return ESP_FAIL;
    }

    return ESP_OK;
}

void ota_update_get_info(ota_info_t *info)
{
    if (!info) return;

    ota_init_mutex();
    xSemaphoreTake(ota_mutex, portMAX_DELAY);
    memcpy(info, &ota_current_info, sizeof(ota_info_t));
    const char *ver = ota_get_current_version();
    if (ver && info->current_version[0] == '\0') {
        strlcpy(info->current_version, ver, sizeof(info->current_version));
    }
    xSemaphoreGive(ota_mutex);
}

bool ota_update_is_running(void)
{
    ota_init_mutex();
    xSemaphoreTake(ota_mutex, portMAX_DELAY);
    bool running = (ota_task_handle != NULL || version_check_task_handle != NULL);
    xSemaphoreGive(ota_mutex);
    return running;
}

const char* ota_get_current_version(void)
{
    const esp_app_desc_t *app_desc = esp_app_get_description();
    return app_desc->version;
}
