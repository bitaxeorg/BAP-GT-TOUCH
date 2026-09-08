#include "hashrate_chart_scale.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define HASHRATE_CHART_Y_TICK_COUNT 6

static int32_t nice_step_at_least(float minimum_step)
{
    static const float factors[] = {1.0f, 1.5f, 2.0f, 2.5f, 3.0f, 4.0f, 5.0f, 6.0f, 8.0f, 10.0f};

    if (minimum_step <= 1.0f)
    {
        return 1;
    }

    float magnitude = powf(10.0f, floorf(log10f(minimum_step)));
    float normalized = minimum_step / magnitude;
    for (size_t i = 0; i < sizeof(factors) / sizeof(factors[0]); i++)
    {
        if (normalized <= factors[i])
        {
            return (int32_t)ceilf(factors[i] * magnitude);
        }
    }

    return (int32_t)ceilf(10.0f * magnitude);
}

void hashrate_chart_scale_calculate(float minimum_value,
                                    float maximum_value,
                                    hashrate_chart_scale_t *scale)
{
    if (!scale)
    {
        return;
    }

    if (!isfinite(minimum_value) || !isfinite(maximum_value) ||
        minimum_value < 0.0f || maximum_value < minimum_value ||
        maximum_value > HASHRATE_CHART_MAX_VALUE)
    {
        scale->minimum = 0;
        scale->maximum = 100;
        scale->step = 20;
        return;
    }

    const int32_t intervals = HASHRATE_CHART_Y_TICK_COUNT - 1;
    float data_span = maximum_value - minimum_value;
    float padding = data_span * 0.05f;
    if (data_span < 1.0f)
    {
        padding = fmaxf(maximum_value * 0.01f, 1.0f);
    }

    float padded_minimum = fmaxf(minimum_value - padding, 0.0f);
    float padded_maximum = fminf(maximum_value + padding, HASHRATE_CHART_MAX_VALUE);
    int32_t step = nice_step_at_least((padded_maximum - padded_minimum) / intervals);

    for (;;)
    {
        int32_t axis_minimum = (int32_t)floorf(padded_minimum / step) * step;
        int32_t axis_maximum = axis_minimum + (step * intervals);
        if ((float)axis_maximum > HASHRATE_CHART_MAX_VALUE)
        {
            axis_maximum = ((int32_t)HASHRATE_CHART_MAX_VALUE / step) * step;
            axis_minimum = axis_maximum - (step * intervals);
        }

        if (axis_minimum >= 0 && (float)axis_minimum <= padded_minimum &&
            (float)axis_maximum >= padded_maximum &&
            (float)axis_maximum <= HASHRATE_CHART_MAX_VALUE)
        {
            scale->minimum = axis_minimum;
            scale->maximum = axis_maximum;
            scale->step = step;
            return;
        }

        step = nice_step_at_least((float)step + fmaxf(1.0f, step * 0.01f));
    }
}

bool hashrate_chart_value_parse(const char *text, float *value)
{
    if (!text || !value || text[0] == '\0' || strlen(text) >= 16)
    {
        return false;
    }

    char *end = NULL;
    float parsed = strtof(text, &end);
    if (end == text || *end != '\0' || !isfinite(parsed) ||
        parsed < 0.0f || parsed > HASHRATE_CHART_MAX_VALUE)
    {
        return false;
    }

    *value = parsed;
    return true;
}
