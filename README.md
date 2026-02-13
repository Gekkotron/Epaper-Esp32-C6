# E-Paper Display - WiFi API Controller

A full-featured ESP32-C6 firmware for controlling tri-color e-paper displays via WiFi with a web interface and REST API.

## 🎯 Features

- **Web Interface** - User-friendly control panel for text placement and styling
- **REST API** - Full HTTP API for programmatic control
- **Multiple Fonts** - Three font sizes (Small 5x8, Medium 6x12, Large 8x16)
- **Text Orientation** - Support for 0°, 90°, 180°, and 270° rotation
- **Special Characters** - Includes °, é, è for temperature and French text
- **Tri-Color Display** - Support for black, red, and white colors
- **Text Scaling** - Variable text size (1x to 5x)
- **Secure Config** - WiFi credentials stored in .env file

## 📋 Hardware Requirements

- **ESP32-C6** development board (Cezerio Mini Dev or similar)
- **UC81xx E-Paper Display** (2.6" tri-color, 152x296 pixels)
- **SPI Connection**:
  - MOSI: GPIO 18 (Violet)
  - SCK: GPIO 19 (Bleu)
  - CS: GPIO 20 (Blanc)
  - DC: GPIO 0 (Jaune)
  - RST: GPIO 1 (Orange)
  - BUSY: GPIO 2 (Brun)
  - PWR: GPIO 14 (Vert)

## 🚀 Quick Start

### 1. Setup WiFi Credentials

Edit `data/.env` with your WiFi information:

```env
WIFI_SSID=your-wifi-name
WIFI_PASSWORD=your-wifi-password
```

### 2. Upload Filesystem

```bash
pio run --target uploadfs
```

### 3. Build and Flash

```bash
pio run --target upload
pio device monitor
```

### 4. Access Web Interface

Once connected, the display will show the IP address. Open in your browser:

```
http://[IP_ADDRESS]
```

## 🌐 API Reference

### Base URL

```
http://[IP_ADDRESS]
```

### Endpoints

#### 1. Display Single Text

**POST** `/api/text`

Display a single text string with optional clearing.

**Request Body:**
```json
{
  "text": "Hello World",
  "x": 10,
  "y": 10,
  "color": 1,
  "scale": 2,
  "clear": true
}
```

**Parameters:**
| Parameter | Type | Description | Default |
|-----------|------|-------------|---------|
| `text` | string | Text to display | "Hello" |
| `x` | integer | X position (0-151) | 10 |
| `y` | integer | Y position (0-295) | 10 |
| `color` | integer | 0=White, 1=Black, 2=Red | 1 |
| `scale` | integer | Text size multiplier (1-5) | 1 |
| `clear` | boolean | Clear screen before drawing | true |

**Response:**
```json
{
  "success": true,
  "message": "Text displayed"
}
```

**Example (curl):**
```bash
curl -X POST http://192.168.1.100/api/text \
  -H "Content-Type: application/json" \
  -d '{
    "text": "Temperature: 25°C",
    "x": 10,
    "y": 10,
    "color": 1,
    "scale": 2
  }'
```

---

#### 2. Display Multiple Texts

**POST** `/api/multi`

Display multiple text elements with different fonts, orientations, and styles.

**Request Body:**
```json
{
  "texts": [
    {
      "text": "Title",
      "x": 10,
      "y": 10,
      "color": 1,
      "scale": 2,
      "font": 1,
      "orientation": 0
    },
    {
      "text": "Subtitle",
      "x": 10,
      "y": 35,
      "color": 2,
      "scale": 1,
      "font": 0,
      "orientation": 0
    }
  ]
}
```

**Text Object Parameters:**
| Parameter | Type | Description | Values |
|-----------|------|-------------|--------|
| `text` | string | Text to display | Any string |
| `x` | integer | X position (0-151) | Required |
| `y` | integer | Y position (0-295) | Required |
| `color` | integer | Text color | 0=White, 1=Black, 2=Red |
| `scale` | integer | Size multiplier | 1-5 |
| `font` | integer | Font size | 0=Small (5x8), 1=Medium (6x12), 2=Large (8x16) |
| `orientation` | integer | Text rotation | 0=0°, 1=90°, 2=180°, 3=270° |

**Response:**
```json
{
  "success": true,
  "message": "3 texts displayed"
}
```

**Example (curl):**
```bash
curl -X POST http://192.168.1.100/api/multi \
  -H "Content-Type: application/json" \
  -d '{
    "texts": [
      {
        "text": "Café: 22°C",
        "x": 10,
        "y": 10,
        "color": 1,
        "scale": 2,
        "font": 1,
        "orientation": 0
      },
      {
        "text": "Humidity: 65%",
        "x": 10,
        "y": 40,
        "color": 2,
        "scale": 1,
        "font": 0,
        "orientation": 0
      }
    ]
  }'
```

**Example (Python):**
```python
import requests
import json

url = "http://192.168.1.100/api/multi"
data = {
    "texts": [
        {
            "text": "Temperature",
            "x": 10,
            "y": 10,
            "color": 1,
            "scale": 2,
            "font": 1,
            "orientation": 0
        },
        {
            "text": "25°C",
            "x": 10,
            "y": 40,
            "color": 2,
            "scale": 3,
            "font": 2,
            "orientation": 0
        }
    ]
}

response = requests.post(url, json=data)
print(response.json())
```

---

#### 3. Clear Display

**POST** `/api/clear`

Clear the entire display to white.

**Request Body:**
```json
{}
```

**Response:**
```json
{
  "success": true,
  "message": "Display cleared"
}
```

**Example:**
```bash
curl -X POST http://192.168.1.100/api/clear
```

---

#### 4. Draw Rectangle

**POST** `/api/rect`

Draw a filled rectangle on the display.

**Request Body:**
```json
{
  "x": 10,
  "y": 10,
  "w": 50,
  "h": 30,
  "color": 1,
  "clear": false
}
```

**Parameters:**
| Parameter | Type | Description | Default |
|-----------|------|-------------|---------|
| `x` | integer | X position | 0 |
| `y` | integer | Y position | 0 |
| `w` | integer | Width in pixels | 50 |
| `h` | integer | Height in pixels | 50 |
| `color` | integer | 0=White, 1=Black, 2=Red | 1 |
| `clear` | boolean | Clear before drawing | false |

**Response:**
```json
{
  "success": true,
  "message": "Rectangle drawn"
}
```

**Example:**
```bash
curl -X POST http://192.168.1.100/api/rect \
  -H "Content-Type: application/json" \
  -d '{
    "x": 10,
    "y": 10,
    "w": 100,
    "h": 50,
    "color": 2
  }'
```

---

#### 5. Web Interface

**GET** `/`

Returns an HTML web interface for visual control of the display.

Features:
- Add/remove multiple text elements
- Configure position, color, scale, font, and orientation
- Live preview of settings
- Quick examples
- Update display button

---

## 📝 Font Information

### Small Font (Font 0)
- **Size:** 5x8 pixels per character
- **Use case:** Maximum text density, status messages
- **Spacing:** 6 pixels between characters

### Medium Font (Font 1) - Default
- **Size:** 6x12 pixels per character
- **Use case:** Balanced readability and space
- **Spacing:** 7 pixels between characters

### Large Font (Font 2)
- **Size:** 8x16 pixels per character
- **Use case:** Titles, important information
- **Spacing:** 9 pixels between characters

### Special Characters
All fonts support:
- **Degree symbol:** ° (UTF-8: `\u00B0`)
- **French accents:** é, è (UTF-8: `\u00E9`, `\u00E8`)

**Example with special chars:**
```json
{
  "text": "Température: 25°C",
  "x": 10,
  "y": 10,
  "color": 1,
  "scale": 1,
  "font": 1,
  "orientation": 0
}
```

---

## 🔄 Text Orientation

Text can be rotated to any of four orientations:

| Orientation | Value | Rotation | Use Case |
|-------------|-------|----------|----------|
| Normal | 0 | 0° | Standard horizontal text |
| Clockwise | 1 | 90° | Vertical text, top-to-bottom |
| Inverted | 2 | 180° | Upside-down, right-to-left |
| Counter-CW | 3 | 270° | Vertical text, bottom-to-top |

**Notes:**
- Position (x, y) is the starting point in the cursor coordinate system
- For 180° rotation, use larger x values (near screen width) for text to appear on-screen
- For 90°/270°, text advances perpendicular to normal direction

---

## 🎨 Display Specifications

- **Resolution:** 152 x 296 pixels (width x height)
- **Colors:** 3 (Black, Red, White)
- **Controller:** UC81xx series
- **Refresh Time:** ~15 seconds for full update
- **Partial Updates:** Not supported (full refresh only)

---

## 📁 Project Structure

```
EPaper/
├── src/
│   ├── main.c              # Main application entry
│   ├── config/
│   │   ├── config.h        # Configuration interface
│   │   └── config.c        # .env parser and loader
│   ├── epaper/
│   │   ├── epaper.h        # E-paper driver interface
│   │   ├── epaper.c        # Display driver implementation
│   │   ├── font5x7.c/h     # Small font (5x8)
│   │   ├── font6x12.c/h    # Medium font (6x12)
│   │   └── font8x16.c/h    # Large font (8x16)
│   ├── wifi/
│   │   ├── wifi.h          # WiFi manager interface
│   │   └── wifi.c          # WiFi connection handler
│   └── webserver/
│       ├── webserver.h     # Web server interface
│       └── webserver.c     # HTTP API and web UI
├── data/
│   └── .env                # WiFi credentials (gitignored)
├── platformio.ini          # Build configuration
├── partitions.csv          # Flash partition table
├── README.md               # This file
└── CONFIG_SETUP.md         # WiFi setup guide
```

---

## 🔧 Configuration

See [CONFIG_SETUP.md](CONFIG_SETUP.md) for detailed WiFi configuration instructions.

**Quick summary:**
1. Edit `data/.env` with your WiFi credentials
2. Run `pio run --target uploadfs` to upload the filesystem
3. Run `pio run --target upload` to flash firmware

**Fallback:** If .env is not configured, the firmware falls back to hardcoded credentials for testing.

---

## 🐛 Troubleshooting

### Display not updating
- Check SPI connections
- Verify power supply (3.3V, adequate current)
- Monitor serial output for errors

### WiFi connection failed
- Verify credentials in `data/.env`
- Ensure 2.4GHz WiFi (ESP32-C6 doesn't support 5GHz)
- Check WiFi signal strength

### SPIFFS mount failed
- Run `pio run --target uploadfs` to upload filesystem
- If that fails, try `pio run --target erase` first
- Check that partition table is correctly configured

### API requests timeout
- Verify ESP32 IP address (shown on display after boot)
- Check that device is on same network
- Display updates take ~15 seconds, increase timeout if needed

### Text appears garbled or cut off
- Check text encoding (use UTF-8)
- Verify coordinates are within screen bounds (0-151, 0-295)
- Ensure scale factor doesn't make text exceed screen size

---

## 📡 Network Integration Examples

### Home Assistant
```yaml
rest_command:
  epaper_display:
    url: "http://192.168.1.100/api/multi"
    method: POST
    content_type: "application/json"
    payload: >
      {
        "texts": [
          {
            "text": "{{ states('sensor.temperature') }}°C",
            "x": 10,
            "y": 10,
            "color": 1,
            "scale": 2,
            "font": 1,
            "orientation": 0
          }
        ]
      }
```

### Node-RED
```javascript
// In a function node
msg.payload = {
    texts: [
        {
            text: "Weather: " + msg.payload.temperature + "°C",
            x: 10,
            y: 10,
            color: 1,
            scale: 2,
            font: 1,
            orientation: 0
        },
        {
            text: msg.payload.condition,
            x: 10,
            y: 40,
            color: 2,
            scale: 1,
            font: 0,
            orientation: 0
        }
    ]
};
return msg;

// Send to HTTP request node with URL: http://192.168.1.100/api/multi
```

---

## 🔐 Security Notes

- WiFi credentials are stored in SPIFFS, not in source code
- `data/.env` is gitignored to prevent credential leaks
- No authentication on API (intended for local network use)
- Consider using firewall rules to restrict access if needed

---

## 📜 License

This project is provided as-is for educational and personal use.

---

## 🤝 Contributing

Issues and improvements are welcome! When submitting changes:
1. Test on actual hardware
2. Document any new API endpoints
3. Update README if adding features

---

## 📞 Support

For issues or questions:
1. Check troubleshooting section above
2. Review serial monitor output for detailed logs
3. Verify hardware connections
4. Ensure latest firmware is flashed

---

**Made with ❤️ for ESP32-C6 and UC81xx E-Paper displays**
