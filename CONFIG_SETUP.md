# WiFi Configuration Setup

This project uses a `.env` file to store WiFi credentials securely without hardcoding them in the source code.

## Setup Instructions

### 1. Create your .env file

Edit the `.env` file in the `data/` directory:

```bash
cd data
nano .env  # or use your preferred editor
```

Replace the placeholder values with your actual WiFi credentials:

```
WIFI_SSID=your-actual-wifi-name
WIFI_PASSWORD=your-actual-wifi-password
```

**Important:**
- Do not use quotes around the values unless they are part of your password
- Spaces in SSID/password are supported
- The `.env` file in `data/` will be uploaded to the ESP32's SPIFFS filesystem

### 2. Upload the filesystem

After editing the `.env` file, upload it to the ESP32's SPIFFS:

```bash
pio run --target uploadfs
```

This uploads all files from the `data/` directory to the ESP32's SPIFFS partition.

### 3. Build and upload the firmware

```bash
pio run --target upload
```

### 4. Monitor the output

```bash
pio device monitor
```

You should see logs indicating:
- Configuration loaded successfully
- WiFi SSID being used (password is hidden in logs)
- Connection status

## Security Notes

- **Never commit the `data/.env` file** with real credentials to version control
- The `.env.example` file in the root directory is a template only
- The `data/.env` file is listed in `.gitignore` to prevent accidental commits

## File Structure

```
EPaper/
├── .env.example          # Template (safe to commit)
├── data/
│   └── .env             # Actual credentials (DO NOT commit)
├── src/
│   ├── config/
│   │   ├── config.h
│   │   └── config.c     # Reads .env from SPIFFS
│   └── main.c           # Uses config for WiFi
└── partitions.csv       # Defines SPIFFS partition
```

## Troubleshooting

### "Failed to mount SPIFFS"
- Make sure you ran `pio run --target uploadfs` to upload the filesystem
- Check that `partitions.csv` is correctly configured

### "WIFI_SSID not found in .env file"
- Verify the `.env` file exists in the `data/` directory
- Check the format: `WIFI_SSID=value` (no spaces around `=`)
- Make sure you uploaded the filesystem after editing the file

### "WiFi connection failed"
- Double-check your SSID and password in `data/.env`
- Ensure your WiFi network is 2.4GHz (ESP32-C6 typically doesn't support 5GHz)
- Check that your WiFi is in range and working

## Alternative: Hardcoded Credentials (Not Recommended)

If you don't want to use `.env`, you can still hardcode credentials in `main.c`:

```c
// Instead of using config
wifi_mgr_connect("YourSSID", "YourPassword");
```

But this is less secure and not recommended for production or shared code.
