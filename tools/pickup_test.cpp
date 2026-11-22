// Pruebas de los filtros de Source/PickupModels.h, sin JUCE.
//
//   ./pickup_test                       -> corre todas las pruebas (sale con error si alguna falla)
//   ./pickup_test --process IN OUT SR DESDE HACIA [BRILLO_DB]
//                                       -> procesa un archivo de muestras float32 mono (crudo)
//
// Compilar:  clang++ -std=c++20 -O2 -o pickup_test tools/pickup_test.cpp

#include "../Source/PickupModels.h"

#include <complex>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace pickup;
using cd = std::complex<double>;

static int failures = 0;

static void check (bool ok, const std::string& what)
{
    if (! ok)
    {
        ++failures;
        std::printf ("FALLA: %s\n", what.c_str());
    }
}

static cd digitalResponse (const Biquad& c, double f, double fs)
{
    const cd z1 = std::polar (1.0, -2.0 * kPi * f / fs); // z^-1
    return (c.b0 + c.b1 * z1 + c.b2 * z1 * z1) / (1.0 + c.a1 * z1 + c.a2 * z1 * z1);
}

static cd analogResponse (const Model& from, const Model& to, double f)
{
    const Poly ds = denominator (from), dt = denominator (to);
    const cd s (0.0, 2.0 * kPi * f);
    const cd num = ds.a2 * s * s + ds.a1 * s + ds.a0;
    const cd den = dt.a2 * s * s + dt.a1 * s + dt.a0;
    return (dt.a0 / ds.a0) * num / den;
}

static double db (cd v) { return 20.0 * std::log10 (std::abs (v)); }

static bool stable (const Biquad& c)
{
    // Polos de z^2 + a1 z + a2: estables si ambos tienen modulo < 1.
    const cd disc = std::sqrt (cd (c.a1 * c.a1 - 4.0 * c.a2, 0.0));
    const cd p1 = (-c.a1 + disc) / 2.0, p2 = (-c.a1 - disc) / 2.0;
    return std::abs (p1) < 1.0 && std::abs (p2) < 1.0;
}

static int runTests()
{
    const double rates[] = { 44100.0, 48000.0, 88200.0, 96000.0, 192000.0 };
    const auto& ms = models();
    double worstError = 0.0;

    for (double fs : rates)
    {
        for (size_t a = 0; a < ms.size(); ++a)
        {
            for (size_t b = 0; b < ms.size(); ++b)
            {
                const Biquad c = conversion (ms[a], ms[b], fs);
                const std::string tag = std::string (ms[a].name) + " -> " + ms[b].name + " @ " + std::to_string ((int) fs);

                check (std::isfinite (c.b0) && std::isfinite (c.b1) && std::isfinite (c.b2)
                           && std::isfinite (c.a1) && std::isfinite (c.a2),
                       "coeficientes finitos " + tag);
                check (stable (c), "estable " + tag);
                check (std::abs (db (digitalResponse (c, 0.0, fs))) < 1e-9, "ganancia 1 en graves " + tag);

                if (a == b)
                    for (double f : { 100.0, 1000.0, 5000.0, 15000.0 })
                        check (std::abs (db (digitalResponse (c, f, fs))) < 1e-9, "neutro si Desde = Hacia " + tag);

                // Diferencia con el modelo analogico en la zona importante (hasta 12 kHz).
                for (double f = 20.0; f <= std::min (12000.0, 0.4 * fs); f *= 1.02)
                {
                    const double e = std::abs (db (digitalResponse (c, f, fs)) - db (analogResponse (ms[a], ms[b], f)));
                    worstError = std::max (worstError, e);
                    if (e > 1.0)
                    {
                        check (false, "diferencia > 1 dB con el modelo analogico a " + std::to_string ((int) f) + " Hz " + tag);
                        break;
                    }
                }
            }
        }

        const Biquad flat = highShelf (fs, 5000.0, 0.0);
        check (flat.b0 == 1.0 && flat.b1 == 0.0 && flat.a1 == 0.0, "brillo 0 dB es neutro");
        const Biquad up = highShelf (fs, 5000.0, 6.0);
        check (std::abs (db (digitalResponse (up, 0.45 * fs, fs)) - 6.0) < 0.3, "brillo +6 dB en agudos");
        check (std::abs (db (digitalResponse (up, 100.0, fs))) < 0.1, "brillo no toca los graves");
    }

    std::printf ("Peor diferencia con el modelo analogico (20 Hz - 12 kHz): %.3f dB\n", worstError);

    // Resumen legible: respuesta de cada capsula (con su carga tipica) y algunas conversiones.
    std::printf ("\nCapsulas (respuesta con cable de 470 pF):\n");
    for (const auto& m : ms)
    {
        const Poly p = denominator (m);
        double peakDb = -100.0, peakF = 0.0, f3 = 0.0;
        for (double f = 20.0; f < 20000.0; f *= 1.005)
        {
            const cd s (0.0, 2.0 * kPi * f);
            const double g = db (p.a0 / (p.a2 * s * s + p.a1 * s + p.a0));
            if (g > peakDb) { peakDb = g; peakF = f; }
            if (f3 == 0.0 && g < -3.0) f3 = f;
        }
        std::printf ("  %-28s pico %+4.1f dB en %5.0f Hz, -3 dB en %5.0f Hz\n", m.name, peakDb, peakF, f3);
    }

    std::printf ("\nConversion Split de humbucker potente -> Strato vintage (48 kHz):\n");
    const Biquad c = conversion (ms[9], ms[0], 48000.0);
    for (double f : { 100.0, 500.0, 1000.0, 2000.0, 3000.0, 4000.0, 5000.0, 6000.0, 8000.0, 10000.0, 15000.0 })
        std::printf ("  %6.0f Hz: %+5.1f dB\n", f, db (digitalResponse (c, f, 48000.0)));

    if (failures == 0)
        std::printf ("\nTodas las pruebas pasaron.\n");
    else
        std::printf ("\n%d pruebas fallaron.\n", failures);

    return failures == 0 ? 0 : 1;
}

static int processFile (int argc, char** argv)
{
    if (argc < 7)
    {
        std::printf ("uso: pickup_test --process IN OUT SR DESDE HACIA [BRILLO_DB]\n");
        return 2;
    }

    const double fs = std::atof (argv[4]);
    const int from = std::atoi (argv[5]);
    const int to = std::atoi (argv[6]);
    const double brightness = argc > 7 ? std::atof (argv[7]) : 0.0;

    std::FILE* in = std::fopen (argv[2], "rb");
    if (in == nullptr) { std::printf ("no se pudo abrir %s\n", argv[2]); return 2; }
    std::vector<float> samples;
    float buf[4096];
    size_t n;
    while ((n = std::fread (buf, sizeof (float), 4096, in)) > 0)
        samples.insert (samples.end(), buf, buf + n);
    std::fclose (in);

    Chain chain;
    chain.conv = conversion (models()[(size_t) from], models()[(size_t) to], fs);
    chain.shelf = highShelf (fs, 5000.0, brightness);
    for (auto& s : samples)
        s = (float) chain.process (s);

    std::FILE* out = std::fopen (argv[3], "wb");
    if (out == nullptr) { std::printf ("no se pudo escribir %s\n", argv[3]); return 2; }
    std::fwrite (samples.data(), sizeof (float), samples.size(), out);
    std::fclose (out);
    return 0;
}

int main (int argc, char** argv)
{
    if (argc > 1 && std::string (argv[1]) == "--process")
        return processFile (argc, argv);
    return runTests();
}
