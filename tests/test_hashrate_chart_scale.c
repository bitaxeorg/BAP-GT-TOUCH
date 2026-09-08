#include "hashrate_chart_scale.h"
#include "hashrate_chart_layout.h"

#include <assert.h>
#include <stdio.h>
#include <math.h>

static void assert_scale(float data_minimum,
                         float data_maximum,
                         int32_t expected_minimum,
                         int32_t expected_maximum,
                         int32_t expected_step)
{
    hashrate_chart_scale_t scale;
    hashrate_chart_scale_calculate(data_minimum, data_maximum, &scale);

    assert(scale.minimum == expected_minimum);
    assert(scale.maximum == expected_maximum);
    assert(scale.step == expected_step);
    assert(scale.minimum <= data_minimum);
    assert(scale.maximum >= data_maximum);
    assert(scale.maximum - scale.minimum == scale.step * 5);
}

static int32_t map_value_to_y(int32_t value,
                              const hashrate_chart_scale_t *scale,
                              int32_t chart_height)
{
    int32_t offset = (value - scale->minimum) * chart_height;
    return chart_height - (offset / (scale->maximum - scale->minimum));
}

static void assert_rounded_range_controls_positions(void)
{
    hashrate_chart_scale_t scale;
    hashrate_chart_scale_calculate(1986.0f, 2584.0f, &scale);

    /* LVGL maps both tick labels and series points through the configured
     * minimum/maximum. Verify the asymmetric rounded range drives their
     * positions instead of only changing the displayed label text. */
    const int32_t chart_height = 250;
    for (int32_t tick = 0; tick < 6; tick++)
    {
        int32_t tick_value = scale.maximum - (tick * scale.step);
        assert(map_value_to_y(tick_value, &scale, chart_height) == tick * 50);
    }

    assert(map_value_to_y(1986, &scale, chart_height) == 238);
    assert(map_value_to_y(2250, &scale, chart_height) == 150);
    assert(map_value_to_y(2584, &scale, chart_height) == 39);
}

static void assert_value_parsing_bounds(void)
{
    float value = -1.0f;
    assert(hashrate_chart_value_parse("2105.25", &value));
    assert(fabsf(value - 2105.25f) < 0.01f);
    assert(hashrate_chart_value_parse("30000", &value));

    assert(!hashrate_chart_value_parse(NULL, &value));
    assert(!hashrate_chart_value_parse("", &value));
    assert(!hashrate_chart_value_parse("nan", &value));
    assert(!hashrate_chart_value_parse("inf", &value));
    assert(!hashrate_chart_value_parse("-1", &value));
    assert(!hashrate_chart_value_parse("30001", &value));
    assert(!hashrate_chart_value_parse("2105abc", &value));
    assert(!hashrate_chart_value_parse("0000000000002105", &value));
}

static void assert_layout_bounds(void)
{
    enum
    {
        screen_height = 480,
        axis_font_height = 16,
        max_digit_advance = 11,
        max_axis_label_digits = 5
    };

    /* The largest permitted label is 30000. Montserrat 16's widest digit
     * advances by less than 11 px, leaving space inside the 70 px gutter. */
    assert((max_axis_label_digits * max_digit_advance) +
           HASHRATE_CHART_AXIS_MAJOR_TICK_LENGTH <
           HASHRATE_CHART_LEFT_GUTTER);

    /* The lowest label is centered on the chart content's lower edge. Keep
     * its full glyph box above the top of the bottom navigation controls. */
    int32_t chart_height = screen_height - HASHRATE_CHART_HEIGHT_REDUCTION;
    int32_t chart_top = screen_height - HASHRATE_CHART_BOTTOM_OFFSET - chart_height;
    int32_t content_bottom = chart_top + HASHRATE_CHART_TOP_PADDING +
                             chart_height - HASHRATE_CHART_TOP_PADDING -
                             HASHRATE_CHART_BOTTOM_PADDING;
    int32_t lowest_label_bottom = content_bottom + (axis_font_height / 2);
    int32_t nav_top = screen_height - HASHRATE_CHART_NAV_HEIGHT;
    assert(lowest_label_bottom < nav_top);
}

static void assert_all_supported_ranges_stay_bounded(void)
{
    const int32_t chart_height = 480 - HASHRATE_CHART_HEIGHT_REDUCTION;
    const int32_t chart_content_height = chart_height -
                                         HASHRATE_CHART_TOP_PADDING -
                                         HASHRATE_CHART_BOTTOM_PADDING;

    for (int32_t minimum = 0; minimum <= 30000; minimum += 1000)
    {
        for (int32_t maximum = minimum; maximum <= 30000; maximum += 1000)
        {
            hashrate_chart_scale_t scale;
            hashrate_chart_scale_calculate((float)minimum, (float)maximum, &scale);
            assert(scale.minimum >= 0);
            assert(scale.minimum <= minimum);
            assert(scale.maximum >= maximum);
            assert(scale.maximum <= (int32_t)HASHRATE_CHART_MAX_VALUE);
            assert(scale.maximum - scale.minimum == scale.step * 5);

            /* The rounded bounds are the actual LVGL range, so every accepted
             * data endpoint must map inside the fixed chart content rectangle. */
            int32_t minimum_y = map_value_to_y(minimum, &scale, chart_content_height);
            int32_t maximum_y = map_value_to_y(maximum, &scale, chart_content_height);
            assert(minimum_y >= 0 && minimum_y <= chart_content_height);
            assert(maximum_y >= 0 && maximum_y <= chart_content_height);

            for (int32_t tick = 0; tick < 6; tick++)
            {
                int32_t tick_value = scale.minimum + (tick * scale.step);
                int32_t tick_y = map_value_to_y(tick_value, &scale, chart_content_height);
                assert(tick_y >= 0 && tick_y <= chart_content_height);
            }
        }
    }
}

int main(void)
{
    assert_scale(1986.0f, 2584.0f, 1950, 2700, 150);
    assert_scale(2105.0f, 2464.0f, 2000, 2500, 100);
    assert_scale(2053.0f, 2053.0f, 2030, 2080, 10);
    assert_scale(0.0f, 100.0f, 0, 125, 25);
    assert_scale(0.0f, 30000.0f, 0, 30000, 6000);
    assert_rounded_range_controls_positions();
    assert_value_parsing_bounds();
    assert_layout_bounds();
    assert_all_supported_ranges_stay_bounded();

    puts("hashrate chart scale tests passed");
    return 0;
}
