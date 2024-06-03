//SPEAKER
#define CONFIG_I2S_BCK_PIN 48
#define CONFIG_I2S_LRCK_PIN 15
#define CONFIG_I2S_DATA_OUT_PIN 46

//MIC
#define CONFIG_I2S_DATA_IN_PIN BOARD_MIC_DATA
#define CONFIG_I2S_DATA_WS_PIN BOARD_MIC_CLOCK

#define SPEAKER_I2S_NUMBER I2S_NUM_0
#define SAMPLE_RATE (16000) //  ( 44100)
#define SAMPLE_BITS 16

#define MODE_MIC 0
#define MODE_SPK 1

#define DATA_SIZE 1024

#define MAX_DATA_LEN (1024 * 9)

uint8_t microphonedata0[1024 * 10];
uint8_t speakerdata0[1024 * 1];
int speaker_offset;
int data_offset;

void InitI2SSpeakerOrMic(int mode)
{
    Serial.printf("InitI2sSpeakerOrMic %d\n", mode);
    esp_err_t err = ESP_OK;

    i2s_driver_uninstall(SPEAKER_I2S_NUMBER);
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample =
             i2s_bits_per_sample_t(SAMPLE_BITS), // is fixed at 12bit, stereo, MSB
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
#if ESP_IDF_VERSION > ESP_IDF_VERSION_VAL(4, 1, 0)
        .communication_format =
            i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB), // Set the format of the communication.
#else                                 
        .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
#endif
//        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
//        .dma_buf_count = 6,
//        .dma_buf_len = 60,
        .intr_alloc_flags = 0,
        .dma_buf_count = 64,
        .dma_buf_len = 1024,
    };
    if (mode == MODE_MIC)
    {
        i2s_config.mode =
            (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM);
            i2s_config.use_apll = 1;
    }
    else
    {
        i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
        i2s_config.use_apll = false;
        i2s_config.tx_desc_auto_clear = true;
    }

    err += i2s_driver_install(SPEAKER_I2S_NUMBER, &i2s_config, 0, NULL);
    i2s_pin_config_t tx_pin_config;

#if (ESP_IDF_VERSION > ESP_IDF_VERSION_VAL(4, 3, 0))
    tx_pin_config.mck_io_num = I2S_PIN_NO_CHANGE;
#endif
   if (mode == MODE_MIC)
    {
        tx_pin_config.bck_io_num = -1;
        tx_pin_config.ws_io_num = CONFIG_I2S_DATA_WS_PIN;
        tx_pin_config.data_out_num = -1;
        tx_pin_config.data_in_num = CONFIG_I2S_DATA_IN_PIN;
    }else{
      tx_pin_config.bck_io_num = CONFIG_I2S_BCK_PIN;
      tx_pin_config.ws_io_num = CONFIG_I2S_LRCK_PIN;
      tx_pin_config.data_out_num = CONFIG_I2S_DATA_OUT_PIN;
      tx_pin_config.data_in_num = -1;
    }
    
    
    err += i2s_set_pin(SPEAKER_I2S_NUMBER, &tx_pin_config);
    
//    err += i2s_set_clk(SPEAKER_I2S_NUMBER, 16000, I2S_BITS_PER_SAMPLE_16BIT,
//                    I2S_CHANNEL_MONO);
}

void maximize_volume(uint8_t *audio_data, size_t len) {
    int16_t *samples = (int16_t*)audio_data;
    size_t num_samples = len / sizeof(int16_t);

    for (size_t i = 0; i < num_samples; i++) {
        float sample = (float)samples[i];
        // Scale sample to max value - assume original data uses full dynamic range
        float max_sample_value = 32767.0f;
        float min_sample_value = -32768.0f;

        if (sample < 0) {
            samples[i] = (int16_t)(sample / min_sample_value * min_sample_value);
        } else {
            samples[i] = (int16_t)(sample / max_sample_value * max_sample_value);
        }
    }
}


void speaker_play(uint8_t *payload, uint32_t len)
{
    Serial.printf("received %lu bytes", len);
    size_t bytes_written;
    InitI2SSpeakerOrMic(MODE_SPK);
    i2s_write(SPEAKER_I2S_NUMBER, payload, len,
            &bytes_written, portMAX_DELAY);
}
