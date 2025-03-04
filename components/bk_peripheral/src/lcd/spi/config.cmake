set(SPI_LCD_DEVICE_FILES "")
set(SPI_LCD_PATH src/lcd/spi)

if (CONFIG_LCD_SPI_ST7796U)
	list(APPEND SPI_LCD_DEVICE_FILES ${SPI_LCD_PATH}/lcd_spi_st7796u.c)
endif()

if (CONFIG_LCD_SPI_GC9D01)
	list(APPEND SPI_LCD_DEVICE_FILES ${SPI_LCD_PATH}/lcd_spi_gc9d01.c)
endif()
