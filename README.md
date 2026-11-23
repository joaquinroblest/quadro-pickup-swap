# Quadro Pickup Swap

Plugin para guitarra eléctrica que hace que una cápsula suene como otra, por ejemplo una bobina de humbucker en split como una single coil de Strato. Viene en formato **Audio Unit** (Logic Pro) y **VST3**, para Mac (Apple Silicon e Intel).

Solo usa filtros, así que **no agrega retraso**: se puede tocar en vivo.

## Controles

| Control | Qué hace |
|---|---|
| **Desde** | La cápsula que tiene la guitarra que estás tocando. |
| **Hacia** | La cápsula que quieres escuchar. |
| **Mezcla** | Cuánto se oye la señal convertida (100 % = solo la conversión). |
| **Brillo** | Ajuste fino de agudos, ±6 dB desde unos 5 kHz hacia arriba. |
| **Salida** | Volumen final. |

Si **Desde** y **Hacia** son iguales, el plugin no cambia nada.

## Interfaz

Diseñada con el sistema de marca de Quadro (brandboard naranjo): fondo negro, acentos en naranjo, textos en papel y humo, líneas finas y las tipografías Archivo Expanded, Stack Sans Text e IBM Plex Mono.

- Estilo **panel de equipo**: franja naranja arriba (textos en negro, como pide la marca) y tornillos en las esquinas.
- **Selector Desde** y **selector Hacia**: perillas de 10 posiciones (01 a 10). Se giran arrastrando, con la rueda del mouse, con las flechas del teclado, o haciendo clic en el número que quieras. Clic en la foto o el nombre abre la lista completa.
- Cada selector muestra la foto de la cápsula (Desde en blanco y negro, Hacia en el naranjo de la marca), su grupo, el nombre y una descripción corta. Las cápsulas en split se muestran con una foto en semitono; las completas, con grano.
- **Respuesta:** pantalla que muestra qué frecuencias sube o baja la combinación elegida, con valores de referencia y el cambio más grande marcado.
- **Perillas** de Mezcla, Brillo y Salida. Doble clic en una perilla la vuelve a su valor inicial.

Para revisar el diseño sin abrir un DAW: `cmake -B build -DQUADRO_UI_SNAPSHOT=ON` arma `QuadroUISnapshot`, que guarda una imagen PNG de la interfaz.

## Modelos

| Modelo | Carácter |
|---|---|
| Strato vintage | La más brillante, con el clásico brillo de Strato. |
| Strato caliente | Single coil con más salida y un poco menos de brillo. |
| Tele puente | Single coil con más cuerpo en los medios agudos. |
| P90 | Single coil ancha, cálida. |
| Humbucker vintage (PAF) | Humbucker clásico. |
| Humbucker moderno | Humbucker con más salida. |
| Humbucker potente | Humbucker de alta salida, el más oscuro. |
| Split de humbucker vintage | Una sola bobina de un humbucker clásico. |
| Split de humbucker moderno | Una sola bobina de un humbucker moderno. |
| Split de humbucker potente | Una sola bobina de un humbucker de alta salida. |

## Cómo se usa

1. Pon el plugin **primero** en la pista de guitarra, antes del ampli y de cualquier efecto. La guitarra tiene que entrar directo a la interfaz, sin pedales.
2. En **Desde**, elige lo que tiene tu guitarra (por ejemplo, *Split de humbucker potente*).
3. En **Hacia**, elige lo que quieres escuchar (por ejemplo, *Strato vintage*).
4. Ajusta **Brillo** a gusto. Si la conversión suena muy brillante o con mucho ruido de púa, bájalo un poco.

Al convertir una cápsula oscura en una muy brillante (por ejemplo *Humbucker potente → Strato vintage*) el plugin sube bastante los agudos, y con ellos el ruido. Es normal: una Strato real también es más ruidosa.

## Cómo funciona

Eléctricamente, una cápsula pasiva se comporta como un filtro: la bobina (inductancia y resistencia) junto con la capacitancia de la bobina y del cable forman una resonancia, que es la "joroba" de brillo de cada cápsula. Los potes de la guitarra y las piezas metálicas de la cápsula (tornillos, tapa, la bobina apagada en un split) le quitan altura a esa joroba.

Cada modelo guarda esos valores eléctricos de referencia (aproximados, a partir de mediciones publicadas de cápsulas típicas). El plugin calcula la respuesta de la cápsula de origen y la de destino, "deshace" la primera y aplica la segunda en un solo filtro. Los graves no cambian.

Supuestos comunes: cable de guitarra de unos 470 pF y entrada de alta impedancia (1 MΩ). Las cápsulas single coil usan potes de 250k y las humbucker y P90 potes de 500k, como en las guitarras típicas.

**Lo que no cambia:** la posición de la cápsula en las cuerdas. Una cápsula de puente seguirá sonando a puente.

## Cómo se arma

GitHub Actions lo compila en cada cambio (ver `.github/workflows/build.yml`). Antes de compilar corre las pruebas de los filtros (`tools/pickup_test.cpp`): estabilidad, ganancia en graves, neutralidad cuando Desde = Hacia y parecido con el modelo analógico. El resultado queda como "artifact" descargable en la pestaña **Actions**.

Al instalar una versión nueva, hay que **cerrar Logic por completo** (no solo el proyecto) para que cargue el plugin actualizado.

## Créditos y licencia

- Estructura de plugin: [JUCE](https://juce.com) (AGPL v3).
- Tipografías: Archivo, Stack Sans Text e IBM Plex Mono, bajo SIL Open Font License 1.1 (ver `Resources/fonts/OFL-licenses.txt`).
- Fotos de Unsplash (licencia de Unsplash), recortadas y tratadas con Adobe: Seth Reese (Strato), Nestor Mehanikov (humbucker), Al Elmes (P90) y Chris Hardy (Tele).
- Marca e isotipo: Quadro.

Por eso este proyecto se distribuye bajo **AGPL v3**: el código es abierto.
