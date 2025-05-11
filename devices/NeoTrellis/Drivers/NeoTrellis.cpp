#include "Device.h"
#include "driver/i2c_master.h"
#include "include/NeoTrellis.h"

namespace Device::NeoTrellis
{
    void Init()
    {
        i2c_master_bus_config_t i2c_bus_config = {};
        i2c_bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
        i2c_bus_config.i2c_port = I2C_NUM_0;
        i2c_bus_config.scl_io_num = neotrellis_i2c_scl;
        i2c_bus_config.sda_io_num = neotrellis_i2c_sda;
        i2c_bus_config.glitch_ignore_cnt = 7;
        i2c_bus_config.flags.enable_internal_pullup = true;

        if (i2c_new_master_bus(&i2c_bus_config, &neotrellis_i2c_bus) != ESP_OK) {
            // Throw error
            return;
        }

        for (int i = 0; i < 4; i++) 
        {
            // Probe NeoTrellis device
            if(i2c_master_probe(neotrellis_i2c_bus, neotrellis_i2c_addr[i], NEO_TRELLIS_I2C_TIMEOUT_VALUE_MS) != ESP_OK) {
                // MLOGE("NeoTrellis", "Device %d not found", i);
                // Throw error
                return;
            }

            // Add NeoTrellis device to I2C bus
            i2c_device_config_t i2c_dev_config = {};
            i2c_dev_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
            i2c_dev_config.device_address = neotrellis_i2c_addr[i];
            i2c_dev_config.scl_speed_hz = 400000;

            if (i2c_master_bus_add_device(neotrellis_i2c_bus, &i2c_dev_config, &neotrellis_i2c_dev[i]) != ESP_OK) {
                // MLOGE("NeoTrellis", "Device %d not added", i);
                // Throw error
                return;
            }

            // Reset NeoTrellis device
            {
                uint8_t cmd[] = { SEESAW_STATUS_BASE, SEESAW_STATUS_SWRST, 0xFF };
                if(i2c_master_transmit(neotrellis_i2c_dev[i], cmd, sizeof(cmd), NEO_TRELLIS_I2C_TIMEOUT_VALUE_MS) != ESP_OK) {
                    // MLOGE("NeoTrellis", "Failed to reset NeoTrellis %d", i);
                    // Throw error
                    return;
                }
            }
        }

        for (int i = 0; i < 4; i++) 
        {
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
                // MLOGE("NeoTrellis", "Device %d not found after reset", i);
                // Throw error
                return;
            }

            // Check NeoTrellis device HW ID and is ready
            bool ready = false;
            for (int j = 0; j < 10; j++)
            {
                uint8_t device_hw_id = 0;
                uint8_t cmd[] = { SEESAW_STATUS_BASE, SEESAW_STATUS_HW_ID };
                if(i2c_master_transmit_receive(neotrellis_i2c_dev[i], cmd, sizeof(cmd), &device_hw_id, 1, NEO_TRELLIS_I2C_TIMEOUT_VALUE_MS) != ESP_OK) {
                    // MLOGE("NeoTrellis", "Failed to read HW ID from NeoTrellis %d", i);
                    // Throw error
                    return;
                }

                if (device_hw_id == SEESAW_HW_ID_CODE_SAMD09) {
                    ready = true;
                    break;
                }
            }
            if (!ready) {
                // MLOGE("NeoTrellis", "NeoTrellis %d not ready", i);
                // Throw error
                return;
            }

            // MLOGD("NeoTrellis", "NeoTrellis %d ready", i);
        }
    }
}
        
