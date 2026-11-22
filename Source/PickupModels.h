#pragma once

// Modelos electricos de capsulas y filtro de conversion "Desde -> Hacia".
// Archivo sin dependencias de JUCE, para poder probarlo fuera del plugin (ver tools/).
//
// Idea fisica: una capsula pasiva se comporta como un filtro paso bajo de segundo orden.
// La bobina (inductancia L y resistencia R) queda en serie, y en la salida hay una capacitancia
// (la de la propia bobina + la del cable) y varias resistencias en paralelo: los potes de la guitarra,
// la entrada del ampli y las perdidas por corrientes parasitas en las piezas metalicas de la capsula.
//
//   H(s) = 1 / (L*C*s^2 + (R*C + L*G)*s + (1 + R*G))     con G = suma de conductancias en paralelo
//
// Para "cambiar de capsula" se divide por la respuesta de la capsula de origen y se multiplica por la
// de destino: Hacia(s) / Desde(s) = Ddesde(s) / Dhacia(s). Eso es un unico filtro de segundo orden
// (biquad), sin retraso. Si Desde y Hacia son iguales, el resultado es exactamente neutro.

#include <algorithm>
#include <array>
#include <cmath>

namespace pickup
{
constexpr double kPi = 3.14159265358979323846;

// Supuestos comunes a todos los modelos.
constexpr double kCableCapacitance = 470e-12; // cable de guitarra tipico (unos 4 a 5 metros)
constexpr double kAmpInput = 1.0e6;           // entrada de ampli o interfaz de alta impedancia

struct Model
{
    const char* name;
    double inductance;     // henrios
    double resistance;     // ohmios (resistencia DC de la bobina)
    double coilCapacitance;// faradios
    double lossResistance; // ohmios: perdidas por corrientes parasitas (tornillos, tapa, bobina apagada en split)
    double potValue;       // ohmios: potes de volumen y tono de la guitarra tipica de esa capsula
};

// Valores de referencia aproximados, a partir de mediciones publicadas de inductancia y resistencia
// de capsulas tipicas. Las perdidas se ajustaron para que la altura del pico sea realista.
inline const std::array<Model, 10>& models()
{
    static const std::array<Model, 10> table { {
        { "Strato vintage",             2.2,   5800.0, 110e-12, 1.5e6,  250e3 },
        { "Strato caliente",            2.8,   7500.0, 120e-12, 1.2e6,  250e3 },
        { "Tele puente",                3.0,   7000.0, 120e-12, 600e3,  250e3 },
        { "P90",                        7.0,   8200.0, 150e-12, 450e3,  500e3 },
        { "Humbucker vintage (PAF)",    4.2,   7800.0, 100e-12, 300e3,  500e3 },
        { "Humbucker moderno",          5.5,  10500.0, 120e-12, 280e3,  500e3 },
        { "Humbucker potente",          8.0,  14000.0, 150e-12, 250e3,  500e3 },
        { "Split de humbucker vintage", 1.85,  3900.0,  70e-12,  75e3,  500e3 },
        { "Split de humbucker moderno", 2.4,   5000.0,  75e-12,  95e3,  500e3 },
        { "Split de humbucker potente", 3.5,   7000.0,  80e-12, 110e3,  500e3 },
    } };
    return table;
}

inline int numModels() { return (int) models().size(); }

// Polinomio analogico a2*s^2 + a1*s + a0 (denominador de la respuesta de la capsula).
struct Poly
{
    double a2, a1, a0;
};

inline Poly denominator (const Model& m)
{
    const double C = m.coilCapacitance + kCableCapacitance;
    const double G = 1.0 / m.lossResistance + 2.0 / m.potValue + 1.0 / kAmpInput;
    return { m.inductance * C,
             m.resistance * C + m.inductance * G,
             1.0 + m.resistance * G };
}

// Frecuencia natural del polinomio (donde esta la resonancia de la capsula), en Hz.
inline double naturalFrequency (const Poly& p)
{
    return std::sqrt (p.a0 / p.a2) / (2.0 * kPi);
}

// Transformacion bilineal de un polinomio de segundo orden, con "pre-warping" en su propia
// frecuencia natural para que la resonancia quede exactamente donde corresponde.
// Devuelve los coeficientes en z^-1 (el factor comun (1 + z^-1)^2 se cancela en el cociente).
inline std::array<double, 3> bilinear (const Poly& p, double sampleRate)
{
    double w0 = std::sqrt (p.a0 / p.a2);
    w0 = std::min (w0, 0.9 * kPi * sampleRate);
    const double K = w0 / std::tan (w0 / (2.0 * sampleRate));
    const double a2K2 = p.a2 * K * K;
    const double a1K = p.a1 * K;
    return { a2K2 + a1K + p.a0,
             2.0 * (p.a0 - a2K2),
             a2K2 - a1K + p.a0 };
}

struct Biquad
{
    double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
};

// Filtro que convierte la capsula "from" en la capsula "to". Ganancia 1 en graves.
inline Biquad conversion (const Model& from, const Model& to, double sampleRate)
{
    const Poly ds = denominator (from);
    const Poly dt = denominator (to);
    const auto zs = bilinear (ds, sampleRate);
    const auto zt = bilinear (dt, sampleRate);
    const double g = dt.a0 / ds.a0;   // iguala el nivel en frecuencias bajas
    const double inv = 1.0 / zt[0];
    return { g * zs[0] * inv, g * zs[1] * inv, g * zs[2] * inv, zt[1] * inv, zt[2] * inv };
}

// Estante de agudos (formula RBJ) para el control "Brillo".
inline Biquad highShelf (double sampleRate, double frequency, double gainDb)
{
    if (std::abs (gainDb) < 1.0e-6)
        return {};

    const double A = std::pow (10.0, gainDb / 40.0);
    const double w0 = 2.0 * kPi * std::min (frequency, 0.45 * sampleRate) / sampleRate;
    const double cw = std::cos (w0), sw = std::sin (w0);
    const double alpha = sw / 2.0 * std::sqrt (2.0); // pendiente S = 1
    const double sqA2alpha = 2.0 * std::sqrt (A) * alpha;

    const double b0 =        A * ((A + 1.0) + (A - 1.0) * cw + sqA2alpha);
    const double b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cw);
    const double b2 =        A * ((A + 1.0) + (A - 1.0) * cw - sqA2alpha);
    const double a0 =             (A + 1.0) - (A - 1.0) * cw + sqA2alpha;
    const double a1 =  2.0 *     ((A - 1.0) - (A + 1.0) * cw);
    const double a2 =             (A + 1.0) - (A - 1.0) * cw - sqA2alpha;

    return { b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0 };
}

// Estado de un biquad en forma directa transpuesta II (en doble precision).
struct BiquadState
{
    double z1 = 0.0, z2 = 0.0;

    inline double process (const Biquad& c, double x)
    {
        const double y = c.b0 * x + z1;
        z1 = c.b1 * x - c.a1 * y + z2;
        z2 = c.b2 * x - c.a2 * y;
        return y;
    }

    void reset() { z1 = z2 = 0.0; }
};

// Cadena completa de un canal: conversion de capsula + brillo.
struct Chain
{
    Biquad conv, shelf;
    BiquadState convState, shelfState;

    inline double process (double x)
    {
        return shelfState.process (shelf, convState.process (conv, x));
    }

    void reset() { convState.reset(); shelfState.reset(); }
};

} // namespace pickup
