#pragma once

enum QuantizationLevels {
    Q8,
    Q4,
    Q3,
};

struct ModelOptions {
    short threadCount = 6;
    bool flashAttention = true;
};
