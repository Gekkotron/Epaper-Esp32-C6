/*uint16_t pin = PIN_NUM_PWR;

gpio_config_t io_conf = {
    .intr_type = GPIO_INTR_DISABLE,
    .mode = GPIO_MODE_OUTPUT,
    .pin_bit_mask = (1ULL << pin),
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .pull_up_en = GPIO_PULLUP_DISABLE
};
gpio_config(&io_conf);
for(int i = 0; i < 10; i++) {
    gpio_set_level(pin, 1); 
    vTaskDelay(pdMS_TO_TICKS(1000));
    gpio_set_level(pin, 0);
    vTaskDelay(pdMS_TO_TICKS(1000));
}*/