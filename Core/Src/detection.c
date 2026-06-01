#include "detection.h"
#include <math.h>

static const double B0 =  0.05;
static const double B1 =  0.00;
static const double B2 = -0.05;
static const double A1 = -1.80;
static const double A2 =  0.81;

static void welford_step(double x, uint32_t n,
                          double *mean, double *M2)
{
    double delta = x - *mean;
    *mean       += delta / (double)n;
    *M2         += delta * (x - *mean);
}

void Detector_Init(DetectorState *s)
{
    s->x1 = 0.0;  s->x2 = 0.0;
    s->y1 = 0.0;  s->y2 = 0.0;
    s->y_bpfilt  = 0.0;

    s->calib_count = 0u;
    s->calib_done  = false;

    s->raw_mean  = 0.0;  s->raw_M2  = 0.0;
    s->filt_mean = 0.0;  s->filt_M2 = 0.0;
    s->filt_std  = 0.0;

    s->baseline      = 0.0;
    s->threshold     = 0.0;
    s->deviation     = 0.0;
    s->abs_deviation = 0.0;
    s->metal_det     = false;
    s->confirm_count = 0u;
    s->hold_count    = 0u;
}

bool Detector_Update(DetectorState *s, double x_raw_hz)
{
    /* ── Band-pass filter ── */
    s->y_bpfilt = (B0 * x_raw_hz)
                + (B1 * s->x1)
                + (B2 * s->x2)
                - (A1 * s->y1)
                - (A2 * s->y2);

    s->x2 = s->x1;    s->x1 = x_raw_hz;
    s->y2 = s->y1;    s->y1 = s->y_bpfilt;

    /* ── Phase 1: Calibration ── */
    if (!s->calib_done)
    {
        s->calib_count++;

        welford_step(x_raw_hz, s->calib_count,
                     &s->raw_mean, &s->raw_M2);

        if (s->calib_count > 400u)
        {
            uint32_t filt_n = s->calib_count - 400u;
            welford_step(s->y_bpfilt, filt_n,
                         &s->filt_mean, &s->filt_M2);
        }

        if (s->calib_count >= CALIB_SAMPLES)
        {
            s->baseline  = s->raw_mean;

            uint32_t filt_n = s->calib_count - 20u;
            if (filt_n >= 2u) {
                s->filt_std = sqrt(s->filt_M2 / (double)(filt_n - 1u));
            } else {
                s->filt_std = 0.0;
            }

            s->threshold  = THRESHOLD_K * s->filt_std;
            s->calib_done = true;
        }

        s->metal_det     = false;
        s->deviation     = 0.0;
        s->abs_deviation = 0.0;
        s->confirm_count = 0u;
        s->hold_count    = 0u;
        return false;
    }

    /* ── Phase 2: Detection ─────────────────────────────────────────
     *
     *  KEY CHANGE: compare |deviation| against threshold so that
     *  BOTH positive and negative spikes are detected.
     *
     *  Metal pieces can cause either a frequency rise or drop
     *  depending on coil geometry and metal type:
     *    - Non-ferrous metals (Al, Cu) typically raise frequency
     *      → positive deviation
     *    - Some geometries or ferrous-adjacent materials lower it
     *      → negative deviation
     *
     *  The band-pass filter output is symmetric around zero, so
     *  using |deviation| catches both cases with the same threshold.
     *
     *  Release uses |deviation| < threshold * RELEASE_RATIO so
     *  hysteresis also works symmetrically in both directions.
     * ──────────────────────────────────────────────────────────── */
    s->deviation     = s->y_bpfilt;
    s->abs_deviation = fabs(s->deviation);

    if (!s->metal_det)
    {
        /* ── Rising edge with confirmation ── */
        if (s->abs_deviation > s->threshold)
        {
            s->confirm_count++;
            if (s->confirm_count >= CONFIRM_SAMPLES)
            {
                s->metal_det  = true;
                s->hold_count = HOLD_SAMPLES;   /* start hold timer */
            }
        }
        else
        {
            s->confirm_count = 0u;
        }
    }
    else
    {
        /* ── Hold phase ──────────────────────────────────────────────
         *
         *  While hold_count > 0, decrement it every sample and do NOT
         *  allow metal_det to go false — even if deviation drops below
         *  the release level.
         *
         *  This prevents the band-pass filter ringing (or a two-lobe
         *  physical response) from creating a brief false release in
         *  the middle of a single metal piece pass, which would cause
         *  the hit counter to increment twice for one piece.
         *
         *  Once hold_count reaches 0, normal hysteresis applies.
         * ────────────────────────────────────────────────────────── */
        if (s->hold_count > 0u)
        {
            s->hold_count--;
            /* Stay true — do not check release condition yet */
        }
        else
        {
            /* Hold expired — apply normal hysteresis release */
            if (s->abs_deviation < (s->threshold * RELEASE_RATIO))
            {
                s->metal_det     = false;
                s->confirm_count = 0u;
            }
        }
    }

    return s->metal_det;
}

bool Detector_CalibDone(const DetectorState *s)
{
    return s->calib_done;
}
