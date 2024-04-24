#include "my_spi.h"
//
#include "driver/spi_master.h"
#include "esp_intr_types.h"
// #include "esp_err.h"

void my_spi_init() {
  spi_bus_config_t spi_bus_config = {
      .mosi_io_num = SPI2_HOST_MOSI,
      // .data0_io_num = 0,
      .miso_io_num = -1,
      // .data1_io_num = -1,
      .sclk_io_num = SPI2_HOST_SCLK,
      // .quadwp_io_num = -1,
      .data2_io_num = -1,
      // .quadhd_io_num = -1,
      .data3_io_num = -1,
      .data4_io_num = -1,
      .data5_io_num = -1,
      .data6_io_num = -1,
      .data7_io_num = -1,
      .max_transfer_sz = 4092,
      .flags = 0,
      .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
      .intr_flags = 0,
  };
  ESP_ERROR_CHECK(
      spi_bus_initialize(SPI2_HOST, &spi_bus_config, SPI_DMA_CH_AUTO));
    
}
