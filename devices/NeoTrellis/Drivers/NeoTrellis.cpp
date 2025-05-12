#include "Device.h"
#include "driver/i2c_master.h"
#include "include/NeoTrellis.h"
#include "rom/ets_sys.h"

namespace Device::NeoTrellis
{
    void Init()
    {
        // Enable power to i2C

        gpio_config_t io_conf;
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        io_conf.pin_bit_mask = (1ULL << neotrellis_i2c_pwr);
        gpio_config(&io_conf);
        gpio_set_level(neotrellis_i2c_pwr, 1);
        
        i2c_master_bus_config_t i2c_bus_config = {};
        i2c_bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
        i2c_bus_config.i2c_port = I2C_NUM_0;
        i2c_bus_config.scl_io_num = neotrellis_i2c_scl;
        i2c_bus_config.sda_io_num = neotrellis_i2c_sda;
        i2c_bus_config.glitch_ignore_cnt = 7;
        i2c_bus_config.flags.enable_internal_pullup = true;

        if (i2c_new_master_bus(&i2c_bus_config, &neotrellis_i2c_bus) != ESP_OK) {
            ESP_LOGE("NeoTrellis", "Failed to create I2C bus");
            // return;
        }

        for (int i = 0; i < 4; i++) 
        {
            ESP_LOGI("NeoTrellis", "Initializing NeoTrellis %d (Part 1)", i);
            // Probe NeoTrellis device
            bool found = false;
            for (int j = 0; j < 10; j++) 
            {
                if(i2c_master_probe(neotrellis_i2c_bus, neotrellis_i2c_addr[i], 10) == ESP_OK) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                ESP_LOGE("NeoTrellis", "Device %d not found", i);
                // return;
            }
            else {
                ESP_LOGI("NeoTrellis", "Device %d found", i);
            }

            // Add NeoTrellis device to I2C bus
            i2c_device_config_t i2c_dev_config = {};
            i2c_dev_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
            i2c_dev_config.device_address = neotrellis_i2c_addr[i];
            i2c_dev_config.scl_speed_hz = 400000;

            if (i2c_master_bus_add_device(neotrellis_i2c_bus, &i2c_dev_config, &neotrellis_i2c_dev[i]) != ESP_OK) {
                ESP_LOGE("NeoTrellis", "Device %d not added", i);
                // return;
            }
            else {
                ESP_LOGI("NeoTrellis", "Device %d added", i);
            }   

            // Reset NeoTrellis device
            {
                uint8_t cmd[] = { SEESAW_STATUS_BASE, SEESAW_STATUS_SWRST, 0xFF };
                if(i2c_master_transmit(neotrellis_i2c_dev[i], cmd, sizeof(cmd), NEO_TRELLIS_I2C_TIMEOUT_VALUE_MS) != ESP_OK) {
                    ESP_LOGE("NeoTrellis", "Failed to reset NeoTrellis %d", i);
                    // return;
                }
                else {
                    ESP_LOGI("NeoTrellis", "NeoTrellis %d reset successful", i);
                }
            }
        }

        for (int i = 0; i < 4; i++) 
        {
            ESP_LOGI("NeoTrellis", "Initializing NeoTrellis %d (Part 2)", i);

            // Reprobe NeoTrellis device
            bool found = false;
            for (int j = 0; j < 10; j++) 
            {
                if(i2c_master_probe(neotrellis_i2c_bus, neotrellis_i2c_addr[i], 10) == ESP_OK) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                ESP_LOGE("NeoTrellis", "Device %d not found after reset", i);
                // return;
            }
            else {
                ESP_LOGI("NeoTrellis", "Device %d found after reset", i);
            }

            // Check NeoTrellis device HW ID and is ready
            bool ready = false;
            int j = 0;
            for (; j < 10; j++)
            {
                uint8_t device_hw_id = 0;
                uint8_t cmd[] = { SEESAW_STATUS_BASE, SEESAW_STATUS_HW_ID };
                if(i2c_master_transmit(neotrellis_i2c_dev[i], cmd, sizeof(cmd), NEO_TRELLIS_I2C_TIMEOUT_VALUE_MS) != ESP_OK) {
                    continue;
                }

                ets_delay_us(250);

                if (i2c_master_receive(neotrellis_i2c_dev[i], &device_hw_id, sizeof(device_hw_id), NEO_TRELLIS_I2C_TIMEOUT_VALUE_MS) != ESP_OK) {
                    continue;
                }
                

                if (device_hw_id == SEESAW_HW_ID_CODE_SAMD09) {
                    ready = true;
                    break;
                }
            }

            if (!ready) {
                ESP_LOGE("NeoTrellis", "NeoTrellis %d not ready", i);
                // return;
            }

            ESP_LOGI("NeoTrellis", "NeoTrellis %d ready after %d tries", i, j);
        }
    }
}
        
