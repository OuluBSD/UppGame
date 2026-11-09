#ifndef UPP_INTERPOLATION_H
#define UPP_INTERPOLATION_H

#include <Core/Core.h>

NAMESPACE_UPP

// Interface for interpolation functions
class Interpolation {
public:
    virtual ~Interpolation() = default;
    
    // Interpolate between 0 and 1 based on input alpha (0.0 to 1.0)
    virtual double Apply(double a) const = 0;
    
    // Inverse interpolation - given the result, return the input value
    virtual double Inverse(double a) const { 
        // Default implementation - try to find inverse through bisection
        if (a <= 0) return 0;
        if (a >= 1) return 1;
        
        // Use bisection method to approximate inverse
        double low = 0.0, high = 1.0;
        for (int i = 0; i < 30; i++) {  // Max 30 iterations
            double mid = (low + high) / 2.0;
            double val = Apply(mid);
            if (abs(val - a) < 1e-10) return mid;  // Found close enough
            if (val < a) low = mid;
            else high = mid;
        }
        return (low + high) / 2.0;
    }
    
    // Interpolate between two values with given alpha
    double Apply(double start, double end, double a) const {
        return start + (end - start) * Apply(a);
    }
};

// Linear interpolation (identity function)
class Linear : public Interpolation {
public:
    double Apply(double a) const override {
        return a;
    }
};

// Sine-based interpolations
class Sine : public Interpolation {
public:
    double Apply(double a) const override {
        return 1.0 - cos(a * M_PI / 2.0);
    }
};

class SineIn : public Interpolation {
public:
    double Apply(double a) const override {
        return 1.0 - cos(a * M_PI / 2.0);
    }
};

class SineOut : public Interpolation {
public:
    double Apply(double a) const override {
        return sin(a * M_PI / 2.0);
    }
};

class SineInOut : public Interpolation {
public:
    double Apply(double a) const override {
        return 0.5 * (1.0 - cos(a * M_PI));
    }
};

// Exponential interpolations
class ExpIn : public Interpolation {
public:
    double Apply(double a) const override {
        return (a == 0.0) ? 0.0 : pow(2.0, 10.0 * (a - 1.0));
    }
};

class ExpOut : public Interpolation {
public:
    double Apply(double a) const override {
        return (a == 1.0) ? 1.0 : (-pow(2.0, -10.0 * a) + 1.0);
    }
};

class ExpInOut : public Interpolation {
public:
    double Apply(double a) const override {
        if (a <= 0.0 || a >= 1.0) return a;
        return (a < 0.5) ? 
            0.5 * pow(2.0, 20.0 * a - 10.0) : 
            -0.5 * pow(2.0, -20.0 * a + 10.0) + 1.0;
    }
};

// Power-based interpolations
class PowIn : public Interpolation {
private:
    double power;
public:
    explicit PowIn(double power) : power(power) {}
    double Apply(double a) const override {
        return pow(a, power);
    }
};

class PowOut : public Interpolation {
private:
    double power;
public:
    explicit PowOut(double power) : power(power) {}
    double Apply(double a) const override {
        return 1.0 - pow(1.0 - a, power);
    }
};

class PowInOut : public Interpolation {
private:
    double power;
public:
    explicit PowInOut(double power) : power(power) {}
    double Apply(double a) const override {
        if (a <= 0.5) return 0.5 * pow(a * 2.0, power);
        return 0.5 * (2.0 - pow(2.0 - a * 2.0, power));
    }
};

// Bounce interpolations
class BounceOut : public Interpolation {
public:
    double Apply(double a) const override {
        if (a < 4.0 / 11.0) {
            return (121.0 * a * a) / 16.0;
        } else if (a < 8.0 / 11.0) {
            return (363.0 / 40.0 * a * a) - (99.0 / 10.0 * a) + 17.0 / 5.0;
        } else if (a < 9.0 / 10.0) {
            return (4356.0 / 361.0 * a * a) - (35442.0 / 1805.0 * a) + 16061.0 / 1805.0;
        } else {
            return (54.0 / 5.0 * a * a) - (513.0 / 25.0 * a) + 268.0 / 25.0;
        }
    }
};

class BounceIn : public Interpolation {
public:
    double Apply(double a) const override {
        return 1.0 - BounceOut().Apply(1.0 - a);
    }
};

class BounceInOut : public Interpolation {
public:
    double Apply(double a) const override {
        if (a < 0.5) {
            return 0.5 * (1.0 - BounceOut().Apply(1.0 - a * 2.0));
        } else {
            return 0.5 * BounceOut().Apply(a * 2.0 - 1.0) + 0.5;
        }
    }
};

// Circle interpolations
class CircleIn : public Interpolation {
public:
    double Apply(double a) const override {
        return 1.0 - sqrt(1.0 - a * a);
    }
};

class CircleOut : public Interpolation {
    public:
    double Apply(double a) const override {
        return sqrt((2.0 - a) * a);
    }
};

class CircleInOut : public Interpolation {
public:
    double Apply(double a) const override {
        if (a < 0.5) {
            return 0.5 * (1.0 - sqrt(1.0 - 4.0 * a * a));
        } else {
            return 0.5 * sqrt(-((2.0 * a - 3.0) * (2.0 * a - 1.0))) + 0.5;
        }
    }
};

// Elastic interpolations
class ElasticIn : public Interpolation {
private:
    double value;
    double power;
    double b;
    double c;
    double a;
public:
    ElasticIn(double value = 2.0, double power = 10.0, double b = 0.0, double c = 0.0, double a = 0.0) 
        : value(value), power(power), b(b), c(c), a(a) {}
    double Apply(double a) const override {
        if (a <= 0.0 || a >= 1.0) return a;
        return -pow(2.0, power * (a - 1.0)) * sin((a - 1.0 - b / (2 * M_PI) * asin(1.0)) * (2.0 * M_PI) / b);
    }
};

class ElasticOut : public Interpolation {
public:
    double Apply(double a) const override {
        if (a <= 0.0 || a >= 1.0) return a;
        return pow(2.0, -power * a) * sin((a - b / (2.0 * M_PI) * asin(1.0)) * (2.0 * M_PI) / b) + 1.0;
    }
private:
    double power = 10.0;
    double b = 0.3;
};

class ElasticInOut : public Interpolation {
public:
    double Apply(double a) const override {
        if (a <= 0.0 || a >= 1.0) return a;
        a *= 2.0;
        if (a < 1.0) {
            return -0.5 * pow(2.0, power * (a - 1.0)) * sin((a - 1.0 - s / (2.0 * M_PI) * asin(1.0)) * (2.0 * M_PI) / s);
        }
        return pow(2.0, -power * (a - 1.0)) * sin((a - 1.0 - s / (2.0 * M_PI) * asin(1.0)) * (2.0 * M_PI) / s) * 0.5 + 1.0;
    }
private:
    double power = 10.0;
    double s = 0.3 * 0.75;
};

// Shared instances for common interpolations
namespace Interpolations {
    static const Linear linear;
    static const Sine sine;
    static const SineIn sineIn;
    static const SineOut sineOut;
    static const SineInOut sineInOut;
    static const ExpIn expIn;
    static const ExpOut expOut;
    static const ExpInOut expInOut;
    static const BounceIn bounceIn;
    static const BounceOut bounceOut;
    static const BounceInOut bounceInOut;
    static const CircleIn circleIn;
    static const CircleOut circleOut;
    static const CircleInOut circleInOut;
}

END_UPP_NAMESPACE

#endif