void flush_microphone()
{
  Serial.printf("[microphone] flushing and sending %d bytes of data\n", data_offset);
  if (data_offset == 0)
    return;
  webSocket.sendBIN(microphonedata0, data_offset);
  data_offset = 0;
}


void audio_recording_task(void *arg) {
  while (1) {
    if (recording) {
      Serial.printf("Reading chunk at %d...\n", data_offset);
      size_t bytes_read;
      i2s_read(
        SPEAKER_I2S_NUMBER,
        (char *)(microphonedata0 + data_offset),
        DATA_SIZE, &bytes_read, (100 / portTICK_RATE_MS));
      data_offset += bytes_read;
      Serial.printf("Read %d bytes in chunk.\n", bytes_read);

      // Only send here
      if (data_offset > MAX_DATA_LEN)
      {
        flush_microphone();
        delay(10);
      }
    }
    else {
      delay(100);    // Wait for recording event
    }
  }
}


enum ValueType {
  CHAR,
  UCHAR,
  SHORT,
  USHORT,
  INT,
  UINT,
  LONG,
  ULONG,
  LONG64,
  ULONG64,
  FLOAT,
  DOUBLE,
  BOOL,
  STRING,
  BYTES
};


// Generic template for typical types
template<typename T>
void setPreference(const char* ns, const char* key, T value, ValueType type) {
  preferences.begin(ns, false);  // Open in read/write mode
    Serial.print("Saving Key: "); Serial.println(key);
  switch (type) {
    case CHAR:
      preferences.putChar(key, static_cast<int8_t>(value));
      break;
    case UCHAR:
      preferences.putUChar(key, static_cast<uint8_t>(value));
      break;
      
    case SHORT:
      preferences.putShort(key, static_cast<int16_t>(value));
      break;
    case USHORT:
      preferences.putUShort(key, static_cast<uint16_t>(value));
      break;
    case INT:
      preferences.putInt(key, static_cast<int32_t>(value));
      break;
    case UINT:
      preferences.putUInt(key, static_cast<uint32_t>(value));
      break;
    case LONG:
    case ULONG:
      preferences.putLong(key, static_cast<int32_t>(value));
      break;
    case LONG64:
    case ULONG64:
      preferences.putLong64(key, static_cast<int64_t>(value));
      break;
    case FLOAT:
      preferences.putFloat(key, static_cast<float>(value));
      break;
    case DOUBLE:
      preferences.putDouble(key, static_cast<double>(value));
      break;
    case BOOL:
      preferences.putBool(key, static_cast<bool>(value));
      break;
    default:
      break;
  }

  preferences.end();
}

// Specialized for STRING
void setPreference(const char* ns, const char* key, const char* value, ValueType type) {
  if (type == STRING) {
    preferences.begin(ns, false);
    preferences.putString(key, value);
    preferences.end();
  }
}

// Specialized for BYTES
void setPreference(const char* ns, const char* key, const void* value, size_t len, ValueType type) {
  if (type == BYTES) {
    preferences.begin(ns, false);
    preferences.putBytes(key, value, len);
    preferences.end();
  }
}

template<typename T>
T getPreference(const char* ns, const char* key, T defaultValue, ValueType type) {
  preferences.begin(ns, true);  // Open in read-only mode

  T value = defaultValue;
  switch (type) {
    case CHAR:
      value = static_cast<T>(preferences.getChar(key, static_cast<int8_t>(defaultValue)));
      break;
    case UCHAR:
      value = static_cast<T>(preferences.getUChar(key, static_cast<uint8_t>(defaultValue)));      
      break;
    case SHORT:
      value = static_cast<T>(preferences.getShort(key, static_cast<int16_t>(defaultValue)));
      break;
    case USHORT:
      value = static_cast<T>(preferences.getUShort(key, static_cast<uint16_t>(defaultValue)));
      break;
    case INT:
      value = static_cast<T>(preferences.getInt(key, static_cast<int32_t>(defaultValue)));
      break;
    case UINT:
      value = static_cast<T>(preferences.getUInt(key, static_cast<uint32_t>(defaultValue)));
      break;
    case LONG:
    case ULONG:
      value = static_cast<T>(preferences.getLong(key, static_cast<int32_t>(defaultValue)));
      break;
    case LONG64:
    case ULONG64:
      value = static_cast<T>(preferences.getLong64(key, static_cast<int64_t>(defaultValue)));
      break;
    case FLOAT:
      value = static_cast<T>(preferences.getFloat(key, static_cast<float>(defaultValue)));
      break;
    case DOUBLE:
      value = static_cast<T>(preferences.getDouble(key, static_cast<double>(defaultValue)));
      break;
    case BOOL:
      value = static_cast<T>(preferences.getBool(key, static_cast<bool>(defaultValue)));
      break;
    default:
      break;
  }

  preferences.end();
  return value;
}

// Specialized for getting strings
const char* getStringPreference(const char* ns, const char* key, const char* defaultValue) {
  preferences.begin(ns, true);
  String value = preferences.getString(key, defaultValue);
  preferences.end();
  // NOTE: Return a copy or ensure the lifetime of the returned value is handled properly
  return strdup(value.c_str());
}

int levelToPercentage(uint8_t level) {
    return map(level, 0, 255, 0, 100);
}
