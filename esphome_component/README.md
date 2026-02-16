# ESPHome Component — E-Paper RWB 2.6" (152×296, tri-color)

Composant ESPHome externe pour l'écran e-paper tri-couleur (Rouge / Blanc / Noir) 2.6 pouces, 152×296 pixels, interface SPI.

## Structure

```
esphome_component/
└── epaper_rwb/
    ├── __init__.py         # (vide – requis par ESPHome)
    ├── display.py          # Configuration YAML → code C++
    ├── epaper_rwb.h        # Header C++
    └── epaper_rwb.cpp      # Implémentation du driver
```

## Configuration YAML

```yaml
external_components:
  - source:
      type: local
      path: esphome_component
    components: [epaper_rwb]

spi:
  clk_pin: GPIO19
  mosi_pin: GPIO18

font:
  - file: "gfonts://Roboto"
    id: roboto_16
    size: 16

  - file: "gfonts://Roboto@700"
    id: roboto_16_bold
    size: 16

color:
  - id: color_red
    red: 100%
    green: 0%
    blue: 0%

display:
  - platform: epaper_rwb
    id: my_epaper
    cs_pin: GPIO20
    dc_pin: GPIO0
    reset_pin: GPIO1
    busy_pin: GPIO2
    power_pin:
      number: GPIO14
      inverted: true        # Active-low power enable
    update_interval: 60s
    rotation: 0              # 0, 90, 180, 270
    lambda: |-
      // Fond blanc (auto_clear par défaut)

      // Rectangle noir
      it.rectangle(10, 68, 180, 26, COLOR_ON);
      it.filled_rectangle(11, 69, 178, 24, COLOR_ON);

      // Texte blanc sur fond noir
      it.print(22, 73, id(roboto_16_bold), COLOR_OFF, "Hello World");

      // Rectangle rouge
      it.filled_rectangle(10, 110, 180, 30, id(color_red));

      // Texte noir sur fond rouge
      it.print(22, 115, id(roboto_16), COLOR_ON, "E-Paper RWB");
```

## Couleurs disponibles

| Couleur | Code dans le lambda           | Résultat à l'écran |
|---------|-------------------------------|---------------------|
| Noir    | `COLOR_ON`                    | Pixel noir          |
| Blanc   | `COLOR_OFF`                   | Pixel blanc         |
| Rouge   | `Color(255, 0, 0)` ou `id()` | Pixel rouge         |

Logique de détection :
- `r > 127 && g < 64 && b < 64` → **Rouge**
- Toute autre couleur non-nulle → **Noir**
- `Color(0,0,0)` / `COLOR_OFF` → **Blanc**

## Câblage (ESP32-C6)

| Signal | GPIO | Couleur fil |
|--------|------|-------------|
| MOSI   | 18   | Violet      |
| CLK    | 19   | Bleu        |
| CS     | 20   | Blanc       |
| DC     | 0    | Jaune       |
| RST    | 1    | Orange      |
| BUSY   | 2    | Brun        |
| PWR    | 14   | Vert        |

## Paramètres optionnels

| Paramètre       | Défaut | Description                                     |
|------------------|--------|-------------------------------------------------|
| `reset_pin`      | —      | Pin de reset hardware                            |
| `busy_pin`       | —      | Pin BUSY (sans → délai fixe 200ms)              |
| `power_pin`      | —      | Pin d'alimentation display                       |
| `update_interval`| 60s    | Fréquence de rafraîchissement                    |
| `rotation`       | 0      | Rotation de l'affichage (0, 90, 180, 270)        |
| `auto_clear_enabled` | true | Efface le buffer avant chaque rendu          |

## Fonctions de dessin (héritées de ESPHome Display)

Toutes les fonctions standard ESPHome sont disponibles dans le lambda :

```cpp
it.print(x, y, font, color, "text");
it.printf(x, y, font, color, "%.1f°C", temp);
it.rectangle(x, y, w, h, color);
it.filled_rectangle(x, y, w, h, color);
it.circle(x, y, radius, color);
it.filled_circle(x, y, radius, color);
it.line(x1, y1, x2, y2, color);
it.image(x, y, image_id);
it.graph(x, y, graph_id);
// ... etc.
```
