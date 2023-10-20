
void mic_record() {
  unsigned int bytes_read = 0;

  i2s_read(SPEAKER_I2S_NUMBER,
           (char*)buffer + rPtr,
           BLOCK_SIZE,  // Number of bytes
           &bytes_read,
           portMAX_DELAY);  // No timeout

  rPtr = rPtr + bytes_read;
  // Wrap this when we get to the end of the buffer
  if (rPtr > 2043) rPtr = 0;
}